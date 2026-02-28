#!/bin/bash
# Re-run CMake configure (needed after adding new .cpp/.hpp files)
"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" -S "$(dirname "$0")" -B "$(dirname "$0")/build" -G "Visual Studio 17 2022" -A x64 -DAC_API_DEVKIT_DIR="C:/Program Files/GRAPHISOFT/API Development Kit 28.4001"
