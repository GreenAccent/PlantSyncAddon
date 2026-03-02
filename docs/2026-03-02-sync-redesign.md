# ClassSync — Synchronizacja klasyfikacji roślin

Dokument do dyskusji. Cel: ustalenie które przypadki niezgodności
występują w praktyce i jakie rozwiązania należy zaimplementować.

---

# 1. STAN OBECNY

## 1.1. Struktura danych klasyfikacji

Każdy wpis ma **ID** (kod hierarchiczny) i **Name** (nazwa rośliny).
ID koduje położenie w drzewie — każdy segment to jeden poziom:

```
Poziom 1 (Kategoria)  : DRZ                → "DRZEWA"
Poziom 2 (Podkat.)    : DRZ.L              → "Liściaste"
Poziom 3 (Rodzaj)     : DRZ.L.01           → "Betula (Brzoza)"
Poziom 4 (Gatunek)    : DRZ.L.01.01        → "Brzoza pożyteczna 'Doorenbos'"
```

## 1.2. Obecne porównywanie (v0.5)

ClassSync porównuje wpisy **wyłącznie po ID**.
- Ten sam ID + różna nazwa = konflikt nazwy (wykrywany).
- Ta sama nazwa + różne ID = dwa oddzielne byty (NIE wykrywany).

## 1.3. Obecne funkcje

| Funkcja | Co robi | Status |
|---------|---------|--------|
| Import | Kopiuje wpis z serwera do projektu | ✅ OK |
| Export | Dodaje wpis z projektu do XML | ✅ OK (append) |
| Use Server | Zmienia nazwę w projekcie na wersję z serwera | ⚠️ Nie rozwiązuje kolizji S2 |
| Use Project | Zmienia nazwę w XML na wersję z projektu | ❌ Łamie zasadę append-only |

---

# 2. MOŻLIWE PRZYPADKI NIEZGODNOŚCI

## 2.1. Przypadki proste

### 2.1.1. S1 — Pełna zgodność — ✅ obsługiwane

```
Projekt: ID: DRZ.L.01.01   Name: "Brzoza pożyteczna 'Doorenbos'"
Serwer:  ID: DRZ.L.01.01   Name: "Brzoza pożyteczna 'Doorenbos'"
```

ID i Name identyczne — brak akcji.

### 2.1.2. S2 — Kolizja ID (to samo ID, różne rośliny) — ⚠️ wymaga przeprojektowania

```
Projekt: ID: DRZ.L.01.05   Name: "Klon pospolity 'Globosum'"
Serwer:  ID: DRZ.L.01.05   Name: "Klon jawor 'Brilliantissimum'"
```

Przyczyna: w kategorii było 4 gatunki, ktoś na jednej maszynie dodał piąty
gatunek X pod ID .05, a na innej maszynie pod tym samym ID .05 zapisano
gatunek Y. ClassSync wykrywa to jako "Conflict" i oferuje Use Server /
Use Project, ale nadpisanie nazwy = utrata jednej rośliny.

### 2.1.3. S3 — Niezgodność ID (ta sama nazwa, inne ID) — ❌ nieobsługiwane

```
Projekt: ID: DR.L.01.01    Name: "Brzoza pożyteczna 'Doorenbos'"
Serwer:  ID: DRZ.L.01.01   Name: "Brzoza pożyteczna 'Doorenbos'"
```

Ta sama roślina, inna konwencja ID. Obecna wersja widzi to jako dwa
osobne byty ("Only in Project" + "Only on Server").

### 2.1.4. S4 — Nowy w projekcie — ✅ obsługiwane (Export)

```
Projekt: ID: DRZ.L.99.01   Name: "Nowy gatunek XYZ"
Serwer:  (brak)
```

### 2.1.5. S5 — Nowy na serwerze — ✅ obsługiwane (Import)

```
Projekt: (brak)
Serwer:  ID: DRZ.L.99.01   Name: "Nowy gatunek XYZ"
```

