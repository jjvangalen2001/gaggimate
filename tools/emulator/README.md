# GaggiMate pc-emulator

Vereist: Docker Desktop.

Start vanuit PowerShell:

```powershell
.\tools\emulator\start-emulator.ps1
```

De browser opent automatisch op een lokaal noVNC-scherm. Klikken en slepen met
de linkermuisknop emuleert aanraken en swipen. De controller in deze emulator is
een softwaremodel; hij kan geen pomp, boiler of andere machinehardware bedienen.

Stoppen:

```powershell
docker container stop gaggimate-emulator
```
