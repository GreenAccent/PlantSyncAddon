# TODO - PlantSync Add-On

## Faza 0: Przygotowanie srodowiska
- [ ] Sprawdzic czy toolset v142 jest dostepny w VS 2022 (lub zdecydowac o v143)
- [ ] Przetestowac budowanie przykladowego add-onu z DevKit (np. Classification_Test)
- [ ] Potwierdzic ze .apx laduje sie w ArchiCAD 28

## Faza 1: Minimalny add-on (Hello World)
- [ ] Stworzyc CMakeLists.txt (standalone, wskazujacy na DevKit 28.4001)
- [ ] Stworzyc minimalne pliki zrodlowe (APIEnvir.h, PlantSync.cpp z 4 funkcjami)
- [ ] Stworzyc zasoby .grc (MDID, stringi, jedno menu)
- [ ] Stworzyc .rc2
- [ ] Zbudowac i zaladowac w ArchiCAD - potwierdzic ze menu sie pojawia
- [ ] Menu handler: WriteReport("Hello from PlantSync!") - potwierdzic dzialanie

## Faza 2: Odczyt klasyfikacji (C++ API)
- [ ] Czytanie systemow klasyfikacji z projektu (ACAPI_Classification_GetClassificationSystems)
- [ ] Czytanie drzewa itemow (GetClassificationSystemRootItems + GetClassificationItemChildren)
- [ ] Wyswietlanie drzewa w raporcie (ACAPI_WriteReport) lub alercie (DGAlert)
- [ ] Znalezienie systemu "Green Accent PLANTS" po nazwie

## Faza 3: Import XML
- [ ] Wczytanie pliku XML z dysku (dialog Open File lub hardcoded sciezka)
- [ ] Uzycie ACAPI_Classification_Import() z xmlString
- [ ] Testowanie z master XML (Green Accent PLANTS.xml)
- [ ] Obsluga konfliktow (Replace vs Skip vs Merge)

## Faza 4: Okno dialogowe
- [ ] Zdefiniowac dialog w .grc (GDLG + DLGH)
- [ ] TreeView do wyswietlania drzewa klasyfikacji
- [ ] Przyciski: Import, Export, Diff, Zamknij
- [ ] Wypelnianie TreeView danymi z ACAPI_Classification_*
- [ ] Kolorowanie/oznaczanie roznic (nowe, zmienione, brakujace)

## Faza 5: Export i Diff
- [ ] Export drzewa klasyfikacji do formatu XML ArchiCAD
- [ ] Porownanie drzewa projektu z master XML (diff)
- [ ] Wyswietlanie roznic w dialogu (status ikony w TreeView)

## Faza 6: Pelna synchronizacja
- [ ] Selektywny import (wybor co importowac z mastera)
- [ ] Selektywny export (wybor co dodac do mastera)
- [ ] Obsluga properties (import/export definicji properties)
- [ ] Obsluga linkowania properties do klasyfikacji (availability)

## Faza 7: Paleta (opcjonalnie)
- [ ] Zamiana dialogu modalnego na palette (okno plywajace)
- [ ] Rejestracja jako modeless window
- [ ] Auto-odswiezanie po zmianach w projekcie (obserwatory)

## Znane wyzwania
- Toolset v142 vs v143 - moze wymagac doinstalowania v142
- ID klasyfikacji nie sa unikalne miedzy projektami - matchowanie po atrybutach
- Duze drzewo (~489 itemow) - wydajnosc TreeView
- Polskie znaki (UTF-8 vs UniString)
- Enum properties maja GUID jako klucz - rozne GUID w roznych projektach
- ACAPI_Classification_Import() przyjmuje GS::UniString, nie const char*