### 2.1.6. S6 — Podwójny konflikt (ID→inny wpis, Name→inny wpis) — ❌ nieobsługiwane

```
Projekt: ID: DRZ.L.01.01   Name: "Brzoza pożyteczna 'Doorenbos'"
Serwer:  ID: DRZ.L.01.01   Name: "Brzoza brodawkowata"            ← ID pasuje tu
Serwer:  ID: DRZ.L.05.03   Name: "Brzoza pożyteczna 'Doorenbos'"  ← Name pasuje tu
```

Kombinacja S2 + S3. ID wskazuje na jeden wpis serwera, Name na inny.

## 2.2. Przypadki hierarchiczne (kaskadowe)

ID koduje położenie w drzewie, więc rozbieżność na poziomie rodzica
powoduje **kaskadę** na wszystkie elementy poniżej.

### 2.2.1. H1 — Rozbieżność kategorii (poziom 1) — ❌ nieobsługiwane

```
Serwer:                          Projekt:
DRZ                "DRZEWA"      DR                 "DRZEWA"
├─ DRZ.L           "Liściaste"   ├─ DR.L            "Liściaste"
│  ├─ DRZ.L.01     "Betula"      │  ├─ DR.L.01      "Betula"
│  │  ├─ DRZ.L.01.01 "Brzoza.."  │  │  ├─ DR.L.01.01 "Brzoza.."
│  │  └─ DRZ.L.01.02 "Brzoza.."  │  │  └─ DR.L.01.02 "Brzoza.."
│  └─ DRZ.L.02     "Quercus"     │  └─ DR.L.02      "Quercus"
└─ DRZ.I           "Igłaste"     └─ DR.I            "Igłaste"
```

`DRZ` vs `DR` — 100+ wpisów z innym prefiksem. Rozbieżność jest JEDNA
(na poziomie kategorii), reszta to kaskada.

### 2.2.2. H2 — Rozbieżność podkategorii (poziom 2) — ❌ nieobsługiwane

```
Serwer:  DRZ > DRZ.L  > DRZ.L.01  > DRZ.L.01.01
Projekt: DRZ > DRZ.LI > DRZ.LI.01 > DRZ.LI.01.01
```

Analogiczne do H1, mniejszy zakres kaskady.

### 2.2.3. H3 — Rozbieżność numeracji (zero-padding) — ❌ nieobsługiwane

```
Serwer:  DRZ.L.01   → "Betula (Brzoza)"
Projekt: DRZ.L.1    → "Betula (Brzoza)"
```

### 2.2.4. H4 — Mieszane konwencje w jednym projekcie — ❌ nieobsługiwane

```
DRZ > DRZ.L > DRZ.L.01 > ...       ← zgodne z serwerem ✓
KRZ > KR.L  > KR.L.01  > ...       ← złamana konwencja ✗
BL.PK > BL.P > BL.P.01 > ...       ← złamana ✗
```

Każda kategoria może wymagać oddzielnego traktowania.

### 2.2.5. H5 — Brakujący poziom pośredni — ❌ nieobsługiwane

```
Serwer:  DRZ > DRZ.L > DRZ.L.01 > DRZ.L.01.01
Projekt: DRZ > DRZ.L.01 > DRZ.L.01.01     ← pominięta podkategoria
```

### 2.2.6. H6 — Wpis pod złym rodzicem — ❌ nieobsługiwane

```
Serwer:  DRZ > DRZ.L > DRZ.L.01.01    "Brzoza pożyteczna 'Doorenbos'"
Projekt: KRZ > KRZ.L > KRZ.L.01.01    "Brzoza pożyteczna 'Doorenbos'"
```

Name match, ale zupełnie inna gałąź drzewa.

## 2.3. Tabela zbiorcza

