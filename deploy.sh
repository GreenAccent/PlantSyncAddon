#!/bin/bash
# Deploy ClassSync.apx to ArchiCAD 28 (requires admin elevation via UAC)
SCRIPT_DIR="$(dirname "$0")"
DEPLOY_PS1="$SCRIPT_DIR/_deploy_now.ps1"
cat > "$DEPLOY_PS1" << 'PS1'
Copy-Item -Force "C:\Users\Green\claude\PlantSyncAddon\build\Release\ClassSync.apx" "C:\Program Files\GRAPHISOFT\Archicad 28\Dodatki\ClassSync.apx"
PS1
powershell -Command "Start-Process powershell -ArgumentList '-ExecutionPolicy','Bypass','-File','$DEPLOY_PS1' -Verb RunAs -Wait"
rm -f "$DEPLOY_PS1"
ls -la "/c/Program Files/GRAPHISOFT/Archicad 28/Dodatki/ClassSync.apx"
