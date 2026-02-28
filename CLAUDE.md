# CLAUDE.md - PlantSync ArchiCAD Add-On

## Projekt
Add-on C++ do ArchiCAD synchronizujacy system klasyfikacji roslin "Green Accent PLANTS"
miedzy projektami ArchiCAD a wspoldzielonym plikiem master XML.

## Srodowisko (stacja produkcyjna - NIE MODYFIKOWAC plikow poza tym katalogiem!)

### ArchiCAD
- **ArchiCAD 28** (build 6300, PL) - glowny, uruchomiony: `C:\Program Files\GRAPHISOFT\Archicad 28\`
- **ArchiCAD 29** - zainstalowany: `C:\Program Files\GRAPHISOFT\Archicad 29\`
- ArchiCAD 26, 27 - tez zainstalowane
- Jezyk: polski (PL) - katalogi: Dodatki, Biblioteka, Dokumentacja itd.
- Katalog add-onow AC28: `C:\Program Files\GRAPHISOFT\Archicad 28\Dodatki\`

### API Development Kit
- **DevKit 28.4001**: `C:\Program Files\GRAPHISOFT\API Development Kit 28.4001\`
  - Naglowki: `Support/Inc/` (ACAPinc.h, APIdefs_*.h)
  - Biblioteki: `Support/Lib/ACAP_STAT.lib`, `Support/Lib/ACAP_STATD.lib`
  - Moduly: `Support/Modules/` (~60 modulow: GSRoot, DGLib, Geometry, JSON...)
  - Narzedzia: `Support/Tools/CompileResources.py`, `Support/Tools/Win/ResConv.exe`
  - Przyklady: `Examples/` (53 przyklady, kazdy z CMakeLists.txt)
  - Dokumentacja HTML: `Documentation/HTML/html/` (Doxygen)
- **Brak DevKit dla AC29** - tylko 28.4001

### Kluczowe przyklady do nauki
- `Examples/Classification_Test/` - operacje CRUD na klasyfikacjach + import XML
- `Examples/DG_Test/` - dialogi, TreeView, ListBox, przyciski (C-style callbacks)
- `Examples/Panel_Test/` - palety, panele ustawien (C++ observer pattern)
- `Examples/Property_Test/` - property definitions CRUD, linkowanie do klasyfikacji
- `Examples/Interface_Functions/` - rejestracja menu, podstawowa struktura add-onu
- `Examples/Notification_Manager/` - obserwatory zmian, notyfikacje

### Visual Studio
- **VS 2022 Community**: `C:\Program Files\Microsoft Visual Studio\2022\Community\`
- MSVC toolsety: 14.44 (najnowszy), 14.38, 14.36
- **UWAGA**: DevKit wymaga toolset v142 (VS 2019). Mamy v143 (VS 2022).
  - Opcja 1: Doinstalowac v142 przez VS Installer > Individual Components > "MSVC v142"
  - Opcja 2: Zmienic CMakeLists na v143 (nieoficjalne, moze dzialac)
- Clang 19.1.5 (bundled z VS)
- CMake 3.31.6 (bundled z VS): `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`
- Ninja 1.12.1 (bundled z VS)
- **cl.exe, cmake, msbuild NIE sa w PATH** - trzeba uzyc Developer Command Prompt lub vcvarsall.bat
- vcvarsall.bat: `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat`
- Windows SDK: 10.0.26100.0, 10.0.22621.0

### Python (istniejace narzedzie)
- Python 3.10: `C:\Users\Green\AppData\Local\Programs\Python\Python310\`
- Pakiet archicad 29.3000 (pip)
- Istniejacy CLI: `C:\Users\Green\claude\plantsync\`
- Master XML: `C:\Users\Green\claude\Green Accent PLANTS.xml`

### Graphisoft Developer Registration
- Developer ID: 860318800
- Authorization Key: VCMXXGWYMZV3XXQK
- PlantSync Local ID: 1954174874
- Portal: https://archicadapi.graphisoft.com/

### Brak
- vcpkg, Conan - niezainstalowane
- GCC/MinGW - niezainstalowane
- DevKit dla AC29

## Architektura add-onu

### Struktura plikow
```
PlantSyncAddon/
  CMakeLists.txt              # AC_ADDON_NAME "ClassSync", toolset v142
  deploy.cmd                  # Kopiuje ClassSync.apx do Dodatki (UAC)
  Src/
    APIEnvir.h                # Boilerplate platformowy
    ClassSync.cpp             # 4 wymagane funkcje + menu handler + ACAPI_RegisterModelessWindow
    ClassSyncPalette.hpp/cpp  # DG::Palette z 3 TreeViews (Project/Conflicts/Server)
    ClassificationData.hpp/cpp # Model danych + odczyt z projektu + algorytm diff
    XmlReader.hpp/cpp         # Parsowanie XML klasyfikacji z pliku na dysku
  RFIX/
    PlantSyncFix.grc          # MDID (860318800, 1954174874)
  RFIX.win/
    PlantSync.rc2             # Windows resources master
  RINT/
    ClassSync.grc             # Stringi, menu, paleta (Palette | close, 960x520, 11 items)