| # | Przypadek | Przykład | Status |
|---|-----------|----------|--------|
| S1 | Pełna zgodność | ID i Name identyczne | ✅ OK |
| S2 | Kolizja ID | To samo ID, różne rośliny | ⚠️ częściowo |
| S3 | Niezgodność ID | Ta sama nazwa, inne ID | ❌ |
| S4 | Nowy w projekcie | Brak w XML | ✅ OK |
| S5 | Nowy na serwerze | Brak w projekcie | ✅ OK |
| S6 | Podwójny konflikt | ID→inny, Name→inny | ❌ |
| H1 | Inna kategoria | DRZ vs DR (cała gałąź) | ❌ |
| H2 | Inna podkategoria | DRZ.L vs DRZ.LI | ❌ |
| H3 | Inna numeracja | 01 vs 1 | ❌ |
| H4 | Mieszane konwencje | Część OK, część nie | ❌ |
| H5 | Brak poziomu | Pominięta podkategoria | ❌ |
| H6 | Zły rodzic | Roślina w złej kategorii | ❌ |

---

# 3. OCZEKIWANY STAN DOCELOWY

## 3.1. Zasada nadrzędna: serwer = append-only

Baza serwerowa (XML) jest współdzielona między wieloma projektami.
Nie wiemy które projekty używają której wersji bazy. Dlatego:

- **Na serwerze NIE WOLNO zmieniać istniejących wpisów** (ani ID, ani Name).
  Zmiana "Brzoza X" → "Brzoza Y" w XML spowoduje lawinę niezgodności
  we wszystkich projektach które używały wcześniej tego wpisu.
- **Na serwerze MOŻNA jedynie DODAWAĆ** nowe wpisy (append-only), ponieważ
  zakładamy że wszystkie istniejące na serwerze wpisy są/mogą być w użyciu
  w innych projektach.
- **Wszelkie korekty (zmiany ID, Name) należy robić w PROJEKCIE** dostosowując
  klasyfikację w projekcie do klasyfikacji przechowywanej na serwerze.

