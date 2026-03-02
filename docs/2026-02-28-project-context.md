# Kontekst biznesowy - ClassSync / Green Accent PLANTS

## Problem

Zespół architektury krajobrazu (Green Accent) pracuje równolegle na wielu
projektach ArchiCAD (20+). Każdy projekt korzysta z własnego systemu
klasyfikacji roślin **"Green Accent PLANTS"**. System zawiera ~489 gatunków
roślin pogrupowanych w 8+ kategorii + 27 definicji właściwości (properties)
w 5 grupach.

**Główny problem**: Gdy ktoś doda nowy gatunek rośliny w jednym projekcie,
pozostałe projekty o tym nie wiedzą. Potrzebujemy współdzielonej biblioteki
klasyfikacji synchronizowanej między projektami.

**Ograniczenie Python API**: Python API ArchiCAD (`archicad` package) jest
**READ-ONLY** dla definicji klasyfikacji. Nie można tworzyć/modyfikować/usuwać
items. Jedyna droga: C++ API.

## Drzewo klasyfikacji (8 kategorii głównych)

| ID | Nazwa | Opis |
|----|-------|------|
| DRZ | DRZEWA | Liściaste (DRZ.L) i igłaste (DRZ.I), ~105 gatunków |
| KRZ | KRZEWY | Liściaste (KRZ.L) i igłaste (KRZ.I), ~150 gatunków |
| TO | TRAWY OZDOBNE | ~60 gatunków |
| BL.PK | BYLINY_PODKRZEWY | ~100 gatunków |
| PN | PNĄCZA | ~15 gatunków |
| RO | ROŚLINY OWOCOWE | |
| CB | CEBULOWE | ~25 gatunków |
| RW | ROŚLINY WODNE/NADWODNE | ~12 gatunków |

Dodatkowo w master: DO (DRZEWA OWOCOWE), KO (KRZEWY OWOCOWE), ZI (ZIOŁA).

Struktura ID: hierarchiczna z kropkami, 4 poziomy:
```
Kategoria > Podkategoria > Rodzaj > Gatunek
DRZ       > DRZ.L        > DRZ.L.01 > DRZ.L.01.01 (Brzoza pożyteczna 'Doorenbos')
```

**UWAGA:** W starszych projektach konwencja ID bywa złamana — np. `DR`
zamiast `DRZ`, `KR` zamiast `KRZ`. To powoduje kaskadowe rozbieżności
w całych gałęziach drzewa (patrz: `2026-03-02-sync-redesign.md`, sekcja 2.2).

## Properties (5 grup, 27 definicji)

- OGÓLNE (1 prop)
- ROŚLINY (3 props - enum)
- LAND4 Add-On (2 props)
- ROŚLINY WŁAŚCIWOŚCI OGÓLNE (9 props)
- ROŚLINY OBLICZENIA (12 props)

Typy: String, Boolean, Numeric (Area), Integer, Enum (z GUID kluczami).

## Problematyka synchronizacji (rozpracowana 2026-03-02)

Początkowe założenie (v0.5): dopasowanie wpisów wyłącznie po ID.
Okazało się niewystarczające — w praktyce występują rozbieżności
konwencji ID między projektami.

**Pełna analiza przypadków:** `docs/2026-03-02-sync-redesign.md`

Kluczowe ustalenia:

1. **Dopasowanie musi być dwuwymiarowe** — niezależnie wg ID i wg Name.
   Kombinacja obu wyników klasyfikuje typ niezgodności (S1–S6, H1–H6).

2. **Serwer (XML) = append-only.** Nie wolno zmieniać istniejących wpisów,
   bo nie wiemy które projekty ich używają. Zmiana wpisu na serwerze
   propaguje się jako "magiczna podmiana" do wszystkich projektów.
   Wszelkie korekty (ID, Name) robimy po stronie projektu.

3. **ArchiCAD wiąże elementy z klasyfikacją po GUID, nie po stringu ID.**
   Zmiana ID wpisu przez API (`ACAPI_Classification_ChangeClassificationItem`)
   jest bezpieczna — elementy w projekcie zachowują powiązanie.

4. **Kolizje ID rozwiązujemy w projekcie** — nadajemy wpisowi nowe wolne ID
   (sprawdzając obie strony), eksportujemy na serwer, importujemy brakujący.
   Wymaga otwarcia każdego projektu z kolizją osobno.

5. **Kaskada** — rozbieżność ID na poziomie kategorii/podkategorii propaguje
   się na 100+ potomków. Algorytm powinien wykrywać to top-down i prezentować
   jako jeden problem do rozwiązania, nie setki osobnych różnic.

## Znane wyzwania techniczne

1. **ID nie są unikalne między projektami** — złamane konwencje (DR vs DRZ)
2. **Properties różnią się między projektami** — różne zestawy
3. **Polskie znaki** — UTF-8 w XML, GS::UniString z CC_UTF8
4. **Enum z GUID** — wartości enum mają GUID jako klucz, różny w każdym projekcie
5. **ACAPI typo**: `API_SkipConflicitingItems` (nie "Conflicting")

## Poprzednie narzędzie (Python CLI)

Narzędzie `plantsync` (Python) - READ-ONLY:
- Parsuje i generuje XML w natywnym formacie ArchiCAD
- Porównuje dwa systemy klasyfikacji (diff)
- Łączy się z ArchiCAD przez Python API i czyta klasyfikacje
- Merguje do master XML
- Nie może TWORZYĆ klasyfikacji — stąd potrzeba C++ add-onu