```

### Wymagane 4 funkcje (eksportowane przez ACAP_STAT.lib)
```cpp
API_AddonType CheckEnvironment(API_EnvirParams* envir);  // Nazwa i opis
GSErrCode RegisterInterface(void);                       // Rejestracja menu
GSErrCode Initialize(void);                              // Instalacja handlerow
GSErrCode FreeData(void);                                // Czyszczenie
```

### Kluczowe API klasyfikacji (C++)
```cpp
// Odczyt
ACAPI_Classification_GetClassificationSystems(systems)
ACAPI_Classification_GetClassificationSystemRootItems(systemGuid, items)
ACAPI_Classification_GetClassificationItemChildren(itemGuid, children)

// Tworzenie (niedostepne w Python API!)
ACAPI_Classification_CreateClassificationSystem(system)
ACAPI_Classification_CreateClassificationItem(item, systemGuid, parentGuid, nextGuid)
ACAPI_Classification_ChangeClassificationItem(item)
ACAPI_Classification_DeleteClassificationItem(itemGuid)

// Import XML (to samo co Options > Classification Manager > Import)
ACAPI_Classification_Import(xmlString, systemConflictPolicy, itemConflictPolicy)
// Polityki: API_ReplaceConflictingSystems / API_MergeConflictingSystems / API_SkipConflictingSystems
//           API_ReplaceConflictingItems / API_SkipConflicitingItems
```

### Kluczowe API properties (C++)
```cpp
ACAPI_Property_CreatePropertyGroup(group)
ACAPI_Property_CreatePropertyDefinition(definition)  // definition.availability = [classificationGuids]
ACAPI_Property_Import(xmlString, conflictPolicy)
```

### Dialogi (DG)
Dwa style:
1. **C-style callback**: `DGModalDialog(resModule, resId, resModule, CallbackFunc, userData)`
2. **C++ observer**: klasa dziedziczy z `DG::Palette` + `DG::PanelObserver` + `DG::ButtonItemObserver`

TreeView C++ API (DG::SingleSelTreeView):
```cpp
Int32 item = tree.AppendItem(parentItem);       // dodaje child, zwraca item ID
tree.SetItemText(item, "Nazwa");
tree.SetItemTextColor(item, Gfx::Color(200,0,0)); // UChar 0-255 RGB
tree.ExpandItem(item);
tree.DisableDraw(); / tree.EnableDraw();          // batch updates
tree.DeleteItem(item);
```

Paleta (okno plywajace):
- GRC typ: `Palette | close` (lub `| grow` z resize handlerem)
- `DG::Palette(resModule, resId, resModule, guid)` + `BeginEventProcessing()` / `EndEventProcessing()`
- Rejestracja: `ACAPI_RegisterModelessWindow(refId, callback, enableFlags, guid)`
- Flagi: `API_PalEnabled_FloorPlan | API_PalEnabled_3D | ...`
- Singleton pattern, APIPalMsg_* callback

Gfx::Color (w GSRoot/Color.hpp):
```cpp
Gfx::Color(UChar r, UChar g, UChar b);  // 0-255
Gfx::Color::Red, ::Green, ::Blue        // stale
```

### Build
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019" -A x64 -DAC_API_DEVKIT_DIR="C:/Program Files/GRAPHISOFT/API Development Kit 28.4001"
cmake --build . --config Release
# Wynik: build/Release/ClassSync.apx
```

### Instalacja add-onu
`deploy.cmd` kopiuje ClassSync.apx do: `C:\Program Files\GRAPHISOFT\Archicad 28\Dodatki\`
Lub: Options > Add-On Manager > wskazac plik
**UWAGA**: Usunac stary PlantSync.apx z Dodatki jesli istnieje!

### Konwencje
- C++17
- UNICODE / _UNICODE
- Zasoby (stringi, menu, dialogi) definiowane w plikach .grc
- Resource ID: 32000+ (nazwa), 32500+ (menu/dialogi)
- .apx = przemianowany .dll eksportujacy GetExportedFuncAddrs@1 i SetImportedFuncAddrs@2
- Pliki .c kompilowane jako C++ (force)
- MSVC runtime: /MD (MultiThreadedDLL)
- _ITERATOR_DEBUG_LEVEL=0

## Kontekst biznesowy
- Firma: Green Accent (architektura krajobrazu)
- 20+ rownoczesnych projektow ArchiCAD
- System klasyfikacji roslin: ~489 gatunkow, 8 kategorii, 27 properties
- Problem: brak synchronizacji klasyfikacji miedzy projektami
- Istniejace rozwiazanie: Python CLI (plantsync) - dziala ale READ-ONLY dla klasyfikacji
