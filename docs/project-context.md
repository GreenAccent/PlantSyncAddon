# Kontekst biznesowy - ClassSync / Green Accent PLANTS

## Problem

Zespol architektury krajobrazu (Green Accent) pracuje rownolegle na wielu projektach ArchiCAD (20+). Kazdy projekt korzysta z wlasnego systemu klasyfikacji roslin **"Green Accent PLANTS"**. System zawiera ~489 gatunkow roslin pogrupowanych w 8+ kategorii + 27 definicji wlasciwosci (properties) w 5 grupach.

**Glowny problem**: Gdy ktos doda nowy gatunek rosliny w jednym projekcie, pozostale projekty o tym nie wiedza. Potrzebujemy wspoldzielonej biblioteki klasyfikacji synchronizowanej miedzy projektami.

**Ograniczenie Python API**: Python API ArchiCAD (`archicad` package) jest **READ-ONLY** dla definicji klasyfikacji. Nie mozna tworzyc/modyfikowac/usuwac items. Jedyna droga: C++ API.

## Drzewo klasyfikacji (8 kategorii glownych)

| ID | Nazwa | Opis |
|----|-------|------|
| DRZ | DRZEWA | Lisciaste (DRZ.L) i iglaste (DRZ.I), ~105 gatunkow |
| KRZ | KRZEWY | Lisciaste (KRZ.L) i iglaste (KRZ.I), ~150 gatunkow |
| TO | TRAWY OZDOBNE | ~60 gatunkow |
| BL.PK | BYLINY_PODKRZEWY | ~100 gatunkow |
| PN | PNACZA | ~15 gatunkow |
| RO | ROSLINY OWOCOWE | |
| CB | CEBULOWE | ~25 gatunkow |
| RW | ROSLINY WODNE/NADWODNE | ~12 gatunkow |

Dodatkowo w master: DO (DRZEWA OWOCOWE), KO (KRZEWY OWOCOWE), ZI (ZIOLA).

Struktura ID: hierarchiczna z kropkami, np:
- `DRZ` > `DRZ.L` > `DR.L.01` > `DR.L.01.01` (Brzoza pozyteczna 'Doorenbos')

## Properties (5 grup, 27 definicji)

- OGOLNE (1 prop)
- ROSLINY (3 props - enum)
- LAND4 Add-On (2 props)
- ROSLINY WLASCIWOSCI OGOLNE (9 props)
- ROSLINY OBLICZENIA (12 props)

Typy: String, Boolean, Numeric (Area), Integer, Enum (z GUID kluczami).

## Znane wyzwania

1. **ID nie sa unikalne miedzy projektami** - matchowanie po ID string
2. **Properties roznia sie miedzy projektami** - rozne zestawy
3. **Polskie znaki** - UTF-8 w XML, GS::UniString z CC_UTF8
4. **Enum z GUID** - wartosci enum maja GUID jako klucz, rozny w kazdym projekcie
5. **ACAPI typo**: `API_SkipConflicitingItems` (nie "Conflicting")

## Poprzednie narzedzie (Python CLI)

Narzedzie `plantsync` (Python) - READ-ONLY:
- Parsuje i generuje XML w natywnym formacie ArchiCAD
- Porownuje dwa systemy klasyfikacji (diff)
- Laczy sie z ArchiCAD przez Python API i czyta klasyfikacje
- Merguje do master XML
- Nie moze TWORZYC klasyfikacji - stad potrzeba C++ add-onu
