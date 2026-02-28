# PlantSync - ArchiCAD Add-On

Add-on do ArchiCAD 28 synchronizujacy system klasyfikacji roslin **"Green Accent PLANTS"**
miedzy projektami ArchiCAD a wspoldzielonym plikiem master XML.

## Problem

Zespol Green Accent pracuje rownolegle na 20+ projektach ArchiCAD. Kazdy projekt ma wlasny
system klasyfikacji roslin (~489 gatunkow). Gdy ktos doda nowy gatunek w jednym projekcie,
pozostale o tym nie wiedza. Potrzebujemy synchronizacji.

Python API ArchiCAD jest **READ-ONLY** dla klasyfikacji - nie mozna programowo tworzyc/modyfikowac
systemow klasyfikacji. C++ API to umozliwia.

## Co robi add-on

1. **Menu w ArchiCAD** - pozycja w menu Options lub wlasne podmenu
2. **Okno dialogowe** z:
   - TreeView pokazujacy drzewo klasyfikacji roslin w projekcie
   - Porownanie z master XML (nowe/zmienione/brakujace pozycje)
   - Przyciski: Import do projektu, Export z projektu, Pokaz diff
3. **Import klasyfikacji** - uzywa `ACAPI_Classification_Import()` do wczytania XML
4. **Export klasyfikacji** - czyta drzewo z projektu i zapisuje do XML
5. **Diff/Status** - porownuje projekt z masterem, pokazuje roznice

## Wymagania

- ArchiCAD 28 (build 6300+)
- API Development Kit 28.4001
- Visual Studio 2022 (z toolsetem v142 lub v143)
- CMake 3.16+
- Python 3.x (do kompilacji zasobow .grc)
- Windows 10/11

## Budowanie

```bash
# Z Developer Command Prompt for VS 2022 (lub po uruchomieniu vcvarsall.bat):
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

Wynik: `build/Release/PlantSync.apx`

## Instalacja

Skopiowac `PlantSync.apx` do katalogu add-onow ArchiCAD:
```
C:\Program Files\GRAPHISOFT\Archicad 28\Dodatki\
```
Lub: Options > Add-On Manager > dodac recznie.

## Struktura projektu

```
PlantSyncAddon/
  CMakeLists.txt            # Konfiguracja budowania
  Src/
    APIEnvir.h              # Boilerplate platformowy
    APICommon.h / .c        # Helpery API
    PlantSync.cpp           # Punkt wejscia (4 wymagane funkcje)
    PlantSyncDialog.hpp/cpp # Okno dialogowe
    XmlUtils.hpp/cpp        # Parsowanie XML
    ClassificationOps.hpp/cpp # Operacje na klasyfikacjach
  RFIX/
    PlantSyncFix.grc        # Identyfikator add-onu (MDID)
  RFIX.win/
    PlantSync.rc2           # Zasoby Windows
  RINT/
    PlantSync.grc           # Stringi, menu, dialogi
```

## Powiazane

- `C:\Users\Green\claude\plantsync\` - istniejace narzedzie Python CLI
- `C:\Users\Green\claude\Green Accent PLANTS.xml` - master XML z klasyfikacjami
- API DevKit: `C:\Program Files\GRAPHISOFT\API Development Kit 28.4001\`
