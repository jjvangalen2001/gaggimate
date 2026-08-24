#include "ConceptUI.h"

#include <WiFi.h>
#include <cstdarg>
#include <cmath>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <display/core/Controller.h>
#include <display/ui/default/DefaultUI.h>
#include <display/ui/default/eez/images.h>

namespace {
constexpr int SCREEN_SIZE = 480;
// Figma LEDRing spans x/y 10..470 in its 480 px frame.
constexpr int RING_RADIUS = 230;
constexpr int RING_TICK_LENGTH = 7;
constexpr lv_color_t WHITE = LV_COLOR_MAKE(0xff, 0xff, 0xff);

const char *safeText(const char *text, const char *fallback = "") {
    return text != nullptr && text[0] != '\0' ? text : fallback;
}

bool setLabelTextIfChanged(lv_obj_t *label, const char *text) {
    const char *next = safeText(text);
    const char *current = lv_label_get_text(label);
    if (current != nullptr && std::strcmp(current, next) == 0) return false;
    lv_label_set_text(label, next);
    return true;
}

bool setLabelTextFmtIfChanged(lv_obj_t *label, const char *format, ...) {
    char text[96];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    return setLabelTextIfChanged(label, text);
}

void setImageSourceIfChanged(lv_obj_t *image, const lv_img_dsc_t *source) {
    if (lv_img_get_src(image) != source) lv_img_set_src(image, source);
}

void uppercaseCopy(char *destination, size_t size, const char *source) {
    if (size == 0) return;
    const char *input = safeText(source, "BREW");
    size_t i = 0;
    for (; i + 1 < size && input[i] != '\0'; i++)
        destination[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(input[i])));
    destination[i] = '\0';
}

void enableGestureBubble(lv_obj_t *object) {
    const uint32_t count = lv_obj_get_child_cnt(object);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t *child = lv_obj_get_child(object, static_cast<int32_t>(i));
        lv_obj_add_flag(child, LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
        enableGestureBubble(child);
    }
}

lv_obj_t *makeLabel(lv_obj_t *parent, const lv_font_t *font, lv_color_t color) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    return label;
}

void makeRoundButton(lv_obj_t *button, int size) {
    lv_obj_set_size(button, size, size);
    lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x080808), 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_80, 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_border_color(button, lv_color_hex(0x545454), 0);
    lv_obj_set_style_shadow_width(button, 18, 0);
    lv_obj_set_style_shadow_opa(button, LV_OPA_30, 0);
}

void drawMenuIcon(lv_obj_t *canvas, int icon) {
    lv_canvas_fill_bg(canvas, LV_COLOR_CHROMA_KEY, LV_OPA_COVER);
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.color = lv_color_hex(0xe6e6e6);
    line.width = 2;
    line.round_start = true;
    line.round_end = true;

    auto path = [&](const lv_point_t *points, uint32_t count) { lv_canvas_draw_line(canvas, points, count, &line); };
    if (icon == 0) {
        // Brew cup and handle, matching the concept's thin-line SVG.
        const lv_point_t cup[] = {{7, 15}, {25, 15}, {25, 24}, {23, 28}, {20, 29}, {12, 29}, {8, 27}, {7, 24}, {7, 15}};
        const lv_point_t handle[] = {{25, 17}, {29, 17}, {31, 19}, {31, 21}, {29, 23}, {25, 23}};
        path(cup, 9);
        path(handle, 6);
        line.width = 1;
        const lv_point_t steam1[] = {{13, 11}, {14, 9}, {14, 8}};
        const lv_point_t steam2[] = {{18, 11}, {19, 9}, {19, 8}};
        const lv_point_t steam3[] = {{23, 11}, {24, 9}, {24, 8}};
        path(steam1, 3); path(steam2, 3); path(steam3, 3);
    } else if (icon == 1) {
        // Exact three-drop layout from MenuIcons.drops (36x36 viewBox).
        line.width = 1;
        const lv_point_t left[] = {{11, 10}, {8, 13}, {8, 16}, {9, 19}, {11, 21}, {13, 18}, {15, 15}, {14, 12}, {11, 10}};
        const lv_point_t middle[] = {{18, 13}, {15, 16}, {14, 19}, {15, 22}, {18, 25}, {21, 22}, {22, 19}, {21, 16}, {18, 13}};
        const lv_point_t right[] = {{25, 10}, {22, 13}, {22, 16}, {23, 19}, {25, 21}, {27, 18}, {29, 15}, {28, 12}, {25, 10}};
        path(left, 9); path(middle, 9); path(right, 9);
    } else if (icon == 2) {
        // Water mode uses the concept's three hot-water droplets, not an invented underline.
        line.width = 1;
        const lv_point_t left[] = {{11, 9}, {7, 14}, {8, 18}, {11, 21}, {14, 18}, {15, 14}, {11, 9}};
        const lv_point_t right[] = {{25, 9}, {21, 14}, {22, 18}, {25, 21}, {28, 18}, {29, 14}, {25, 9}};
        const lv_point_t low[] = {{18, 16}, {14, 21}, {15, 25}, {18, 28}, {21, 25}, {22, 21}, {18, 16}};
        path(left, 7); path(right, 7); path(low, 7);
    } else {
        // Sample the SVG ellipse (rx=11, ry=7, rotation=-25°) instead of a rough polygon.
        lv_point_t bean[25];
        constexpr float rot = -25.0f * static_cast<float>(M_PI) / 180.0f;
        for (int i = 0; i <= 24; i++) {
            const float a = i * 2.0f * static_cast<float>(M_PI) / 24.0f;
            const float ex = 11.0f * std::cos(a), ey = 7.0f * std::sin(a);
            bean[i].x = static_cast<lv_coord_t>(18 + ex * std::cos(rot) - ey * std::sin(rot));
            bean[i].y = static_cast<lv_coord_t>(18 + ex * std::sin(rot) + ey * std::cos(rot));
        }
        path(bean, 25);
        line.width = 1;
        line.color = lv_color_hex(0x9a9a9a);
        const lv_point_t seam[] = {{14, 12}, {16, 16}, {16, 21}, {14, 25}};
        path(seam, 4);
        const lv_point_t seam2[] = {{22, 11}, {20, 15}, {20, 20}, {22, 24}};
        path(seam2, 4);
    }
}

void drawPrimaryIcon(lv_obj_t *canvas, bool active, lv_color_t color) {
    lv_canvas_fill_bg(canvas, LV_COLOR_CHROMA_KEY, LV_OPA_COVER);
    if (active) {
        for (int y = 4; y <= 13; y++)
            for (int x = 4; x <= 13; x++) lv_canvas_set_px_color(canvas, x, y, color);
    } else {
        for (int y = 2; y <= 16; y++) {
            const int half = 7 - std::abs(y - 9);
            const int endX = 4 + (half * 11 / 7);
            for (int x = 4; x <= endX; x++) lv_canvas_set_px_color(canvas, x, y, color);
        }
    }
}

void drawTargetIcon(lv_obj_t *canvas, lv_color_t color) {
    lv_canvas_fill_bg(canvas, LV_COLOR_CHROMA_KEY, LV_OPA_COVER);
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.color = color;
    line.width = 1;
    line.round_start = true;
    line.round_end = true;
    const lv_point_t horizontal[] = {{1, 6}, {11, 6}};
    const lv_point_t vertical[] = {{6, 1}, {6, 11}};
    const lv_point_t corner[] = {{1, 3}, {3, 3}, {3, 1}};
    lv_canvas_draw_line(canvas, horizontal, 2, &line);
    lv_canvas_draw_line(canvas, vertical, 2, &line);
    lv_canvas_draw_line(canvas, corner, 3, &line);
}

void drawContextIcon(lv_obj_t *canvas, int icon, lv_color_t color) {
    lv_canvas_fill_bg(canvas, LV_COLOR_CHROMA_KEY, LV_OPA_COVER);
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.color = color;
    line.width = 2;
    line.round_start = true;
    line.round_end = true;
    auto path = [&](const lv_point_t *points, uint32_t count) { lv_canvas_draw_line(canvas, points, count, &line); };
    if (icon == 0) {
        const lv_point_t a[] = {{3, 7}, {6, 5}, {9, 7}, {12, 9}, {15, 7}, {18, 5}, {21, 7}, {24, 9}};
        const lv_point_t b[] = {{3, 14}, {6, 12}, {9, 14}, {12, 16}, {15, 14}, {18, 12}, {21, 14}, {24, 16}};
        const lv_point_t c[] = {{3, 21}, {6, 19}, {9, 21}, {12, 23}, {15, 21}, {18, 19}, {21, 21}, {24, 23}};
        path(a, 8); path(b, 8); path(c, 8);
    } else if (icon == 1) {
        const lv_point_t drop[] = {{14, 2}, {9, 9}, {6, 15}, {7, 21}, {10, 25}, {14, 26},
                                   {18, 25}, {21, 21}, {22, 15}, {19, 9}, {14, 2}};
        path(drop, 11);
        line.width = 1;
        const lv_point_t shine[] = {{10, 19}, {11, 22}, {14, 23}};
        path(shine, 3);
    } else if (icon == 2) {
        const lv_point_t steam1[] = {{5, 25}, {6, 19}, {10, 14}, {10, 7}};
        const lv_point_t steam2[] = {{13, 26}, {14, 19}, {18, 14}, {18, 6}};
        const lv_point_t steam3[] = {{22, 25}, {21, 19}, {18, 15}};
        path(steam1, 4); path(steam2, 4); path(steam3, 3);
    } else {
        // Timer icon from CtxIcons.timer (22x24), centred in the 28px canvas.
        lv_point_t timerCircle[25];
        for (int i = 0; i <= 24; i++) {
            const float a = i * 2.0f * static_cast<float>(M_PI) / 24.0f;
            timerCircle[i].x = static_cast<lv_coord_t>(14 + std::cos(a) * 8.0f);
            timerCircle[i].y = static_cast<lv_coord_t>(16 + std::sin(a) * 8.0f);
        }
        path(timerCircle, 25);
        const lv_point_t hand1[] = {{14, 16}, {14, 11}};
        const lv_point_t hand2[] = {{14, 16}, {18, 19}};
        const lv_point_t crown[] = {{10, 3}, {18, 3}};
        const lv_point_t stem[] = {{14, 3}, {14, 7}};
        path(hand1, 2); path(hand2, 2); path(crown, 2); path(stem, 2);
    }
}

void setZoom(void *obj, int32_t value) { lv_obj_set_style_transform_zoom(static_cast<lv_obj_t *>(obj), value, 0); }
void setRotation(void *obj, int32_t value) { lv_img_set_angle(static_cast<lv_obj_t *>(obj), value); }
void setTranslateY(void *obj, int32_t value) { lv_obj_set_style_translate_y(static_cast<lv_obj_t *>(obj), value, 0); }
void setGlowOpacity(void *obj, int32_t value) { lv_obj_set_style_bg_opa(static_cast<lv_obj_t *>(obj), value, 0); }

float clamp01(float value) { return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value); }

uint8_t opacity(float value) { return static_cast<uint8_t>(clamp01(value) * 255.0f); }

float triangle(float phase) { return phase < 0.5f ? phase * 2.0f : (1.0f - phase) * 2.0f; }

