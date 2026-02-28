@echo off
:: Deploy ClassSync.apx to ArchiCAD 28 Add-Ons folder
:: Waits for Archicad.exe to close, then copies with admin rights (UAC)

set "SOURCE=%~dp0build\Release\ClassSync.apx"
set "DEST=C:\Program Files\Graphisoft\Archicad 28\Dodatki\ClassSync.apx"
set "OLD_ADDON=C:\Program Files\Graphisoft\Archicad 28\Dodatki\PlantSync.apx"

if not exist "%SOURCE%" (
    echo ERROR: Cannot find %SOURCE%
    echo Build first: cmake --build build --config Release
    pause
    exit /b 1
)

:: Wait for ArchiCAD to close
tasklist /FI "IMAGENAME eq Archicad.exe" 2>NUL | find /I "Archicad.exe" >NUL
if %errorlevel% == 0 (
    echo Waiting for Archicad.exe to close...
    :waitloop
    timeout /t 2 /nobreak >NUL
    tasklist /FI "IMAGENAME eq Archicad.exe" 2>NUL | find /I "Archicad.exe" >NUL
    if %errorlevel% == 0 goto waitloop
    echo Archicad.exe closed.
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

:: Re-launch as admin (with wait loop built in)
echo Elevating privileges...
powershell -Command "Start-Process cmd -ArgumentList '/c echo Waiting for Archicad.exe... && powershell -Command \"while (Get-Process Archicad -ErrorAction SilentlyContinue) { Start-Sleep 2 }\" && echo Archicad closed. && if exist \"%OLD_ADDON%\" (del /F \"%OLD_ADDON%\" && echo Removed old PlantSync.apx) && copy /Y \"%SOURCE%\" \"%DEST%\" && echo OK: Copied ClassSync.apx && pause' -Verb RunAs"
