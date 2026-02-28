# Build Notes - ClassSync

## Build Commands

```bash
# Full path to cmake (not in PATH):
"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"

# Configure (only needed once, or after adding new .cpp/.hpp files):
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DAC_API_DEVKIT_DIR="C:/Program Files/GRAPHISOFT/API Development Kit 28.4001"

# Build:
cmake --build build --config Release

# Deploy (requires admin - copies .apx to ArchiCAD Dodatki):
deploy.cmd
```

## Key Build Facts
- VS 2022 Community with toolset v142 (VS 2019 compat, set in CMakeLists.txt)
- cmake/msbuild/cl.exe NOT in PATH - use full paths or vcvarsall.bat
- Runtime: /MD (MultiThreadedDLL)
- _ITERATOR_DEBUG_LEVEL=0
- Output: .apx (renamed .dll)
- Exports: GetExportedFuncAddrs@1, SetImportedFuncAddrs@2
- Resource compilation: CompileResources.py from DevKit
- C files compiled as C++ (LANGUAGE CXX property)
- CMake uses file(GLOB) - must re-run cmake configure after adding new source files!

## Deploy
```cmd
deploy.cmd
```
- Waits for Archicad.exe to close (polling co 2s)
- Elevates to admin via UAC (PowerShell Start-Process -Verb RunAs)
- Removes old PlantSync.apx if exists
- Copies ClassSync.apx to `C:\Program Files\GRAPHISOFT\Archicad 28\Dodatki\`

## Common Build Errors Encountered
- `AttachToAllItems` not found: requires DG::CompoundItemObserver inheritance. Use individual Attach() instead.
- Linker: unresolved externals after adding new .cpp: re-run cmake configure to update GLOB
- `__ACDLL_CALL` not recognized: don't use it on entry point functions
- MDID validation: must use registered Developer ID + Local ID
- `ToCStr()` on concatenation: use `GS::UniString` directly with `ToCStr(0, MaxUSize, CC_UTF8)`
- VS generator: use `"Visual Studio 17 2022"` (not 16 2019 which was for old VS)
