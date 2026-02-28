# ClassSync - ArchiCAD Add-On

Add-on do ArchiCAD 28 synchronizujacy system klasyfikacji roslin **"Green Accent PLANTS"**
miedzy projektami ArchiCAD a wspoldzielonym plikiem master XML.

## Problem

Zespol Green Accent pracuje rownolegle na 20+ projektach ArchiCAD. Kazdy projekt ma wlasny
system klasyfikacji roslin (~489 gatunkow). Gdy ktos doda nowy gatunek w jednym projekcie,
pozostale o tym nie wiedza. Python API ArchiCAD jest READ-ONLY dla klasyfikacji - jedyna
droga to C++ API.

## Co robi add-on

- **Menu ClassSync > Sync** - otwiera palete porownania
- **3-panelowy widok**: Project | Differences | Server (XML)
- **Import** - importuje brakujace klasyfikacje z XML do projektu
- **Export** - dodaje brakujace z projektu do pliku XML
- **Resolve Conflicts** - Use Project / Use Server dla roznic w nazwach
- **Kolorowanie diff**: zielony=nowe, niebieski=brakujace, ceglasty=konflikt

## Budowanie

```bash
# Configure (raz, lub po dodaniu nowych plikow):
bash configure.sh

# Build:
bash build.sh

# Deploy do ArchiCAD (wymaga admin):
bash deploy.sh
```

## Wymagania

- ArchiCAD 28 (build 6300+)
- API Development Kit 28.4001
- Visual Studio 2022 (z toolsetem v142)
- CMake 3.16+
- Python 3.x (kompilacja zasobow .grc)
- Windows 10/11