void canvasPath(lv_obj_t *canvas, const lv_point_t *points, uint32_t count, lv_color_t color, int width) {
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.color = color;
    line.width = width;
    line.round_start = true;
    line.round_end = true;
    lv_canvas_draw_line(canvas, points, count, &line);
}

void drawSleepingEye(lv_obj_t *canvas) {
    lv_canvas_fill_bg(canvas, LV_COLOR_CHROMA_KEY, LV_OPA_COVER);
    lv_point_t outer[25], lower[19];
    for (int i = 0; i < 25; i++) {
        const float t = i / 24.0f;
        outer[i] = {static_cast<lv_coord_t>(3 + 32 * t),
                    static_cast<lv_coord_t>(14 - 12 * 4 * t * (1 - t))};
    }
    for (int i = 0; i < 19; i++) {
        const float t = i / 18.0f;
        lower[i] = {static_cast<lv_coord_t>(10 + 18 * t),
                    static_cast<lv_coord_t>(15 - 5 * 4 * t * (1 - t))};
    }
    canvasPath(canvas, outer, 25, lv_color_hex(0xd4aa3a), 3);
    canvasPath(canvas, lower, 19, lv_color_hex(0x766021), 1);
}

void fillHeart(lv_obj_t *canvas, int width, int height, lv_color_t color) {
    lv_canvas_fill_bg(canvas, LV_COLOR_CHROMA_KEY, LV_OPA_COVER);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const float nx = (2.0f * x - width) / static_cast<float>(width);
            const float ny = (2.0f * (height - 1 - y) - height * 0.1f) / static_cast<float>(height);
            const float a = nx * nx + ny * ny - 0.62f;
            if (a * a * a - nx * nx * ny * ny * ny <= 0.0f) lv_canvas_set_px_color(canvas, x, y, color);
        }
    }
}

void drawTongue(lv_obj_t *canvas) {
    lv_canvas_fill_bg(canvas, LV_COLOR_CHROMA_KEY, LV_OPA_COVER);
    for (int y = 2; y <= 13; y++) {
        const int inset = y < 5 ? (5 - y) : (y > 10 ? y - 10 : 0);
        for (int x = 3 + inset; x <= 17 - inset; x++) lv_canvas_set_px_color(canvas, x, y, lv_color_hex(0x5ab4d4));
    }
}

void drawSmile(lv_obj_t *canvas) {
    lv_canvas_fill_bg(canvas, LV_COLOR_CHROMA_KEY, LV_OPA_COVER);
    lv_point_t points[29];
    for (int i = 0; i < 29; i++) {
        const float t = i / 28.0f;
        points[i] = {static_cast<lv_coord_t>(5 + 34 * t),
                     static_cast<lv_coord_t>(5 + 15 * 4 * t * (1 - t))};
    }
    canvasPath(canvas, points, 29, lv_color_hex(0x6c6c6c), 2);
}

void drawStandbyHint(lv_obj_t *canvas) {
    lv_canvas_fill_bg(canvas, LV_COLOR_CHROMA_KEY, LV_OPA_COVER);
    const lv_color_t color = lv_color_hex(0x383838);
    const lv_point_t arrow[] = {{12, 2}, {12, 12}};
    const lv_point_t head[] = {{7, 8}, {12, 2}, {17, 8}};
    canvasPath(canvas, arrow, 2, color, 1);
    canvasPath(canvas, head, 3, color, 1);
    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.bg_opa = LV_OPA_TRANSP;
    rect.border_opa = LV_OPA_COVER;
    rect.border_color = color;
    rect.border_width = 1;
    rect.radius = 4;
    lv_canvas_draw_rect(canvas, 4, 14, 16, 12, &rect);
    rect.bg_opa = LV_OPA_COVER;
    rect.radius = LV_RADIUS_CIRCLE;
    rect.border_width = 0;
    lv_canvas_draw_rect(canvas, 10, 18, 4, 4, &rect);
}

void drawStatusChartFill(lv_event_t *event) {
    lv_obj_t *chartObject = lv_event_get_target(event);
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(event);
    if (dsc == nullptr || dsc->part != LV_PART_ITEMS || dsc->p1 == nullptr || dsc->p2 == nullptr ||
        dsc->line_dsc == nullptr)
        return;

    lv_draw_mask_line_param_t lineMask;
    lv_draw_mask_line_points_init(&lineMask, dsc->p1->x, dsc->p1->y, dsc->p2->x, dsc->p2->y,
                                  LV_DRAW_MASK_LINE_SIDE_BOTTOM);
    const int16_t lineMaskId = lv_draw_mask_add(&lineMask, nullptr);

    lv_draw_mask_fade_param_t fadeMask;
    lv_draw_mask_fade_init(&fadeMask, &chartObject->coords, LV_OPA_20, chartObject->coords.y1,
                           LV_OPA_TRANSP, chartObject->coords.y2);
    const int16_t fadeMaskId = lv_draw_mask_add(&fadeMask, nullptr);

    lv_draw_rect_dsc_t fill;
    lv_draw_rect_dsc_init(&fill);
    // The Figma graph fill is only a quiet tint. A lower opacity also prevents
    // the clipped final fill rectangle from reading as a vertical graph line.
    fill.bg_opa = LV_OPA_20;
    fill.bg_color = dsc->line_dsc->color;
    lv_area_t area = {dsc->p1->x, LV_MIN(dsc->p1->y, dsc->p2->y),
                      static_cast<lv_coord_t>(dsc->p2->x - 1), chartObject->coords.y2};
    lv_draw_rect(dsc->draw_ctx, &fill, &area);

    lv_draw_mask_remove_id(lineMaskId);
    lv_draw_mask_remove_id(fadeMaskId);
    lv_draw_mask_free_param(&lineMask);
    lv_draw_mask_free_param(&fadeMask);
}
} // namespace

ConceptUI::ConceptUI(Controller *controller, DefaultUI *owner) : controller(controller), owner(owner) {}