Konsekwencja: przycisk "Use Project" (nadpisz nazwę w XML) jest niebezpieczny,
ponieważ łamie tę zasadę → do usunięcia lub silnego ograniczenia (edytujemy
tylko w sytuacji, kiedy użytkownik chce skorygować wpis, który niedawno dodał
i jest pewien, że ten wpis jeszcze nie jest używany w innych projektach.
Można zrobić okno komunikatu ostrzegające "Editing ID or Name on server side
will cause conflicts in all projects using this item".

## 3.2. Nowy algorytm dopasowania

Dla każdego wpisu w projekcie ClassSync szuka odpowiednika na serwerze
**dwoma niezależnymi ścieżkami**:

- **Szukaj wg ID** — czy na serwerze istnieje wpis z takim samym kodem?
- **Szukaj wg Name** — czy na serwerze istnieje wpis z taką samą nazwą?

Te dwa wyszukiwania działają niezależnie od siebie. Jeden wpis projektu
może znaleźć odpowiednik wg ID, wg Name, wg obu albo wg żadnego.
Kombinacja wyników mówi nam z jakim przypadkiem mamy do czynienia:

| Dopasowano wg ID? | Dopasowano wg Name? | Ten sam wpis? | Przypadek |
|--------------------|----------------------|---------------|-----------|
| ✅ tak | ✅ tak | tak | S1 — pełna zgodność |
| ✅ tak | ❌ nie | — | S2 — kolizja ID |
| ❌ nie | ✅ tak | — | S3 — niezgodność ID |
| ✅ tak | ✅ tak | nie (różne wpisy!) | S6 — podwójny konflikt |
| ❌ nie | ❌ nie | — | S4 — nowy wpis, brak na serwerze |

Analogicznie wpisy serwera, które nie zostały dopasowane z żadnej strony
= S5 (nowy na serwerze, brak w projekcie).

**Wykrywanie kaskady:** Jeżeli wpis nadrzędny (np. kategoria) został
dopasowany wg Name ale nie wg ID → jego potomkowie to prawdopodobnie
kaskada (H1–H4), a nie osobne problemy. Grupujemy je jako jeden
wpis "ID cascade".

## 3.3. Rozwiązania per przypadek

### 3.3.1. R1 — S1 → Brak akcji

Wpis zsynchronizowany (ID i Name identyczne po obu stronach).

### 3.3.2. R2 — S2 → Reassign ID + Export + Import

```
Projekt: ID: DRZ.L.01.05   Name: "Klon pospolity 'Globosum'"
Serwer:  ID: DRZ.L.01.05   Name: "Klon jawor 'Brilliantissimum'"
```

1. **Zmień ID w projekcie** na następne wolne (np. .05 → .07).
   ArchiCAD zachowuje powiązania elementów (wewnętrznie GUID).
2. **Eksportuj** wpis projektu pod nowym ID na serwer.
3. **Importuj** wpis serwera pod oryginalnym ID do projektu.

Wynik: serwer ma oba gatunki (append-only zachowane), projekt ma oba,
elementy wskazują na właściwą roślinę.

**Następne wolne ID** — ClassSync sprawdza OBA źródła:
```
Serwer:  .01, .02, .03, .04, .05 ("Klon jawor")
Projekt: .01, .02, .03, .04, .05 ("Klon pospolity"), .06
Następne wolne = .07
```

**Ważne:** Kolizje można rozwiązać TYLKO otwierając projekt z kolizją.
20 projektów z kolizjami = 20 osobnych synchronizacji.

**UI:** Przycisk "Reassign ID" z dialogiem podpowiadającym wolne ID.
**Wymaga write mode** (eksport zapisuje do XML).

### 3.3.3. R3 — S3 → Use Server ID

```
Projekt: ID: DR.L.01.01    Name: "Brzoza pożyteczna 'Doorenbos'"
Serwer:  ID: DRZ.L.01.01   Name: "Brzoza pożyteczna 'Doorenbos'"
```

1. **Zmień ID w projekcie** z DR.L.01.01 na DRZ.L.01.01.
   Elementy zachowują powiązanie (GUID się nie zmienia).

**UI:** Przycisk "Use Server ID".
**Nie wymaga write mode** (zmiana tylko w projekcie).

### 3.3.4. R4/R5 — S4/S5 → Export / Import

Bez zmian — obecne przyciski Export i Import działają poprawnie.

### 3.3.5. R6 — S6 → Use Server ID + Import

```
Projekt: ID: DRZ.L.01.05   Name: "Klon pospolity 'Globosum'"
Serwer:  ID: DRZ.L.01.05   Name: "Klon jawor"                ← kolizja ID
Serwer:  ID: DRZ.L.01.08   Name: "Klon pospolity 'Globosum'"  ← match Name
```

1. **Zmień ID w projekcie** .05 → .08 (bo serwer ma tę roślinę pod .08).
2. **Importuj** .05 "Klon jawor" z serwera do projektu.

**Nie wymaga write mode** (obie operacje w projekcie).

### 3.3.6. R7 — H1–H4 → Fix ID cascade

```
Serwer:  DRZ > DRZ.L > DRZ.L.01 > DRZ.L.01.01 ...  (100+ wpisów)
Projekt: DR  > DR.L  > DR.L.01  > DR.L.01.01  ...   (100+ wpisów)
```

1. Wykryj rozbieżność na najwyższym poziomie (DR vs DRZ, match po Name).
2. **Zmień ID rodzica** w projekcie (DR → DRZ).
3. **Zmień ID wszystkich potomków** rekurencyjnie.

**UI:** Jeden wpis "ID cascade: DR → DRZ (N wpisów)" z przyciskiem
"Fix ID cascade". Nie pokazuj potomków jako osobnych różnic.

**Nie wymaga write mode** (zmiana tylko w projekcie).

**UWAGA:** Przed kaskadą sprawdzić czy nie powstanie duplikat ID
z wpisem który już istnieje w projekcie pod nowym prefiksem.

### 3.3.7. R8 — H5 → utwórz brakujący poziom + przenieś potomków

Złożoność wysoka (delete + create, bo API nie ma "move").
Wymaga ręcznej decyzji użytkownika.

### 3.3.8. R9 — H6 → utwórz pod właściwym rodzicem + przepnij elementy + usuń stary

Złożoność wysoka, wymaga ręcznej interwencji.

## 3.4. Nowe funkcje ClassSync

| Funkcja | Przypadki | Opis |
|---------|-----------|------|
| Use Server ID | S3, S6 | Zmień ID wpisu w projekcie na wersję z serwera |
| Reassign ID | S2 | Nadaj wpisowi nowy unikalny ID (sprawdzając obie strony) |
| Fix ID cascade | H1–H4 | Zmień prefiks ID całej gałęzi w projekcie |

### 3.4.1. Masowa korekta kategorii (Fix ID cascade)

Przypadki H1–H4 (złamane konwencje ID, np. DR zamiast DRZ) dotyczą
potencjalnie 100+ wpisów w jednej gałęzi. Ręczne poprawianie każdego
wpisu z osobna byłoby niewykonalne.

**Fix ID cascade** to narzędzie do masowej korekty — jednym kliknięciem
zmienia prefiks ID rodzica i wszystkich jego potomków w projekcie,
dopasowując je do konwencji serwera.

Przykład:
```
Przed:  DR > DR.L > DR.L.01 > DR.L.01.01 ... (100+ wpisów)
Po:     DRZ > DRZ.L > DRZ.L.01 > DRZ.L.01.01 ... (te same rośliny, poprawione ID)
```

**Działa tylko po stronie projektu** — serwer nie jest modyfikowany.
Każdy projekt z błędną konwencją trzeba otworzyć osobno i wykonać
masową korektę. ClassSync wykrywa te rozbieżności automatycznie
i grupuje je jako jeden wpis "ID cascade" z liczbą dotkniętych wpisów.

**Przed wykonaniem** ClassSync sprawdza czy zmiana prefiksu nie
spowoduje kolizji z wpisami które już istnieją pod nowym prefiksem.

## 3.5. UI — propozycja panelu Differences

```
Differences:
  ID Collisions (3)            ← S2: to samo ID, różne rośliny
  ID Mismatches (5)            ← S3: ta sama nazwa, inne ID
  ID Cascades (2)              ← H1–H4: gałąź z innym prefiksem
  Only in Project (12)         ← S4: nowe, brak w XML
  Only on Server (8)           ← S5: nowe, brak w projekcie
```

---

# 4. POTRZEBNE DECYZJE

## 4.1. Zakres problemu

4.1.1. **Które przypadki faktycznie występują w Waszych projektach?**
(S2, S3, H1–H4 — które widzieliście?)

4.1.2. **Czy H1–H4 (rozbieżność ID kategorii/podkategorii) to główny problem?**
Ile projektów ma złamane konwencje ID?

4.1.3. **Czy nazwy roślin (Name) są stabilne między projektami?**
Czy ta sama roślina ma zawsze taką samą nazwę, a różnią się tylko ID?

4.1.4. **Czy zdarza się że ta sama roślina ma ZARÓWNO inne ID JAK I inną nazwę?**
(wtedy automatyczne dopasowanie jest niemożliwe)

## 4.2. Zasady

4.2.1. **Czy zgadzacie się z zasadą "serwer = append-only"?**
ClassSync nigdy nie modyfikuje istniejących wpisów w XML,
korekty tylko po stronie projektu.

4.2.2. **Czy dopuszczacie usuwanie wpisów z projektu** jeżeli wpis jest
duplikatem i nie jest przypisany do żadnego elementu?

## 4.3. Priorytet

4.3.1. **Kolejność wdrażania?**
Sugerujemy: S4/S5 (już działają) → S3/H1 (najczęstsze?) → S2 → reszta.

4.3.2. **Czy H5 (brakujący poziom) i H6 (zły rodzic) w ogóle występują?**
Jeśli nie — pomijamy w implementacji.
