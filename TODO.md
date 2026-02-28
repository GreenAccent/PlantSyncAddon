# TODO - ClassSync Add-On

## Zrealizowane
- [x] Toolset v142 zainstalowany w VS 2022
- [x] Minimalny add-on (PlantSync) - buduje sie, laduje w ArchiCAD 28
- [x] MDID zarejestrowany (860318800, 1954174874)
- [x] Odczyt klasyfikacji z projektu (ACAPI)
- [x] Modal dialog z TreeView - dziala
- [x] deploy.cmd z UAC (czeka na zamkniecie ArchiCAD)
- [x] Rename PlantSync -> ClassSync
- [x] Modeless palette (DG::Palette) z 3 panelami
- [x] Parsowanie XML z dysku (XmlReader)
- [x] Algorytm diff (porownanie projekt vs serwer)
- [x] Kolorowanie diff w TreeViews (Gfx::Color)
- [x] Panel Conflicts (srodkowy) z podsumowaniem
- [x] Diagnostyka ACAPI_WriteReport w XML readerze i RefreshData
- [x] **v0.4**: Numer wersji w palecie (label + logi)
- [x] **v0.4**: File picker (Browse) + preferencje ACAPI (sciezka XML)
- [x] **v0.4**: Import z serwera (ACAPI_Classification_Import, merge+skip)
- [x] **v0.4**: Export do serwera (XmlWriter - AddItemToXml)
- [x] **v0.4**: Resolve Conflicts (Use Project / Use Server)
- [x] **v0.4**: Kolory jak diff (zielony=nowe, niebieski=brakujace, ceglasty=konflikt)
- [x] **v0.4**: Fix redraw (tree.Redraw() po EnableDraw)
- [x] **v0.4**: Fix label "Name Conflicts" -> "Conflicts"
- [x] **v0.4**: Aktywacja przyciskow akcji na podstawie selekcji w drzewku

## Do zrobienia (v0.5)
- [x] **Refresh przy otwarciu** - RefreshData() przy kazdym otwarciu palety (Show po Hide)
- [ ] **Edytor obiektow (panel properties)** - po zaznaczeniu itemu w drzewku Project lub Server wyswietlic jego wlasciwosci w panelu edycyjnym; mozliwosc recznej edycji; przy zaznaczeniu konfliktu (srodkowe drzewko) edytor nieaktywny (dotyczy dwoch obiektow)
- [ ] **Resize + zapamietywanie rozmiaru** - Palette | grow, proporcjonalny 3-kolumnowy layout, rozmiar zapisywany w preferencjach
- [x] **Status/postep wczytywania** - label statusu ("Reading project...", "Reading XML...", "Comparing...", "Updating trees...")
- [x] **Rename na ClassSync** - pliki zasobow, referencje, deploy.cmd, README, docs
- [x] **Menu: "ClassSync" > "Sync"** - menu "ClassSync" z opcja "Sync"

## Do zrobienia (v0.5 - krytyczne)
- [ ] **Walidacja XML** - sprawdzanie poprawnosci struktury XML przed obrobka; brak oficjalnej schemy ArchiCAD - trzeba napisac wlasna walidacje (wymagane tagi: BuildingInformation/Classification/System/Items/Item, wymagane pola Item: ID/Name/Description/Children, poprawnosc hierarchii, formaty ID); walidacja przed importem i po eksporcie
- [ ] **Blokowanie XML (locking)** - mechanizm blokad zapobiegajacy jednoczesnej edycji tego samego XML przez wielu uzytkownikow; granularnosc do ustalenia: caly plik, fragment drzewa (galaz), lub pojedynczy obiekt; moze wybierany przez usera; plik lockowy obok XML lub sekcja w samym XML; musi obslugiwac crash recovery (stale locki)
- [ ] **Changelog / sledzenie zmian** - dodatek generuje log zmian: kto edytowal, co zmienil, kiedy; format do ustalenia (osobny plik changelog obok XML, lub sekcja w XML); identyfikacja usera (nazwa komputera / login Windows / konfigurowalny nick)

## Do zrobienia (v0.5 - UX)
- [ ] **Synchronizacja selekcji** - klik na konflikcie (srodkowe drzewko) automatycznie przechodzi do odpowiedniego itemu w drzewku Project i Server, zeby nie trzeba bylo szukac reczne

## Do zrobienia (pozniej)
- [x] Menu checkmark (zaznaczenie gdy paleta otwarta)
- [ ] Obsluga properties (import/export definicji)
- [ ] Auto-odswiezanie po zmianach w projekcie (obserwatory notyfikacji)
- [ ] SVN integration (zamiast statycznej sciezki do pliku)
- [ ] Bulk export (Export All - eksport wszystkich brakujacych naraz)
- [ ] Bulk import wybranych (import pojedynczego itemu zamiast calego XML)

## Znane wyzwania
- ID klasyfikacji nie sa unikalne miedzy projektami - matchowanie po ID string
- Polskie znaki w XML (UTF-8 -> GS::UniString via CC_UTF8)
- Enum properties maja GUID jako klucz - rozne GUID w roznych projektach
- ACAPI_Classification_Import() przyjmuje GS::UniString, nie const char*
- DG::Palette wymaga C++ observer pattern (nie C-style callback)
- Export: parentId okreslany przez obcinanie ostatniego segmentu ID (np. DRZ.L.01 -> DRZ.L)
- CMake GLOB wymaga re-konfiguracji po dodaniu nowych plikow (cmake .. -G ...)
