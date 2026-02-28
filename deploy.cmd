@echo off
:: Deploy ClassSync.apx to ArchiCAD 28 Add-Ons folder
:: Requires admin rights (UAC prompt)

set "SOURCE=%~dp0build\Release\ClassSync.apx"
set "DEST=C:\Program Files\Graphisoft\Archicad 28\Dodatki\ClassSync.apx"
set "OLD_ADDON=C:\Program Files\Graphisoft\Archicad 28\Dodatki\PlantSync.apx"

if not exist "%SOURCE%" (
    echo ERROR: Cannot find %SOURCE%
    echo Build first: cmake --build build --config Release
    pause
    exit /b 1
)

:: Check if running as admin
net session >nul 2>&1
if %errorlevel% == 0 (
    if exist "%OLD_ADDON%" (
        del /F "%OLD_ADDON%"
        echo Removed old PlantSync.apx
    )
    copy /Y "%SOURCE%" "%DEST%"
    if %errorlevel% == 0 (
        echo OK: Copied ClassSync.apx to Dodatki
    ) else (
        echo ERROR: Failed to copy
    )
    pause
    exit /b %errorlevel%
)

:: Re-launch as admin
echo Elevating privileges...
powershell -Command "Start-Process cmd -ArgumentList '/c if exist \"%OLD_ADDON%\" (del /F \"%OLD_ADDON%\" && echo Removed old PlantSync.apx) && copy /Y \"%SOURCE%\" \"%DEST%\" && echo OK: Copied ClassSync.apx && pause' -Verb RunAs"