void ConceptUI::init() {
    root = lv_obj_create(nullptr);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x050505), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_add_event_cb(root, eventCallback, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(root, eventCallback, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(root, eventCallback, LV_EVENT_RELEASED, this);

    for (int i = 0; i < 2; i++) {
        stateBackgroundPixels[i] = static_cast<lv_color_t *>(ps_malloc(SCREEN_SIZE * 240 * sizeof(lv_color_t)));
        assert(stateBackgroundPixels[i] != nullptr);
        stateBackgroundImages[i].header.cf = LV_IMG_CF_TRUE_COLOR;
        stateBackgroundImages[i].header.w = SCREEN_SIZE;
        stateBackgroundImages[i].header.h = 240;
        stateBackgroundImages[i].data_size = SCREEN_SIZE * 240 * sizeof(lv_color_t);
        stateBackgroundImages[i].data = reinterpret_cast<const uint8_t *>(stateBackgroundPixels[i]);
    }
    stateBackground = lv_img_create(root);
    lv_img_set_src(stateBackground, &stateBackgroundImages[activeBackgroundBuffer]);
    lv_obj_align(stateBackground, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(stateBackground, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    applyStateGradient(0);

    buildRing();

    connection = makeLabel(root, &dm_mono_12, lv_color_hex(0x606060));
    lv_obj_align(connection, LV_ALIGN_TOP_MID, 0, 32);
    connectionIcons = lv_obj_create(root);
    lv_obj_set_size(connectionIcons, 44, 16);
    lv_obj_align(connectionIcons, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_opa(connectionIcons, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(connectionIcons, 0, 0);
    lv_obj_set_style_pad_all(connectionIcons, 0, 0);
    lv_obj_set_style_pad_column(connectionIcons, 10, 0);
    lv_obj_set_flex_flow(connectionIcons, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(connectionIcons, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(connectionIcons, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    wifiIcon = lv_img_create(connectionIcons);
    lv_img_set_src(wifiIcon, &concept_wifi);
    bluetoothIcon = lv_img_create(connectionIcons);
    lv_img_set_src(bluetoothIcon, &concept_bluetooth);

    profileRow = lv_obj_create(root);
    lv_obj_set_size(profileRow, 240, 18);
    lv_obj_align(profileRow, LV_ALIGN_TOP_MID, 0, 54);
    lv_obj_set_style_bg_opa(profileRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(profileRow, 0, 0);
    lv_obj_set_style_pad_all(profileRow, 0, 0);
    lv_obj_set_style_pad_column(profileRow, 6, 0);
    lv_obj_set_flex_flow(profileRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(profileRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(profileRow, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    profileLeft = lv_img_create(profileRow);
    lv_img_set_src(profileLeft, &concept_profile_left);
    lv_obj_set_style_img_opa(profileLeft, 46, 0);
    modeLabel = makeLabel(profileRow, &dm_sans_12_bold, lv_color_hex(0x9a9a9a));
    lv_obj_set_style_text_letter_space(modeLabel, 2, 0);
    profileRight = lv_img_create(profileRow);
    lv_img_set_src(profileRight, &concept_profile_right);
    lv_obj_set_style_img_opa(profileRight, 46, 0);

    stateRow = lv_obj_create(root);
    lv_obj_set_size(stateRow, 150, 18);
    lv_obj_align(stateRow, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_bg_opa(stateRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(stateRow, 0, 0);
    lv_obj_set_style_pad_all(stateRow, 0, 0);
    lv_obj_set_style_pad_column(stateRow, 5, 0);
    lv_obj_set_flex_flow(stateRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(stateRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(stateRow, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    stateSpinner = lv_img_create(stateRow);
    lv_img_set_src(stateSpinner, &concept_spinner);
    lv_obj_set_style_img_recolor(stateSpinner, lv_color_hex(0x4a8cf0), 0);
    lv_obj_set_style_img_recolor_opa(stateSpinner, LV_OPA_COVER, 0);
    lv_anim_t spinnerAnimation;
    lv_anim_init(&spinnerAnimation);
    lv_anim_set_var(&spinnerAnimation, stateSpinner);
    lv_anim_set_exec_cb(&spinnerAnimation, setRotation);
    lv_anim_set_values(&spinnerAnimation, 0, 3600);
    lv_anim_set_time(&spinnerAnimation, 1100);
    lv_anim_set_repeat_count(&spinnerAnimation, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&spinnerAnimation);
    stateLabel = makeLabel(stateRow, &dm_mono_9, lv_color_hex(0x4a8cf0));
    lv_obj_set_style_text_letter_space(stateLabel, 1, 0);

    contextIcon = lv_img_create(root);
    lv_img_set_src(contextIcon, &concept_heat);
    lv_obj_set_style_img_recolor(contextIcon, lv_color_hex(0x4a8cf0), 0);
    lv_obj_set_style_img_recolor_opa(contextIcon, LV_OPA_COVER, 0);
    lv_obj_align(contextIcon, LV_ALIGN_TOP_MID, 0, 120);

    statusTimerIcon = lv_img_create(root);
    lv_img_set_src(statusTimerIcon, &concept_timer);
    lv_obj_align(statusTimerIcon, LV_ALIGN_TOP_MID, 0, 99);
    lv_obj_set_style_img_opa(statusTimerIcon, 179, 0);

    statusMetrics = lv_obj_create(root);
    lv_obj_set_size(statusMetrics, 220, 24);
    lv_obj_align(statusMetrics, LV_ALIGN_TOP_MID, 0, 254);
    lv_obj_set_style_bg_opa(statusMetrics, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(statusMetrics, 0, 0);
    lv_obj_set_style_pad_all(statusMetrics, 0, 0);
    lv_obj_set_style_pad_column(statusMetrics, 6, 0);
    lv_obj_set_flex_flow(statusMetrics, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(statusMetrics, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(statusMetrics, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    statusWeight = makeLabel(statusMetrics, &dm_mono_17, lv_color_hex(0x292929));
    lv_obj_t *weightUnit = makeLabel(statusMetrics, &dm_sans_10_units, lv_color_hex(0x383838));
    lv_label_set_text(weightUnit, "g");
    lv_obj_t *metricDot = lv_obj_create(statusMetrics);
    lv_obj_set_size(metricDot, 2, 2);
    lv_obj_set_style_radius(metricDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(metricDot, 0, 0);
    lv_obj_set_style_bg_color(metricDot, lv_color_hex(0x191919), 0);
    lv_obj_set_style_pad_all(metricDot, 0, 0);
    statusPressureIcon = lv_img_create(statusMetrics);
    lv_img_set_src(statusPressureIcon, &concept_pressure);
    statusPressure = makeLabel(statusMetrics, &dm_mono_15, lv_color_hex(0x292929));
    lv_obj_t *pressureUnit = makeLabel(statusMetrics, &dm_sans_10_units, lv_color_hex(0x383838));
    lv_label_set_text(pressureUnit, "bar");

    mainValue = makeLabel(root, &dm_sans_88_bold, WHITE);
    lv_obj_align(mainValue, LV_ALIGN_CENTER, -18, 0);
    lv_obj_add_flag(mainValue, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(mainValue, eventCallback, LV_EVENT_CLICKED, this);

    mainDecimal = makeLabel(root, &dm_sans_38, lv_color_hex(0x888b91));
    lv_label_set_text(mainDecimal, ".0");
    mainUnit = makeLabel(root, &dm_sans_16_light, lv_color_hex(0x686b71));
    lv_label_set_text(mainUnit, "°C");

    targetRow = lv_obj_create(root);
    lv_obj_set_size(targetRow, 240, 18);
    lv_obj_align(targetRow, LV_ALIGN_TOP_MID, 0, 300);
    lv_obj_set_style_bg_opa(targetRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(targetRow, 0, 0);
    lv_obj_set_style_pad_all(targetRow, 0, 0);
    lv_obj_set_style_pad_column(targetRow, 5, 0);
    lv_obj_set_flex_flow(targetRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(targetRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(targetRow, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    targetIcon = lv_img_create(targetRow);
    lv_img_set_src(targetIcon, &concept_target);
    lv_obj_set_style_img_recolor(targetIcon, lv_color_hex(0x4a8cf0), 0);
    lv_obj_set_style_img_recolor_opa(targetIcon, LV_OPA_COVER, 0);
    secondaryValue = makeLabel(targetRow, &dm_sans_12, lv_color_hex(0x616161));
    targetDot = lv_obj_create(targetRow);
    lv_obj_set_size(targetDot, 2, 2);
    lv_obj_set_style_radius(targetDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(targetDot, 0, 0);
    lv_obj_set_style_bg_color(targetDot, lv_color_hex(0x242424), 0);
    lv_obj_set_style_pad_all(targetDot, 0, 0);
    lv_obj_clear_flag(targetDot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    targetPressureIcon = lv_img_create(targetRow);
    lv_img_set_src(targetPressureIcon, &concept_pressure);
    lv_obj_set_style_img_recolor(targetPressureIcon, lv_color_hex(0x4a8cf0), 0);
    lv_obj_set_style_img_recolor_opa(targetPressureIcon, LV_OPA_COVER, 0);
    targetPressure = makeLabel(targetRow, &dm_sans_12, lv_color_hex(0x616161));

    phaseLabel = makeLabel(root, &dm_mono_9, lv_color_hex(0xd82828));
    lv_obj_set_style_text_letter_space(phaseLabel, 2, 0);
    lv_obj_align(phaseLabel, LV_ALIGN_TOP_MID, 0, 61);

    chart = lv_chart_create(root);
    lv_obj_set_size(chart, 148, 80);
    lv_obj_align(chart, LV_ALIGN_TOP_MID, 0, 286);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 60);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_div_line_count(chart, 4, 0);
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_pad_all(chart, 0, 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0xd82828), LV_PART_MAIN);
    lv_obj_set_style_line_opa(chart, 24, LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 1, LV_PART_MAIN);
    lv_obj_set_style_line_dash_width(chart, 2, LV_PART_MAIN);
    lv_obj_set_style_line_dash_gap(chart, 3, LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 1, LV_PART_ITEMS);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
    pressureSeries = lv_chart_add_series(chart, lv_color_hex(0xd82828), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, pressureSeries, LV_CHART_POINT_NONE);
    lv_obj_add_event_cb(chart, drawStatusChartFill, LV_EVENT_DRAW_PART_BEGIN, nullptr);

    chartEndpoint = lv_obj_create(root);
    lv_obj_set_size(chartEndpoint, 4, 4);
    lv_obj_set_style_radius(chartEndpoint, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(chartEndpoint, 0, 0);
    lv_obj_set_style_pad_all(chartEndpoint, 0, 0);
    lv_obj_set_style_bg_color(chartEndpoint, lv_color_hex(0xd82828), 0);
    lv_obj_clear_flag(chartEndpoint, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    primaryButton = lv_btn_create(root);
    makeRoundButton(primaryButton, 50);
    lv_obj_align(primaryButton, LV_ALIGN_TOP_MID, 0, 344);
    lv_obj_add_event_cb(primaryButton, eventCallback, LV_EVENT_CLICKED, this);
    primaryIcon = makeLabel(primaryButton, &lv_font_montserrat_20, WHITE);
    lv_obj_add_flag(primaryIcon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_center(primaryIcon);
    primaryCanvas = lv_img_create(primaryButton);
    lv_img_set_src(primaryCanvas, &concept_play);
    lv_obj_set_style_img_recolor(primaryCanvas, lv_color_hex(0x777777), 0);
    lv_obj_set_style_img_recolor_opa(primaryCanvas, LV_OPA_COVER, 0);
    lv_obj_center(primaryCanvas);

    for (int i = 0; i < 5; i++) {
        profileDots[i] = lv_obj_create(root);
        lv_obj_set_size(profileDots[i], i == 0 ? 12 : 4, 4);
        lv_obj_align(profileDots[i], LV_ALIGN_BOTTOM_MID, (i - 2) * 12, -36);
        lv_obj_set_style_radius(profileDots[i], 2, 0);
        lv_obj_set_style_border_width(profileDots[i], 0, 0);
        lv_obj_set_style_bg_color(profileDots[i], WHITE, 0);
        lv_obj_set_style_bg_opa(profileDots[i], i == 0 ? LV_OPA_50 : LV_OPA_10, 0);
        lv_obj_clear_flag(profileDots[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }
    swipeUpHint = lv_img_create(root);
    lv_img_set_src(swipeUpHint, &concept_up_hint);
    lv_obj_set_style_img_opa(swipeUpHint, 41, 0);
    lv_obj_align(swipeUpHint, LV_ALIGN_BOTTOM_MID, 0, -45);

    editPanel = lv_obj_create(root);
    lv_obj_set_size(editPanel, 285, 185);
    lv_obj_align(editPanel, LV_ALIGN_CENTER, 0, 4);
    lv_obj_set_style_bg_opa(editPanel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(editPanel, 0, 0);
    lv_obj_clear_flag(editPanel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *tempName = makeLabel(editPanel, &dm_sans_9, lv_color_hex(0x777777));
    lv_label_set_text(tempName, "TEMP");
    lv_obj_align(tempName, LV_ALIGN_TOP_LEFT, 5, 18);
    lv_obj_t *targetName = makeLabel(editPanel, &dm_sans_9, lv_color_hex(0x777777));
    lv_label_set_text(targetName, "TARGET");
    lv_obj_align(targetName, LV_ALIGN_TOP_LEFT, 5, 83);
    editTempValue = makeLabel(editPanel, &lv_font_montserrat_18, WHITE);
    lv_obj_align(editTempValue, LV_ALIGN_TOP_MID, 30, 16);
    editTargetValue = makeLabel(editPanel, &lv_font_montserrat_18, WHITE);
    lv_obj_align(editTargetValue, LV_ALIGN_TOP_MID, 30, 81);
    for (int i = 0; i < 4; i++) {
        editButtons[i] = lv_btn_create(editPanel);
        makeRoundButton(editButtons[i], 38);
        const int row = i / 2;
        const int side = i % 2;
        lv_obj_align(editButtons[i], LV_ALIGN_TOP_LEFT, 78 + side * 153, 7 + row * 65);
        lv_obj_add_event_cb(editButtons[i], eventCallback, LV_EVENT_CLICKED, this);
        lv_obj_t *sign = makeLabel(editButtons[i], &lv_font_montserrat_18, WHITE);
        lv_label_set_text(sign, side == 0 ? "-" : "+");
        lv_obj_center(sign);
    }
    editDoneButton = lv_btn_create(editPanel);
    lv_obj_set_size(editDoneButton, 90, 36);
    lv_obj_align(editDoneButton, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_radius(editDoneButton, 18, 0);
    lv_obj_set_style_bg_color(editDoneButton, lv_color_hex(0x141414), 0);
    lv_obj_set_style_border_width(editDoneButton, 1, 0);
    lv_obj_set_style_border_color(editDoneButton, lv_color_hex(0x555555), 0);
    lv_obj_add_event_cb(editDoneButton, eventCallback, LV_EVENT_CLICKED, this);
    lv_obj_t *doneLabel = makeLabel(editDoneButton, &dm_sans_9, lv_color_hex(0xaaaaaa));
    lv_label_set_text(doneLabel, "DONE");
    lv_obj_center(doneLabel);

    brand = makeLabel(root, &dm_sans_30_bold, WHITE);
    lv_label_set_text(brand, "GAGGI");
    lv_obj_align(brand, LV_ALIGN_CENTER, -43, 0);
    brandMate = makeLabel(root, &dm_sans_30_light, lv_color_hex(0xa6a6a6));
    lv_label_set_text(brandMate, "MATE");
    lv_obj_align(brandMate, LV_ALIGN_CENTER, 52, 0);
    standbyError = makeLabel(root, &dm_sans_11, lv_color_hex(0x9a4040));
    lv_label_set_long_mode(standbyError, LV_LABEL_LONG_DOT);
    lv_obj_set_width(standbyError, 250);
    lv_obj_set_style_text_align(standbyError, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(standbyError, LV_ALIGN_CENTER, 0, 67);
    lv_obj_add_flag(standbyError, LV_OBJ_FLAG_HIDDEN);

    standbyClock = makeLabel(root, &dm_mono_13, lv_color_hex(0x454545));
    lv_label_set_text(standbyClock, "--:--");
    lv_obj_align(standbyClock, LV_ALIGN_TOP_MID, 0, 78);
    standbyProfileRow = lv_obj_create(root);
    lv_obj_set_size(standbyProfileRow, 220, 18);
    lv_obj_align(standbyProfileRow, LV_ALIGN_CENTER, 0, 35);
    lv_obj_set_style_bg_opa(standbyProfileRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(standbyProfileRow, 0, 0);
    lv_obj_set_style_pad_all(standbyProfileRow, 0, 0);
    lv_obj_set_style_pad_column(standbyProfileRow, 5, 0);
    lv_obj_set_flex_flow(standbyProfileRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(standbyProfileRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(standbyProfileRow, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    standbyTargetIcon = lv_img_create(standbyProfileRow);
    lv_img_set_src(standbyTargetIcon, &concept_target);
    lv_obj_set_style_img_opa(standbyTargetIcon, 46, 0);
    standbyProfile = makeLabel(standbyProfileRow, &dm_sans_11, lv_color_hex(0x393939));
    lv_label_set_text(standbyProfile, "ESPRESSO");
    lv_obj_t *standbyProfileDot = lv_obj_create(standbyProfileRow);
    lv_obj_set_size(standbyProfileDot, 2, 2);
    lv_obj_set_style_radius(standbyProfileDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(standbyProfileDot, 0, 0);
    lv_obj_set_style_bg_color(standbyProfileDot, lv_color_hex(0x393939), 0);
    lv_obj_set_style_pad_all(standbyProfileDot, 0, 0);
    standbyProfileTemp = makeLabel(standbyProfileRow, &dm_sans_11, lv_color_hex(0x393939));
    lv_label_set_text(standbyProfileTemp, "93°C");

    standbyFace = lv_obj_create(root);
    lv_obj_set_size(standbyFace, 160, 130);
    lv_obj_center(standbyFace);
    lv_obj_set_style_bg_opa(standbyFace, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(standbyFace, 0, 0);
    lv_obj_set_style_pad_all(standbyFace, 0, 0);
    lv_obj_clear_flag(standbyFace, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    for (int i = 0; i < 2; i++) {
        standbyEyes[i] = lv_img_create(root);
        lv_img_set_src(standbyEyes[i], &concept_sleeping_eye);
        lv_obj_set_pos(standbyEyes[i], i == 0 ? 184 : 258, 220);
        standbyHeartEyes[i] = lv_img_create(root);
        lv_img_set_src(standbyHeartEyes[i], &concept_heart_eye);
        lv_obj_set_pos(standbyHeartEyes[i], i == 0 ? 186 : 260, 213);
    }
    standbyMouth = lv_img_create(root);
    lv_img_set_src(standbyMouth, &concept_tongue);
    lv_obj_set_pos(standbyMouth, 230, 257);
    standbySmile = lv_img_create(root);
    lv_img_set_src(standbySmile, &concept_smile);
    lv_obj_set_pos(standbySmile, 218, 257);
    for (int i = 0; i < 2; i++) {
        standbyBlush[i] = lv_obj_create(root);
        lv_obj_set_size(standbyBlush[i], 22, 10);
        lv_obj_set_pos(standbyBlush[i], i == 0 ? 166 : 292, 243);
        lv_obj_set_style_radius(standbyBlush[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(standbyBlush[i], 0, 0);
        lv_obj_set_style_bg_color(standbyBlush[i], lv_color_hex(0xe8607a), 0);
    }
    // Z glyphs use the nearest enabled native sizes; the primary UI typography
    // remains the bundled DM Sans family.
    const lv_font_t *zFonts[] = {&lv_font_montserrat_14, &lv_font_montserrat_18, &lv_font_montserrat_24};
    const int zX[] = {98, 112, 124};
    const int zY[] = {30, 14, 0};
    for (int i = 0; i < 3; i++) {
        standbyZ[i] = makeLabel(root, zFonts[i], lv_color_hex(0xc8aa32));
        lv_label_set_text(standbyZ[i], "z");
        lv_obj_set_pos(standbyZ[i], 160 + zX[i], 175 + zY[i]);
    }
    const int heartX[] = {30, 80, 125};
    for (int i = 0; i < 3; i++) {
        standbyFloatingHearts[i] = lv_img_create(root);
        lv_img_set_src(standbyFloatingHearts[i], &concept_mini_heart);
        lv_obj_set_pos(standbyFloatingHearts[i], 160 + heartX[i], 195);
    }
    standbyHint = lv_img_create(root);
    lv_img_set_src(standbyHint, &concept_standby_hint);
    lv_obj_align(standbyHint, LV_ALIGN_BOTTOM_MID, 0, -62);

    standbyEnteredAt = lv_tick_get();
    buildMenu();

    standbyButton = lv_btn_create(root);
    lv_obj_set_size(standbyButton, 30, 30);
    // Figma 5:1111: the 22 px power glyph starts at y=402.
    lv_obj_align(standbyButton, LV_ALIGN_BOTTOM_MID, 0, -52);
    lv_obj_set_style_bg_opa(standbyButton, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(standbyButton, 0, 0);
    lv_obj_set_style_shadow_width(standbyButton, 0, 0);
    lv_obj_set_style_pad_all(standbyButton, 0, 0);
    lv_obj_add_event_cb(standbyButton, eventCallback, LV_EVENT_CLICKED, this);
    lv_obj_t *standbyIcon = lv_img_create(standbyButton);
    lv_img_set_src(standbyIcon, &concept_power);
    lv_obj_set_style_img_opa(standbyIcon, 107, 0);
    lv_obj_center(standbyIcon);
    menuDownHint = lv_img_create(root);
    lv_img_set_src(menuDownHint, &concept_down_hint);
    lv_obj_set_style_img_opa(menuDownHint, 38, 0);
    // Figma 5:1111: 14x9 down hint at x=233, y=441.
    lv_obj_align(menuDownHint, LV_ALIGN_TOP_MID, 0, 441);

    grindTargetRow = lv_obj_create(root);
    lv_obj_set_size(grindTargetRow, 170, 26);
    lv_obj_align(grindTargetRow, LV_ALIGN_TOP_MID, 0, 296);
    lv_obj_set_style_bg_opa(grindTargetRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grindTargetRow, 0, 0);
    lv_obj_set_style_pad_all(grindTargetRow, 0, 0);
    lv_obj_set_style_pad_column(grindTargetRow, 8, 0);
    lv_obj_set_flex_flow(grindTargetRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(grindTargetRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(grindTargetRow, LV_OBJ_FLAG_SCROLLABLE);
    grindMinus = makeLabel(grindTargetRow, &lv_font_montserrat_18, lv_color_hex(0xc89020));
    lv_label_set_text(grindMinus, "-");
    lv_obj_add_flag(grindMinus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(grindMinus, eventCallback, LV_EVENT_CLICKED, this);
    grindTargetTimer = lv_img_create(grindTargetRow);
    lv_img_set_src(grindTargetTimer, &concept_timer);
    lv_obj_set_style_img_recolor(grindTargetTimer, lv_color_hex(0xc89020), 0);
    lv_obj_set_style_img_recolor_opa(grindTargetTimer, LV_OPA_COVER, 0);
    grindTargetValue = makeLabel(grindTargetRow, &dm_sans_12, lv_color_hex(0xc89020));
    grindPlus = makeLabel(grindTargetRow, &lv_font_montserrat_18, lv_color_hex(0xc89020));
    lv_label_set_text(grindPlus, "+");
    lv_obj_add_flag(grindPlus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(grindPlus, eventCallback, LV_EVENT_CLICKED, this);
    applyView();
    updateStandbyFace();
    enableGestureBubble(root);
}

void ConceptUI::setEditing(bool enabled) {
    editing = enabled && view == View::Brew;
    if (editing) lv_obj_clear_flag(editPanel, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(editPanel, LV_OBJ_FLAG_HIDDEN);
    for (auto *obj : {stateRow, contextIcon, mainValue, mainDecimal, mainUnit, targetRow, primaryButton}) {
        if (editing) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        else if (view != View::Standby && view != View::Menu && view != View::Status) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    for (auto *dot : profileDots) {
        if (editing) lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
        else if (view == View::Brew) lv_obj_clear_flag(dot, LV_OBJ_FLAG_HIDDEN);
    }
    if (editing) lv_obj_add_flag(swipeUpHint, LV_OBJ_FLAG_HIDDEN);
    else if (view == View::Brew) lv_obj_clear_flag(swipeUpHint, LV_OBJ_FLAG_HIDDEN);
}

void ConceptUI::buildRing() {
    for (int i = 0; i < 80; i++) {
        const float angle = (i * 360.0f / 80.0f - 90.0f) * static_cast<float>(M_PI) / 180.0f;
        const int x1 = SCREEN_SIZE / 2 + static_cast<int>(std::lround(std::cos(angle) * (RING_RADIUS - RING_TICK_LENGTH)));
        const int y1 = SCREEN_SIZE / 2 + static_cast<int>(std::lround(std::sin(angle) * (RING_RADIUS - RING_TICK_LENGTH)));
        const int x2 = SCREEN_SIZE / 2 + static_cast<int>(std::lround(std::cos(angle) * RING_RADIUS));
        const int y2 = SCREEN_SIZE / 2 + static_cast<int>(std::lround(std::sin(angle) * RING_RADIUS));
        const int minX = LV_MIN(x1, x2);
        const int minY = LV_MIN(y1, y2);
        tickPoints[i][0].x = x1 - minX;
        tickPoints[i][0].y = y1 - minY;
        tickPoints[i][1].x = x2 - minX;
        tickPoints[i][1].y = y2 - minY;
        ticks[i] = lv_line_create(root);
        lv_line_set_points(ticks[i], tickPoints[i], 2);
        lv_obj_set_pos(ticks[i], minX, minY);
        lv_obj_set_style_line_width(ticks[i], 2, 0);
        lv_obj_set_style_line_rounded(ticks[i], true, 0);
        lv_obj_set_style_line_color(ticks[i], lv_color_hex(0x252525), 0);
        lv_obj_clear_flag(ticks[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }
}

void ConceptUI::buildMenu() {
    static const char *labels[] = {"BREW", "STEAM", "WATER", "GRIND"};
    static const lv_img_dsc_t *icons[] = {&concept_menu_cup, &concept_menu_drops, &concept_menu_water,
                                          &concept_menu_bean};
    // Figma node 5:1111: 166 px grid, 64.75 px columns, 36 px gap;
    // rows are 71.5 px with a 22 px gap and start at y=157.5.
    static const int x[] = {-50, 50, -50, 50};
    static const int y[] = {-47, -47, 47, 47};
    for (int i = 0; i < 4; i++) {
        menuButtons[i] = lv_btn_create(root);
        lv_obj_set_size(menuButtons[i], 65, 72);
        lv_obj_align(menuButtons[i], LV_ALIGN_CENTER, x[i], y[i]);
        lv_obj_set_style_bg_opa(menuButtons[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(menuButtons[i], 0, 0);
        lv_obj_set_style_shadow_width(menuButtons[i], 0, 0);
        lv_obj_set_style_pad_all(menuButtons[i], 0, 0);
        lv_obj_add_event_cb(menuButtons[i], eventCallback, LV_EVENT_CLICKED, this);
        lv_obj_t *icon = lv_img_create(menuButtons[i]);
        lv_img_set_src(icon, icons[i]);
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 8);
        lv_obj_t *label = makeLabel(menuButtons[i], &dm_sans_9, lv_color_hex(0xa6a6a6));
        lv_obj_set_style_text_letter_space(label, 1, 0);
        lv_label_set_text(label, labels[i]);
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 50);
    }
}

void ConceptUI::show(ScreensEnum screenId) {
    const View previousView = view;
    editing = false;
    switch (screenId) {
    case SCREEN_ID_STANDBY_SCREEN: view = View::Standby; break;
    case SCREEN_ID_MENU_SCREEN:
    case SCREEN_ID_MENU_SCREEN_NEW: view = View::Menu; break;
    case SCREEN_ID_BREW_SCREEN:
    case SCREEN_ID_PROFILE_SCREEN:
    case SCREEN_ID_NEW_PROFILE_SCREEN: view = View::Brew; break;
    case SCREEN_ID_STATUS_SCREEN: view = View::Status; break;
    case SCREEN_ID_STEAM_SCREEN: view = View::Steam; break;
    case SCREEN_ID_WATER_SCREEN: view = View::Water; break;
    case SCREEN_ID_GRIND_SCREEN: view = View::Grind; break;
    case SCREEN_ID_INFO_SCREEN: view = View::Info; break;
    default: view = View::Brew; break;
    }
    if (view != previousView) {
        mainLayoutDirty = true;
        renderedPrimaryState = 0xff;
    }
    if (view == View::Standby && previousView != View::Standby) {
        standbyEnteredAt = lv_tick_get();
        faceVisible = false;
        standbyHearts = false;
    }
    if (view == View::Status && previousView != View::Status) {
        lv_chart_set_all_value(chart, pressureSeries, LV_CHART_POINT_NONE);
        lastChartSampleAt = 0;
        previewChartKind = -1;
        pressureHistoryCount = 0;
        completeChartRendered = false;
    }
    applyView();
    if (view == View::Standby) updateStandbyFace();
    if (lv_scr_act() != root) {
        // EEZ still updates its generated screen internally for compatibility,
        // but the concept layer must replace it in the same frame. A fade here
        // exposes the generated UI underneath and causes an unwanted flash.
        lv_scr_load(root);
    }
}

void ConceptUI::setMode(View next) {
    controller->deactivate();
    switch (next) {
    case View::Brew: controller->setMode(MODE_BREW); owner->changeScreen(SCREEN_ID_BREW_SCREEN); break;
    case View::Steam: controller->setMode(MODE_STEAM); owner->changeScreen(SCREEN_ID_STEAM_SCREEN); break;
    case View::Water: controller->setMode(MODE_WATER); owner->changeScreen(SCREEN_ID_WATER_SCREEN); break;
    case View::Grind: controller->setMode(MODE_GRIND); owner->changeScreen(SCREEN_ID_GRIND_SCREEN); break;
    default: break;
    }
}

void ConceptUI::applyView() {
    const bool standby = view == View::Standby;
    const bool menu = view == View::Menu;
    const bool status = view == View::Status;
    for (auto *obj : {brand, brandMate, standbyError, standbyClock, standbyProfileRow, standbyHint}) {
        if (standby) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    if (standby) lv_obj_clear_flag(standbyFace, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(standbyFace, LV_OBJ_FLAG_HIDDEN);
    for (auto *obj : {standbyEyes[0], standbyEyes[1], standbyHeartEyes[0], standbyHeartEyes[1], standbyMouth,
                      standbySmile, standbyBlush[0], standbyBlush[1], standbyZ[0], standbyZ[1], standbyZ[2],
                      standbyFloatingHearts[0], standbyFloatingHearts[1], standbyFloatingHearts[2]}) {
        if (standby) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    if (standby || menu) lv_obj_add_flag(stateBackground, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(stateBackground, LV_OBJ_FLAG_HIDDEN);
    for (auto *button : menuButtons) {
        if (menu) lv_obj_clear_flag(button, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(button, LV_OBJ_FLAG_HIDDEN);
    }
    if (menu) lv_obj_clear_flag(standbyButton, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(standbyButton, LV_OBJ_FLAG_HIDDEN);
    if (menu) lv_obj_clear_flag(menuDownHint, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(menuDownHint, LV_OBJ_FLAG_HIDDEN);
    for (auto *obj : {profileRow, stateRow, contextIcon, mainValue, mainDecimal, mainUnit, targetRow, primaryButton}) {
        if (standby || menu) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    for (auto *dot : profileDots) {
        if (view == View::Brew && !editing) lv_obj_clear_flag(dot, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
    }
    if (view == View::Brew && !editing) lv_obj_clear_flag(swipeUpHint, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(swipeUpHint, LV_OBJ_FLAG_HIDDEN);
    if (view == View::Grind) lv_obj_clear_flag(grindTargetRow, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(grindTargetRow, LV_OBJ_FLAG_HIDDEN);
    for (auto *tick : ticks) {
        if (standby) lv_obj_add_flag(tick, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(tick, LV_OBJ_FLAG_HIDDEN);
    }
    if (status) {
        lv_obj_clear_flag(phaseLabel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(profileRow, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(stateRow, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(targetRow, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(chart, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(chartEndpoint, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(contextIcon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(statusTimerIcon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(statusMetrics, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(phaseLabel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(chart, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(chartEndpoint, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(statusTimerIcon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(statusMetrics, LV_OBJ_FLAG_HIDDEN);
    }
    setEditing(false);
    if (menu) lv_obj_clear_flag(connection, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(connection, LV_OBJ_FLAG_HIDDEN);
    if (standby) lv_obj_clear_flag(connectionIcons, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(connectionIcons, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(connection, LV_ALIGN_TOP_MID, 0, 44);
}

void ConceptUI::update(const ConceptUIState &inputState) {
    // EEZ's generated standby flow can still advance its own screen after the
    // initial change. Keep the concept root authoritative so that transition
    // can never expose the generated black/original screen underneath it.
    if (root != nullptr && lv_scr_act() != root) lv_scr_load(root);
    ConceptUIState state = inputState;
    const bool controllerConnected = controller->getClientController()->isConnected();
    if (controllerConnected) standalonePreview = false;
    if (standalonePreview && !controllerConnected) {
        const float elapsed = lv_tick_elaps(standalonePreviewStartedAt) / 1000.0f;
        const float frozenElapsed = LV_MIN(elapsed, 13.0f);
        state.temperature = 93.0f;
        state.targetTemperature = 93.0f;
        state.temperatureStable = true;
        state.error = false;
        state.elapsedSeconds = frozenElapsed;
        state.elapsedPercentage = 1.0f;
        state.preview = true;
        if (elapsed < 3.0f) {
            state.phase = "INFUSION";
            state.pressure = 2.5f * elapsed / 3.0f;
            state.weight = 0.0f;
        } else if (elapsed < 13.0f) {
            state.phase = "BREW";
            const float brewElapsed = elapsed - 3.0f;
            if (brewElapsed < 2.0f) state.pressure = 2.5f + brewElapsed * 3.25f;
            else if (brewElapsed < 8.0f) state.pressure = 9.0f;
            else state.pressure = 9.0f * (1.0f - (brewElapsed - 8.0f) / 2.0f);
            state.weight = brewElapsed * 2.5f;
        } else {
            state.phase = "COMPLETE";
            state.pressure = 0.0f;
            state.weight = 25.0f;
            state.processComplete = true;
        }
    }
    lastState = state;
    const float tempRatio = state.targetTemperature > 0 ? state.temperature / state.targetTemperature : 0.0f;
    // Simple-pump profiles have no controller pressure target even though a
    // pressure sensor still reports live bar values. Use the dial's 10 bar
    // design scale in that case so the right-hand LED ring remains functional.
    const float pressureScale = state.targetPressure > 0.1f ? state.targetPressure : 10.0f;
    const float pressureRatio = state.pressure / pressureScale;
    updateRing(tempRatio, pressureRatio);
    int gradient = 0;
    if (view == View::Status) {
        lv_obj_clear_flag(mainDecimal, LV_OBJ_FLAG_HIDDEN);
        if (state.processComplete) gradient = 4;
        else {
            char phaseUpper[32]; uppercaseCopy(phaseUpper, sizeof(phaseUpper), state.phase);
            gradient = std::strstr(phaseUpper, "INFUS") != nullptr ? 2 : 3;
        }
    } else if (view == View::Brew) gradient = state.temperatureStable ? 1 : 0;
    else if (view == View::Steam) gradient = 5;
    else if (view == View::Water) gradient = 6;
    else if (view == View::Grind) gradient = 7;
    float gradientFill = 1.0f;
    if (view == View::Brew && !state.temperatureStable) {
        const float temperatureRatio = state.targetTemperature > 0.0f ? state.temperature / state.targetTemperature : 0.0f;
        gradientFill = LV_CLAMP(0.12f, temperatureRatio, 1.0f);
    }
    applyStateGradient(gradient, gradientFill);
    if (view != View::Standby && view != View::Menu) lv_obj_clear_flag(stateBackground, LV_OBJ_FLAG_HIDDEN);
    updateHeatingGradient();
    setLabelTextFmtIfChanged(connection, "%s  %s", state.wifi ? "WiFi" : "--", state.connected ? "BT" : "--");

    if (view == View::Standby) {
        std::time_t now = std::time(nullptr);
        std::tm *local = std::localtime(&now);
        if (local != nullptr) setLabelTextFmtIfChanged(standbyClock, "%02d:%02d", local->tm_hour, local->tm_min);
        setLabelTextIfChanged(standbyProfile, safeText(state.profile, "ESPRESSO"));
        setLabelTextFmtIfChanged(standbyProfileTemp, "%.0f°C", state.targetTemperature);
        setLabelTextIfChanged(brand, "GAGGI");
        lv_obj_set_style_text_font(brand, &dm_sans_30_bold, 0);
        lv_obj_set_style_text_color(brand, WHITE, 0);
        setLabelTextIfChanged(standbyError, state.error ? safeText(state.errorLabel, "SYSTEM ERROR") : "");
        if (state.error) lv_obj_clear_flag(standbyError, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(standbyError, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    if (view == View::Menu) {
        setLabelTextFmtIfChanged(connection, "%.0f C     %.1f bar", state.temperature, state.pressure);
        return;
    }

    const char *mode = "BREW";
    if (view == View::Steam) mode = "STEAM";
    else if (view == View::Water) mode = "FLUSH";
    else if (view == View::Grind) mode = "GRINDER";
    else if (view == View::Info) mode = "SYSTEM";
    char profileUpper[48];
    uppercaseCopy(profileUpper, sizeof(profileUpper), view == View::Brew ? state.profile : mode);
    setLabelTextIfChanged(modeLabel, profileUpper);
    lv_obj_set_style_img_opa(profileLeft, visualProfileIndex > 0 ? 46 : LV_OPA_TRANSP, 0);
    lv_obj_set_style_img_opa(profileRight, visualProfileIndex < 4 ? 46 : LV_OPA_TRANSP, 0);
    lv_obj_set_style_img_opa(wifiIcon, state.wifi ? 97 : 25, 0);
    lv_obj_set_style_img_opa(bluetoothIcon, state.connected ? 97 : 25, 0);
    setLabelTextFmtIfChanged(editTempValue, "%.1f C", state.targetTemperature);
    setLabelTextIfChanged(editTargetValue, safeText(state.brewTarget, state.volumetric ? "0 g" : "0:00"));

    char main[32];
    char secondary[80];
    bool mainGeometryChanged = false;
    if (view == View::Status) {
        lv_obj_add_flag(targetRow, LV_OBJ_FLAG_HIDDEN);
        char phaseUpper[32];
        uppercaseCopy(phaseUpper, sizeof(phaseUpper), safeText(state.phase, "BREW"));
        setLabelTextIfChanged(phaseLabel, state.processComplete ? "COMPLETE" : phaseUpper);
        const uint32_t statusAccent = state.processComplete ? 0x28c870 :
            (std::strstr(phaseUpper, "INFUS") != nullptr ? 0xd07020 : 0xd82828);
        lv_obj_set_style_text_color(phaseLabel, lv_color_hex(statusAccent), 0);
        lv_obj_set_style_img_recolor(statusTimerIcon, lv_color_hex(statusAccent), 0);
        lv_obj_set_style_img_recolor_opa(statusTimerIcon, LV_OPA_COVER, 0);
        lv_obj_set_style_img_recolor(statusPressureIcon, lv_color_hex(statusAccent), 0);
        lv_obj_set_style_img_recolor_opa(statusPressureIcon, LV_OPA_COVER, 0);
        const int elapsedWhole = static_cast<int>(state.elapsedSeconds);
        const int elapsedTenth = static_cast<int>(state.elapsedSeconds * 10.0f) % 10;
        lv_obj_set_style_text_font(mainValue, &dm_sans_88_bold, 0);
        mainGeometryChanged |= setLabelTextFmtIfChanged(mainValue, "%d", elapsedWhole);
        mainGeometryChanged |= setLabelTextFmtIfChanged(mainDecimal, ".%d", elapsedTenth);
        mainGeometryChanged |= setLabelTextIfChanged(mainUnit, "s");
        lv_obj_set_style_text_color(mainValue, state.elapsedPercentage > 0.0f ? WHITE : lv_color_hex(0x282828), 0);
        snprintf(secondary, sizeof(secondary), "%.1f g  -  %.1f bar", state.weight, state.pressure);
        setLabelTextIfChanged(secondaryValue, secondary);
        setLabelTextFmtIfChanged(statusWeight, "%.1f", state.weight);
        setLabelTextFmtIfChanged(statusPressure, "%.1f", state.pressure);
        lv_obj_set_style_text_color(statusWeight, lv_color_hex(state.weight > 0 ? 0xadadad : 0x292929), 0);
        lv_obj_set_style_text_color(statusPressure, lv_color_hex(state.pressure > 0 ? statusAccent : 0x292929), 0);
        lv_chart_set_series_color(chart, pressureSeries, lv_color_hex(statusAccent));
        lv_obj_set_style_line_color(chart, lv_color_hex(statusAccent), LV_PART_MAIN);
        lv_obj_set_style_bg_color(chartEndpoint, lv_color_hex(statusAccent), 0);
        const float endpointPressure = LV_CLAMP(0.0f, state.pressure, 10.0f);
        const int endpointY = 286 + static_cast<int>((10.0f - endpointPressure) * 8.0f);
        lv_obj_set_pos(chartEndpoint, 312, endpointY - 2);
        const uint32_t now = lv_tick_get();
#ifdef GAGGIMATE_SIM
        if (state.preview) {
            const int chartKind = state.processComplete ? 2 :
                (std::strstr(phaseUpper, "INFUS") != nullptr ? 0 : 1);
            if (chartKind != previewChartKind) {
                previewChartKind = chartKind;
                lv_coord_t *points = lv_chart_get_y_array(chart, pressureSeries);
                for (int i = 0; i < 60; ++i) {
                    if (chartKind == 0) {
                        // Figma pre-infusion: quick pressure ramp followed by a 2.5 bar plateau.
                        points[i] = i < 18 ? static_cast<lv_coord_t>((i * 25) / 17) : 25;
                    } else if (chartKind == 1) {
                        // Figma brew: the settled nine-bar trace carries across the plot.
                        points[i] = static_cast<lv_coord_t>(87 + LV_MIN(i, 12) / 4);
                    } else {
                        // Complete shows the entire shot in the same 148x80
                        // viewport: pre-infusion, ramp, brew plateau and tail.
                        if (i < 9) points[i] = static_cast<lv_coord_t>(i * 25 / 8);
                        else if (i < 17) points[i] = 25;
                        else if (i < 25) points[i] = static_cast<lv_coord_t>(25 + (i - 16) * 65 / 8);
                        else if (i < 53) points[i] = static_cast<lv_coord_t>(90 - (i % 6 == 0 ? 2 : 0));
                        else points[i] = static_cast<lv_coord_t>(90 - (i - 52) * 13);
                    }
                }
                // Live sampling may already have advanced the circular chart
                // before preview state arrives. Reset its origin so the Figma
                // trace ends cleanly instead of wrapping into a vertical tail.
                lv_chart_refresh(chart);
            }
            // The regular simulator update runs immediately before this
            // preview and can advance LVGL's circular origin once. Pin it on
            // every preview frame so Infusion, Brew and Complete never wrap.
            lv_chart_set_x_start_point(chart, pressureSeries, 0);
        } else
#endif
        if (state.processComplete) {
            if (!completeChartRendered) {
                renderCompleteChart();
                completeChartRendered = true;
            }
        } else if (lastChartSampleAt == 0 || lv_tick_elaps(lastChartSampleAt) >= 160) {
            lastChartSampleAt = now;
            const lv_coord_t sample = static_cast<lv_coord_t>(LV_CLAMP(0.0f, state.pressure * 10.0f, 100.0f));
            if (pressureHistoryCount >= PRESSURE_HISTORY_CAPACITY) {
                // Preserve long shots without allocating: halve the history by
                // averaging adjacent samples, then continue recording.
                for (int i = 0; i < PRESSURE_HISTORY_CAPACITY / 2; ++i)
                    pressureHistory[i] = static_cast<lv_coord_t>((pressureHistory[i * 2] + pressureHistory[i * 2 + 1]) / 2);
                pressureHistoryCount = PRESSURE_HISTORY_CAPACITY / 2;
            }
            pressureHistory[pressureHistoryCount++] = sample;
            lv_chart_set_next_value(chart, pressureSeries, sample);
        }
    } else if (view == View::Grind) {
        lv_obj_add_flag(targetRow, LV_OBJ_FLAG_HIDDEN);
        setImageSourceIfChanged(contextIcon, &concept_grind);
        lv_obj_set_style_img_recolor(contextIcon, lv_color_hex(0xc89020), 0);
        lv_obj_set_style_text_font(mainValue, &dm_sans_88_bold, 0);
        const bool grindActive = controller->isGrindActive();
        const int grindTargetSeconds = controller->getTargetGrindDuration() / 1000;
        snprintf(main, sizeof(main), "%d", grindTargetSeconds);
        mainGeometryChanged |= setLabelTextIfChanged(mainValue, main);
        lv_obj_add_flag(mainDecimal, LV_OBJ_FLAG_HIDDEN);
        mainGeometryChanged |= setLabelTextIfChanged(mainUnit, "s");
        setLabelTextFmtIfChanged(grindTargetValue, "%ds", grindTargetSeconds);
        setLabelTextIfChanged(stateLabel, grindActive ? "GRINDING" : "");
        lv_obj_set_style_text_color(stateLabel, lv_color_hex(0xc89020), 0);
        if (grindActive) lv_obj_clear_flag(stateSpinner, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(stateSpinner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_img_recolor(stateSpinner, lv_color_hex(0xc89020), 0);
    } else {
        lv_obj_clear_flag(mainDecimal, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(targetRow, LV_OBJ_FLAG_HIDDEN);
        const uint32_t accent = view == View::Brew ? (state.temperatureStable ? 0x2ab8d8 : 0x4a8cf0) : 0x4a8cf0;
        if (view == View::Brew) setImageSourceIfChanged(contextIcon, state.temperatureStable ? &concept_drop : &concept_heat);
        else if (view == View::Steam) setImageSourceIfChanged(contextIcon, &concept_steam);
        else setImageSourceIfChanged(contextIcon, &concept_water);
        lv_obj_set_style_img_recolor(contextIcon, lv_color_hex(accent), 0);
        const float shownTemperature = state.temperature > 0.0f ? state.temperature : 22.0f;
        lv_obj_set_style_text_font(mainValue, &dm_sans_88_bold, 0);
        snprintf(main, sizeof(main), "%.0f", std::floor(shownTemperature));
        snprintf(secondary, sizeof(secondary), "%.0f°C", state.targetTemperature);
        mainGeometryChanged |= setLabelTextIfChanged(mainValue, main);
        mainGeometryChanged |= setLabelTextFmtIfChanged(mainDecimal, ".%d", static_cast<int>(std::round(shownTemperature * 10.0f)) % 10);
        mainGeometryChanged |= setLabelTextIfChanged(mainUnit, "°C");
        lv_obj_set_style_text_color(mainValue, WHITE, 0);
        setLabelTextIfChanged(secondaryValue, secondary);
        lv_obj_set_style_img_recolor(targetIcon, lv_color_hex(accent), 0);
        // The overview is a temperature target screen. Sensor offset/noise can
        // report pressure while idle, but pressure belongs to infusion/brew and
        // must not make an extra icon, separator and bar value appear here.
        const bool showPressure = false;
        setLabelTextFmtIfChanged(targetPressure, "%.1f bar", state.pressure);
        for (auto *obj : {targetDot, targetPressureIcon, targetPressure}) {
            if (showPressure) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_set_style_img_recolor(targetPressureIcon, lv_color_hex(accent), 0);
        if (view == View::Brew) {
            setLabelTextIfChanged(stateLabel, state.temperatureStable ? "READY" : "HEATING");
            lv_obj_set_style_text_color(stateLabel, lv_color_hex(state.temperatureStable ? 0x2ab8d8 : 0x4a8cf0), 0);
            if (state.temperatureStable) lv_obj_add_flag(stateSpinner, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(stateSpinner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_img_recolor(stateSpinner, lv_color_hex(0x4a8cf0), 0);
        } else {
            setLabelTextIfChanged(stateLabel, "");
            lv_obj_add_flag(stateSpinner, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (mainLayoutDirty || mainGeometryChanged) {
        lv_obj_update_layout(root);
        if (view == View::Status) lv_obj_align(mainValue, LV_ALIGN_TOP_MID, -18, 125);
        else lv_obj_align(mainValue, LV_ALIGN_TOP_MID, -18, 190);
        lv_obj_align_to(mainDecimal, mainValue, LV_ALIGN_OUT_RIGHT_TOP, -2, 32);
        lv_obj_align_to(mainUnit, mainDecimal, LV_ALIGN_OUT_TOP_LEFT, 0, -3);
        if (view == View::Status) {
            lv_obj_align(primaryButton, LV_ALIGN_TOP_MID, 0, 395);
        } else {
            lv_obj_align(contextIcon, LV_ALIGN_TOP_MID, 0, 120);
            lv_obj_align(targetRow, LV_ALIGN_TOP_MID, 0, 300);
            lv_obj_align(grindTargetRow, LV_ALIGN_TOP_MID, 0, 296);
            lv_obj_align(primaryButton, LV_ALIGN_TOP_MID, 0, 344);
        }
        mainLayoutDirty = false;
    }
    updateProfileDots();
    updatePrimaryButton();
}

void ConceptUI::updateProfileDots() {
    if (renderedProfileIndex == visualProfileIndex) return;
    renderedProfileIndex = visualProfileIndex;
    int totalWidth = 0;
    for (int i = 0; i < 5; i++) totalWidth += i == visualProfileIndex ? 12 : 4;
    totalWidth += 4 * 5;
    int cursor = -totalWidth / 2;
    for (int i = 0; i < 5; i++) {
        const bool active = i == visualProfileIndex;
        const int width = active ? 12 : 4;
        lv_obj_set_width(profileDots[i], width);
        lv_obj_set_style_bg_opa(profileDots[i], active ? LV_OPA_50 : LV_OPA_10, 0);
        lv_obj_align(profileDots[i], LV_ALIGN_BOTTOM_MID, cursor + width / 2, -36);
        cursor += width + 5;
    }
}

void ConceptUI::updateTouchGesture() {
    lv_indev_t *indev = lv_indev_get_act();
    if (indev == nullptr || view != View::Brew) return;
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    const bool pressed = indev->proc.state == LV_INDEV_STATE_PRESSED;
    if (pressed && !touchTracking) {
        touchTracking = true;
        touchStart = point;
    } else if (!pressed && touchTracking) {
        touchTracking = false;
        const int dx = point.x - touchStart.x;
        const int dy = point.y - touchStart.y;
        if (std::abs(dx) >= 38 && std::abs(dx) > std::abs(dy)) {
            if (dx < 0) {
                visualProfileIndex = LV_MIN(4, visualProfileIndex + 1);
                owner->onNextProfile();
            } else {
                visualProfileIndex = LV_MAX(0, visualProfileIndex - 1);
                owner->onPreviousProfile();
            }
            owner->onProfileSelect();
        }
    }
}

void ConceptUI::updateStandbyFace() {
    if (view != View::Standby) return;
    const uint32_t elapsed = lv_tick_elaps(standbyEnteredAt);
    // Standby starts as the restrained logo screen. Only after 30 seconds does
    // it slowly cross-fade into the character, never through an empty frame.
    constexpr uint32_t LOGO_HOLD_MS = 30000;
    constexpr float CHARACTER_FADE_MS = 5000.0f;
    const float reveal = clamp01((static_cast<float>(elapsed) - LOGO_HOLD_MS) / CHARACTER_FADE_MS);
    faceVisible = elapsed >= LOGO_HOLD_MS;
    const uint8_t brandOpa = opacity(1.0f - reveal);
    const uint8_t faceOpa = opacity(reveal);
    for (auto *obj : {brand, brandMate, standbyClock, standbyProfileRow, connectionIcons})
        lv_obj_set_style_opa(obj, brandOpa, 0);
    lv_obj_set_style_opa(standbyFace, faceOpa, 0);
    lv_obj_set_style_img_opa(standbyHint, 46, 0);

    if (!faceVisible) {
        for (int i = 0; i < 2; i++) {
            lv_obj_set_style_img_opa(standbyEyes[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_img_opa(standbyHeartEyes[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_bg_opa(standbyBlush[i], LV_OPA_TRANSP, 0);
        }
        lv_obj_set_style_img_opa(standbyMouth, LV_OPA_TRANSP, 0);
        lv_obj_set_style_img_opa(standbySmile, LV_OPA_TRANSP, 0);
        for (int i = 0; i < 3; i++) {
            lv_obj_set_style_text_opa(standbyZ[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_img_opa(standbyFloatingHearts[i], LV_OPA_TRANSP, 0);
        }
        return;
    }
    const uint32_t faceElapsed = elapsed - LOGO_HOLD_MS;
    constexpr uint32_t CHARACTER_STATE_MS = 24000;
    const uint32_t stateIndex = faceElapsed / CHARACTER_STATE_MS;
    standbyHearts = (stateIndex & 1U) != 0;
    const float stateFade = clamp01((faceElapsed % CHARACTER_STATE_MS) / 4000.0f);
    const float heartMix = stateIndex == 0 ? 0.0f : (standbyHearts ? stateFade : 1.0f - stateFade);
    const float sleepMix = 1.0f - heartMix;

    const float breathePhase = (faceElapsed % 11000) / 11000.0f;
    const float breathe = 1.0f + 0.015f * triangle(breathePhase);
    auto breatheOffset = [breathe](lv_obj_t *object, float centerX, float centerY) {
        lv_obj_set_style_translate_x(object, static_cast<int>((centerX - 240.0f) * (breathe - 1.0f)), 0);
        lv_obj_set_style_translate_y(object, static_cast<int>((centerY - 240.0f) * (breathe - 1.0f)), 0);
    };
    for (int i = 0; i < 2; i++) {
        const float eyeCenterX = i == 0 ? 203.0f : 277.0f;
        breatheOffset(standbyEyes[i], eyeCenterX, 229.0f);
        breatheOffset(standbyHeartEyes[i], eyeCenterX, 229.0f);
        breatheOffset(standbyBlush[i], i == 0 ? 177.0f : 303.0f, 248.0f);

        const float blinkPhase = ((faceElapsed + (i == 0 ? 0 : 700)) % 12000) / 12000.0f;
        float blinkOpacity = 1.0f;
        if (blinkPhase >= .90f && blinkPhase < .94f) blinkOpacity = 1.0f - (blinkPhase - .90f) / .04f * .86f;
        else if (blinkPhase >= .94f && blinkPhase < .97f) blinkOpacity = .14f + (blinkPhase - .94f) / .03f * .86f;
        lv_obj_set_style_img_opa(standbyEyes[i], opacity(reveal * sleepMix * blinkOpacity), 0);

        const float beatPhase = ((faceElapsed + i * 350) % 4400) / 4400.0f;
        float beat = 1.0f;
        if (beatPhase < .15f) beat = 1.0f + beatPhase / .15f * .22f;
        else if (beatPhase < .25f) beat = 1.22f - (beatPhase - .15f) / .10f * .18f;
        else if (beatPhase < .35f) beat = 1.04f + (beatPhase - .25f) / .10f * .14f;
        else if (beatPhase < .48f) beat = 1.18f - (beatPhase - .35f) / .13f * .18f;
        const float beatOpacity = .82f + (beat - 1.0f) / .22f * .18f;
        lv_obj_set_style_img_opa(standbyHeartEyes[i], opacity(reveal * heartMix * beatOpacity), 0);
        lv_obj_set_style_bg_opa(standbyBlush[i], opacity(reveal * heartMix *
            (0.30f + 0.25f * triangle(((faceElapsed + i * 600) % 3500) / 3500.0f))), 0);
    }
    lv_obj_set_style_img_opa(standbyMouth, opacity(reveal * sleepMix), 0);
    lv_obj_set_style_img_opa(standbySmile, opacity(reveal * heartMix), 0);
    breatheOffset(standbyMouth, 240.0f, 264.5f);
    breatheOffset(standbySmile, 240.0f, 267.0f);

    const uint32_t zDelay[] = {0, 2600, 5200};
    for (int i = 0; i < 3; i++) {
        const float p = ((faceElapsed + 7600 - zDelay[i]) % 7600) / 7600.0f;
        const float alpha = p < .15f ? p / .15f : (p < .85f ? 1.0f - .2f * ((p - .15f) / .70f) : (1.0f - p) / .15f * .8f);
        lv_obj_set_style_text_opa(standbyZ[i], opacity(reveal * sleepMix * alpha * .75f), 0);
        const float zCenterX[] = {264.0f, 280.5f, 296.0f};
        const float zCenterY[] = {212.0f, 198.0f, 187.0f};
        lv_obj_set_style_translate_x(standbyZ[i], static_cast<int>((zCenterX[i] - 240.0f) * (breathe - 1.0f)), 0);
        lv_obj_set_style_translate_y(standbyZ[i], static_cast<int>((zCenterY[i] - 240.0f) * (breathe - 1.0f) - 52 * p), 0);
    }
    const uint32_t heartDelay[] = {0, 2400, 4800};
    for (int i = 0; i < 3; i++) {
        const float p = ((faceElapsed + 7200 - heartDelay[i]) % 7200) / 7200.0f;
        const float alpha = p < .18f ? p / .18f * .85f : (1.0f - p) / .82f * .85f;
        lv_obj_set_style_img_opa(standbyFloatingHearts[i], opacity(reveal * heartMix * alpha), 0);
        const float heartCenterX[] = {195.0f, 245.0f, 290.0f};
        lv_obj_set_style_translate_x(standbyFloatingHearts[i], static_cast<int>((heartCenterX[i] - 240.0f) * (breathe - 1.0f)), 0);
        lv_obj_set_style_translate_y(standbyFloatingHearts[i], static_cast<int>((200.0f - 240.0f) * (breathe - 1.0f) - 60 * p), 0);
    }
}

void ConceptUI::renderCompleteChart() {
    if (pressureHistoryCount == 0) return;
    lv_coord_t *points = lv_chart_get_y_array(chart, pressureSeries);
    if (pressureHistoryCount == 1) {
        for (int i = 0; i < 60; ++i) points[i] = pressureHistory[0];
    } else {
        for (int i = 0; i < 60; ++i) {
            const float position = i * (pressureHistoryCount - 1) / 59.0f;
            const int left = static_cast<int>(position);
            const int right = LV_MIN(left + 1, static_cast<int>(pressureHistoryCount - 1));
            const float mix = position - left;
            points[i] = static_cast<lv_coord_t>(pressureHistory[left] * (1.0f - mix) + pressureHistory[right] * mix);
        }
    }
    // Live sampling advances LVGL's circular series origin. The completed
    // history above is chronological, so point 0 must be drawn at the left.
    lv_chart_set_x_start_point(chart, pressureSeries, 0);
    lv_chart_refresh(chart);
}

void ConceptUI::updateRing(float temperatureRatio, float pressureRatio) {
    temperatureRatio = LV_CLAMP(0.0f, temperatureRatio, 1.0f);
    pressureRatio = LV_CLAMP(0.0f, pressureRatio, 1.0f);
    const int temperatureTicks = static_cast<int>(temperatureRatio * 39.0f) + 1;
    const int pressureTicks = static_cast<int>(pressureRatio * 39.0f) + 1;
    if (temperatureTicks == renderedTemperatureTicks && pressureTicks == renderedPressureTicks) return;
    renderedTemperatureTicks = temperatureTicks;
    renderedPressureTicks = pressureTicks;
    for (int i = 0; i < 80; i++) {
        const bool left = i >= 40;
        const float progress = left ? (i - 40) / 39.0f : (39 - i) / 39.0f;
        lv_color_t color = WHITE;
        lv_opa_t lineOpacity = 20; // Figma inactive ticks: white at 8%.
        if (left && progress <= temperatureRatio) {
            color = lv_color_hex(0xd22626);
            // The physical/concept ring is intentionally weakest at six
            // o'clock and becomes solid toward the upper half.
            lineOpacity = static_cast<lv_opa_t>(92 + progress * 146.0f);
        }
        if (!left && progress <= pressureRatio) {
            color = lv_color_hex(0x2d69d7);
            lineOpacity = static_cast<lv_opa_t>(118 + progress * 112.0f);
        }
        lv_obj_set_style_line_color(ticks[i], color, 0);
        lv_obj_set_style_line_opa(ticks[i], lineOpacity, 0);
    }
}

void ConceptUI::applyStateGradient(int gradient, float fill) {
    const uint8_t fillStep = static_cast<uint8_t>(LV_CLAMP(0, static_cast<int>(std::lround(fill * 100.0f)), 100));
    if (gradient == renderedGradient && fillStep == renderedGradientFill) return;
    renderedGradient = gradient;
    renderedGradientFill = fillStep;
    const uint8_t nextBuffer = activeBackgroundBuffer ^ 1U;
    // Figma glow colors after compositing each status' opacity over #050505.
    static constexpr uint8_t targets[8][3] = {
        {9, 66, 86}, {9, 66, 86}, {86, 35, 2}, {101, 7, 7},
        {10, 69, 31}, {10, 46, 102}, {10, 49, 98}, {82, 57, 7},
    };
    // Keep the proven ordered pattern for brew. The other hues use a
    // decorrelated threshold below: repeating Bayer rows were visible as a
    // yellow/green/cyan band on the physical RGB565 panel.
    static constexpr uint8_t threshold[8][8] = {
        {0, 32, 8, 40, 2, 34, 10, 42}, {48, 16, 56, 24, 50, 18, 58, 26},
        {12, 44, 4, 36, 14, 46, 6, 38}, {60, 28, 52, 20, 62, 30, 54, 22},
        {3, 35, 11, 43, 1, 33, 9, 41}, {51, 19, 59, 27, 49, 17, 57, 25},
        {15, 47, 7, 39, 13, 45, 5, 37}, {63, 31, 55, 23, 61, 29, 53, 21},
    };
    // The glow fades into actual panel black. Quantising or dithering the
    // background itself creates a visible light contour before the glow.
    constexpr float base[3] = {0.0f, 0.0f, 0.0f};
    for (int y = 0; y < 240; y++) {
        const float screenY = static_cast<float>(y + 240);
        const float vertical = (480.0f - screenY) / 170.0f;
        const float revealTop = 480.0f - 240.0f * fillStep / 100.0f;
        for (int x = 0; x < SCREEN_SIZE; x++) {
            const float horizontal = (static_cast<float>(x) - 239.5f) / 312.0f;
            const float distance = std::sqrt(horizontal * horizontal + vertical * vertical);
            const float radialGlow = LV_MAX(0.0f, 1.0f - distance);
            // During heating the glow rises with temperature, but its leading
            // edge fades across 42 px instead of exposing a hard horizontal cut.
            float reveal = 1.0f;
            if (fillStep < 100) {
                reveal = LV_CLAMP(0.0f, (screenY - (revealTop - 42.0f)) / 42.0f, 1.0f);
                reveal = reveal * reveal * (3.0f - 2.0f * reveal);
            }
            const float glow = radialGlow * reveal;
            const float red = base[0] + (targets[gradient][0] - base[0]) * glow;
            const float green = base[1] + (targets[gradient][1] - base[1]) * glow;
            const float blue = base[2] + (targets[gradient][2] - base[2]) * glow;
            if (glow <= 0.012f) {
                stateBackgroundPixels[nextBuffer][y * SCREEN_SIZE + x] = lv_color_black();
                continue;
            }
            const float redDither =
                (static_cast<float>(threshold[(y + x / 8) & 7][x & 7]) - 31.5f) / 64.0f;
            // Use the same continuous ordered-dither path for every status.
            // The previous non-red RGB565 quantiser introduced a visibly
            // different horizontal colour zone on the physical blue panel.
            // This is the renderer already validated for the red brew glow.
            const float strength = LV_MIN(1.0f, glow * 3.0f);
            const float noise = redDither * strength;
            stateBackgroundPixels[nextBuffer][y * SCREEN_SIZE + x] = lv_color_make(
                LV_CLAMP(0, static_cast<int>(red + noise * 16.0f), 255),
                LV_CLAMP(0, static_cast<int>(green + noise * 8.0f), 255),
                LV_CLAMP(0, static_cast<int>(blue + noise * 16.0f), 255));
        }
    }
    // Publish only after every pixel is ready. Drawing and generation never
    // touch the same PSRAM buffer, preventing a half-old/half-new flash.
    lv_img_cache_invalidate_src(&stateBackgroundImages[nextBuffer]);
    lv_img_set_src(stateBackground, &stateBackgroundImages[nextBuffer]);
    activeBackgroundBuffer = nextBuffer;
}

void ConceptUI::updateHeatingGradient() {
    // Keep the native gradient fully opaque. Blending a pre-quantized layer
    // creates a second RGB565 quantisation pass and reintroduces banding.
    lv_obj_set_style_opa(stateBackground, LV_OPA_COVER, 0);
}

void ConceptUI::updatePrimaryButton() {
    const bool statusRunning = view == View::Status && !lastState.processComplete;
    bool active = statusRunning || controller->isActive() || controller->isGrindActive();
    const uint8_t visualState = static_cast<uint8_t>((view == View::Status ? 8 : 0) |
        (lastState.processComplete ? 4 : 0) | (active ? 2 : 0) | (lastState.temperatureStable ? 1 : 0));
    if (visualState == renderedPrimaryState) return;
    renderedPrimaryState = visualState;
    const uint32_t accent = lastState.processComplete ? 0x28c870 :
        (active ? 0xd82828 : (lastState.temperatureStable ? 0xffffff : 0x777777));
    if (view == View::Status && lastState.processComplete) setImageSourceIfChanged(primaryCanvas, &concept_up_hint);
    else setImageSourceIfChanged(primaryCanvas, active ? &concept_stop : &concept_play);
    if (!active) {
        lv_obj_set_style_img_recolor(primaryCanvas, lv_color_hex(accent), 0);
        lv_obj_set_style_img_recolor_opa(primaryCanvas, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_img_recolor_opa(primaryCanvas, LV_OPA_TRANSP, 0);
    }
    lv_obj_set_style_border_color(primaryButton,
                                  lv_color_hex(lastState.processComplete ? 0x28c870 :
                                      (active ? 0xd82828 : (lastState.temperatureStable ? 0x2ab8d8 : 0x555555))), 0);
}

void ConceptUI::handlePrimary() {
    if (view == View::Status) {
        if (standalonePreview) {
            standalonePreview = false;
            show(SCREEN_ID_BREW_SCREEN);
            return;
        }
        if (lastState.processComplete) {
            controller->clear();
            owner->changeScreen(SCREEN_ID_BREW_SCREEN);
        } else {
            controller->deactivate();
            controller->clear();
        }
    } else if (view == View::Brew) {
        if (!controller->getClientController()->isConnected()) {
            standalonePreview = true;
            standalonePreviewStartedAt = lv_tick_get();
            // The generated EEZ status flow has no active script while the
            // controller is absent and asserts in stopScript(). Standalone
            // preview is entirely native ConceptUI, so switch its view
            // directly without touching EEZ's state machine.
            show(SCREEN_ID_STATUS_SCREEN);
            return;
        }
        controller->activate();
    } else if (view == View::Water) {
        controller->isActive() ? controller->deactivate() : controller->activate();
    } else if (view == View::Grind) {
        controller->isGrindActive() ? controller->deactivateGrind() : controller->activateGrind();
    }
}

void ConceptUI::handleGesture(lv_dir_t direction) {
    if (direction == LV_DIR_TOP && view != View::Standby && view != View::Status && view != View::Menu) {
        controller->deactivate();
        owner->changeScreen(SCREEN_ID_MENU_SCREEN_NEW);
    } else if (direction == LV_DIR_BOTTOM && view == View::Menu) {
        switch (controller->getMode()) {
        case MODE_STEAM: owner->changeScreen(SCREEN_ID_STEAM_SCREEN); break;
        case MODE_WATER: owner->changeScreen(SCREEN_ID_WATER_SCREEN); break;
        case MODE_GRIND: owner->changeScreen(SCREEN_ID_GRIND_SCREEN); break;
        default: owner->changeScreen(SCREEN_ID_BREW_SCREEN); break;
        }
    } else if (direction == LV_DIR_LEFT && view == View::Brew) {
        visualProfileIndex = (visualProfileIndex + 1) % 5;
        owner->onNextProfile();
        owner->onProfileSelect();
    } else if (direction == LV_DIR_RIGHT && view == View::Brew) {
        visualProfileIndex = (visualProfileIndex + 4) % 5;
        owner->onPreviousProfile();
        owner->onProfileSelect();
    }
}

void ConceptUI::eventCallback(lv_event_t *event) {
    auto *self = static_cast<ConceptUI *>(lv_event_get_user_data(event));
    // Bubbling preserves the original child as `target`. Use the object whose
    // callback is currently executing so clicks on an icon still activate its
    // parent button (menu, play/stop and adjustment controls).
    lv_obj_t *target = lv_event_get_current_target(event);
    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev != nullptr) {
            lv_indev_get_point(indev, &self->touchStart);
            self->touchTracking = true;
        }
        return;
    }
    if (code == LV_EVENT_RELEASED && self->touchTracking) {
        self->touchTracking = false;
        lv_indev_t *indev = lv_indev_get_act();
        if (indev == nullptr) return;
        lv_point_t end;
        lv_indev_get_point(indev, &end);
        const int dx = end.x - self->touchStart.x;
        const int dy = end.y - self->touchStart.y;
        if (std::abs(dx) >= 35 || std::abs(dy) >= 35) {
            self->suppressNextClick = true;
            if (std::abs(dx) > std::abs(dy)) self->handleGesture(dx < 0 ? LV_DIR_LEFT : LV_DIR_RIGHT);
            else self->handleGesture(dy < 0 ? LV_DIR_TOP : LV_DIR_BOTTOM);
        } else if (self->view == View::Standby) {
            self->controller->setMode(MODE_BREW);
            self->owner->changeScreen(SCREEN_ID_BREW_SCREEN);
        }
        return;
    }
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t direction = lv_indev_get_gesture_dir(lv_indev_get_act());
        lv_indev_wait_release(lv_indev_get_act());
        self->suppressNextClick = true;
        self->handleGesture(direction);
        return;
    }
    if (code == LV_EVENT_CLICKED && self->suppressNextClick) {
        self->suppressNextClick = false;
        return;
    }
    if (target == self->root && self->view == View::Standby) {
        self->controller->setMode(MODE_BREW);
        self->owner->changeScreen(SCREEN_ID_BREW_SCREEN);
    } else if (target == self->primaryButton) {
        self->handlePrimary();
    } else if (target == self->mainValue && self->view == View::Brew) {
        self->setEditing(true);
    } else if (target == self->editButtons[0]) {
        self->owner->markProfileDirty();
        self->controller->lowerTemp();
    } else if (target == self->editButtons[1]) {
        self->owner->markProfileDirty();
        self->controller->raiseTemp();
    } else if (target == self->editButtons[2]) {
        self->owner->markProfileDirty();
        self->controller->lowerBrewTarget();
    } else if (target == self->editButtons[3]) {
        self->owner->markProfileDirty();
        self->controller->raiseBrewTarget();
    } else if (target == self->editDoneButton) {
        self->controller->onProfileSave();
        self->owner->markProfileClean();
        self->setEditing(false);
    } else if (target == self->grindMinus) {
        self->controller->lowerGrindTarget();
    } else if (target == self->grindPlus) {
        self->controller->raiseGrindTarget();
    } else if (target == self->menuButtons[0]) {
        self->setMode(View::Brew);
    } else if (target == self->menuButtons[1]) {
        self->setMode(View::Steam);
    } else if (target == self->menuButtons[2]) {
        self->setMode(View::Water);
    } else if (target == self->menuButtons[3]) {
        self->setMode(View::Grind);
    } else if (target == self->standbyButton) {
        if (self->view == View::Standby) {
            self->owner->changeScreen(SCREEN_ID_MENU_SCREEN_NEW);
        } else {
            self->controller->activateStandby();
            self->owner->changeScreen(SCREEN_ID_STANDBY_SCREEN);
        }
    }
}
