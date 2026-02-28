# CLAUDE.md - ClassSync ArchiCAD Add-On

## Projekt
Add-on C++ do ArchiCAD synchronizujacy system klasyfikacji roslin "Green Accent PLANTS"
miedzy projektami ArchiCAD a wspoldzielonym plikiem master XML.
Wiecej kontekstu: `docs/project-context.md`

## Srodowisko (stacja produkcyjna - NIE MODYFIKOWAC plikow poza tym katalogiem!)

### ArchiCAD
- **ArchiCAD 28** (build 6300, PL) - glowny: `C:\Program Files\GRAPHISOFT\Archicad 28\`
- **ArchiCAD 29** - zainstalowany: `C:\Program Files\GRAPHISOFT\Archicad 29\`
- Katalog add-onow AC28: `C:\Program Files\GRAPHISOFT\Archicad 28\Dodatki\`

### API Development Kit
- **DevKit 28.4001**: `C:\Program Files\GRAPHISOFT\API Development Kit 28.4001\`
  - Naglowki: `Support/Inc/` (ACAPinc.h, APIdefs_*.h)
  - Biblioteki: `Support/Lib/ACAP_STAT.lib`
  - Moduly: `Support/Modules/` (~60 modulow: GSRoot, DGLib, Geometry, JSON...)
  - Przyklady: `Examples/` (Classification_Test, DG_Test, Panel_Test, Property_Test...)
- Szczegoly API: `docs/archicad-api.md`

### Visual Studio & Build
- **VS 2022 Community** z toolsetem **v142** (VS 2019 compat)
- cmake/msbuild NIE sa w PATH - uzywac pelnych sciezek
- Szczegoly budowania: `docs/build-notes.md`

### Graphisoft Developer Registration
- Developer ID: 860318800
- PlantSync Local ID: 1954174874

## Architektura add-onu (v0.4)

### Struktura plikow
```
PlantSyncAddon/
  CLAUDE.md                   # Ten plik
  CMakeLists.txt              # AC_ADDON_NAME "ClassSync", toolset v142, GLOB sources
  deploy.cmd                  # Kopiuje ClassSync.apx do Dodatki (UAC, czeka na zamkniecie AC)
  Green Accent PLANTS.xml     # Master XML z klasyfikacjami roslin (~489 gatunkow, 2.3MB)
  Src/
    APIEnvir.h                # Boilerplate platformowy
    ClassSync.cpp             # 4 wymagane funkcje + menu handler + preferences load/save
    ClassSyncPalette.hpp/cpp  # DG::Palette, 18 kontrolek, singleton, import/export/resolve
    ClassificationData.hpp/cpp # Model danych (z GUIDs) + odczyt z projektu + diff
    XmlReader.hpp/cpp         # Parsowanie XML klasyfikacji z pliku na dysku
    XmlWriter.hpp/cpp         # Modyfikacja XML (ChangeItemName, AddItem)
  RFIX/
    PlantSyncFix.grc          # MDID (860318800, 1954174874)
  RFIX.win/
    PlantSync.rc2             # Windows resources master
  RINT/
    ClassSync.grc             # Stringi, menu, paleta (Palette | close, 960x560, 18 items)
  docs/
    archicad-api.md           # Sygnatury API: DG, Classification, Preferences, FileDialog
    build-notes.md            # Komendy budowania, typowe bledy, deploy
    project-context.md        # Kontekst biznesowy, drzewo klasyfikacji, properties
```

### Wymagane 4 funkcje (eksportowane przez ACAP_STAT.lib)
```cpp
API_AddonType CheckEnvironment(API_EnvirParams* envir);  // Nazwa i opis
GSErrCode RegisterInterface(void);                       // Rejestracja menu
GSErrCode Initialize(void);                              // Menu handler + preferences + palette registration
GSErrCode FreeData(void);                                // Save prefs + cleanup
```

### Paleta (ClassSyncPalette)
- 3 kolumny TreeView: Project | Differences | Server (XML)
- 4 przyciski akcji: Import, Export, Use Project, Use Server
- Browse button + preferences dla sciezki XML
- Label wersji (v0.4)
- Kolory: zielony (0,130,60)=nowe, niebieski (0,80,170)=brakujace, ceglasty (180,50,0)=konflikt
- Singleton pattern, `BeginEventProcessing()`/`EndEventProcessing()`
- `ACAPI_RegisterModelessWindow` z APIPalMsg_* callback
- `DG::TreeViewObserver` do sledzenia selekcji w drzewku konfliktow

### Mechanizm rozwiazywania roznic
- **Import**: `ACAPI_Classification_Import(xml, MergeConflicting, SkipConflicting)` - importuje brakujace z XML
- **Export**: `AddItemToXml()` - dodaje brakujacy item do pliku XML (szuka rodzica po ID)
- **Use Project**: `ChangeItemNameInXml()` - zmienia nazwe w XML na wersje z projektu
- **Use Server**: `ACAPI_Classification_ChangeClassificationItem()` - zmienia nazwe w projekcie na wersje z XML

### Build & Deploy
```bash
# Build (cmake pelna sciezka, bo nie w PATH):
"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build --config Release

# Deploy (reczne uruchomienie z admin):
deploy.cmd
```

### Konwencje
- C++17, UNICODE/_UNICODE
- Zasoby w plikach .grc, Resource ID: 32000+ (nazwa), 32500+ (menu), 32600 (paleta)
- .apx = .dll eksportujacy GetExportedFuncAddrs@1 i SetImportedFuncAddrs@2
- MSVC /MD, _ITERATOR_DEBUG_LEVEL=0
- Po dodaniu nowych plikow .cpp/.hpp: re-run cmake configure (GLOB)
- Preferencje: ACAPI_SetPreferences/GetPreferences z struct ClassSyncPrefs
