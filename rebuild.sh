#!/bin/bash
# Clean rebuild ClassSync add-on
"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build "$(dirname "$0")/build" --config Release --clean-first
