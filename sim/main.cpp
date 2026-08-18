// Desktop simulator entry point. Runs the real display Controller on a single
// cooperative loop on the main thread (LVGL/SDL must stay on the main thread on
// macOS), driving the firmware's loop methods directly since the FreeRTOS tasks
// are no-ops in the simulator.
#include "ESPAsyncWebServer.h"
#include "SdlDriver.h"
#include <Arduino.h>
#include <cstdlib>
#include <cstring>
#include <display/core/Controller.h>
#include <display/plugins/ShotHistoryPlugin.h>
#include <display/ui/default/DefaultUI.h>

// The generated UI event handlers reference this global (see main.h on device).
Controller controller;

int main(int argc, char **argv) {
    // Optional: `--screenshot <path> [delayMs]` renders for a bit, saves a BMP, exits.
    const char *shotPath = nullptr;
    const char *startScreen = nullptr;
    unsigned long shotDelayMs = 4000;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            shotPath = argv[++i];
            if (i + 1 < argc)
                shotDelayMs = strtoul(argv[i + 1], nullptr, 10);
        } else if (strcmp(argv[i], "--screen") == 0 && i + 1 < argc) {
            startScreen = argv[++i];
        }
    }

    controller.setup(); // builds the UI, installs the SDL driver, marks screen ready

    // The sim has a real network (the WebUI is reachable), so present as Wi-Fi
    // connected: seeding credentials sends setupWifi() down the STA path, and the
    // WiFi shim's begin() reports WL_CONNECTED. This makes the standby screen show
    // the clock and Wi-Fi icon (and avoids captive-portal AP mode). Seed only once.
    Settings &settings = controller.getSettings();
    if (settings.getWifiSsid().isEmpty())
        settings.setWifiSsid("GaggiMate-Sim");
    if (settings.getWifiPassword().isEmpty())
        settings.setWifiPassword("simulator");

    SdlDriver *drv = SdlDriver::getInstance();
    DefaultUI *ui = controller.getUI();
    const unsigned long start = millis();
    bool shotTaken = false;
    bool startScreenApplied = false;

    while (!drv->shouldQuit()) {
        controller.loop();      // connection lifecycle, comms pump, plugins
        controller.loopLogic(); // process + control logic (normally a FreeRTOS task)
        if (ui && startScreen && !startScreenApplied && millis() - start >= 500) {
            if (strcmp(startScreen, "standby") == 0) ui->changeScreen(SCREEN_ID_STANDBY_SCREEN);
            else if (strcmp(startScreen, "menu") == 0) ui->changeScreen(SCREEN_ID_MENU_SCREEN_NEW);
            else if (strcmp(startScreen, "brew") == 0) {
                controller.setMode(MODE_BREW);
                ui->changeScreen(SCREEN_ID_BREW_SCREEN);
            }
            else if (strcmp(startScreen, "steam") == 0) ui->changeScreen(SCREEN_ID_STEAM_SCREEN);
            else if (strcmp(startScreen, "water") == 0) ui->changeScreen(SCREEN_ID_WATER_SCREEN);
            else if (strcmp(startScreen, "grind") == 0) ui->changeScreen(SCREEN_ID_GRIND_SCREEN);
            else if (strcmp(startScreen, "status") == 0 || strcmp(startScreen, "preinfuse") == 0 ||
                     strcmp(startScreen, "brewing") == 0 || strcmp(startScreen, "complete") == 0) {
                controller.setMode(MODE_BREW);
                ui->changeScreen(SCREEN_ID_STATUS_SCREEN);
            }
            startScreenApplied = true;
        }

        // Shot history sampling normally runs in its own FreeRTOS task (a no-op in
        // the sim), so drive record() here at its native cadence.
        {
            static unsigned long lastShotSample = 0;
            if (millis() - lastShotSample >= SHOT_LOG_SAMPLE_INTERVAL_MS) {
                lastShotSample = millis();
                ShotHistory.record();
            }
        }

        if (ui) {
            ui->loop();
            ui->loopProfiles();
            if (startScreenApplied && startScreen != nullptr) {
                if (strcmp(startScreen, "preinfuse") == 0)
                    ui->previewConceptStatus(2.2f, 0.0f, 2.5f, "INFUSION", false);
                else if (strcmp(startScreen, "brewing") == 0)
                    ui->previewConceptStatus(13.3f, 25.1f, 9.0f, "BREW", false);
                else if (strcmp(startScreen, "complete") == 0)
                    ui->previewConceptStatus(17.3f, 36.0f, 0.4f, "COMPLETE", true);
            }
        }
        gm_web_pump(); // service the embedded WebUI HTTP/WS server

        // Settings persistence normally runs in a deferred save task (a no-op in the
        // sim), so flush dirty settings to NVS periodically. save(true) is a cheap
        // no-op when nothing changed.
        {
            static unsigned long lastSave = 0;
            if (millis() - lastSave >= 2000) {
                lastSave = millis();
                controller.getSettings().save(true);
            }
        }

        drv->pumpAndRender();

        if (shotPath && !shotTaken && millis() - start >= shotDelayMs) {
            drv->screenshot(shotPath);
            shotTaken = true;
            break;
        }
        delay(5);
    }
    return 0;
}
