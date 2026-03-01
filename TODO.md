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
- [ ] **Edytor obiektow (panel properties)** - WIP na branch `feature/editor`; po zaznaczeniu itemu w drzewku Project lub Server wyswietlic Name/Description w panelu edycyjnym; edycja + Save; przy zaznaczeniu konfliktu edytor nieaktywny
- [x] **Resize** - Palette | grow, proporcjonalny 3-kolumnowy layout z PanelResized handler
- [x] **Status/postep wczytywania** - label statusu ("Reading project...", "Reading XML...", "Comparing...", "Updating trees...")
- [x] **Rename na ClassSync** - pliki zasobow, referencje, deploy.cmd, README, docs
- [x] **Menu: "ClassSync" > "Sync"** - menu "ClassSync" z opcja "Sync"

## Do zrobienia (v0.5 - krytyczne)
- [ ] **Walidacja XML** - sprawdzanie poprawnosci struktury XML przed obrobka; brak oficjalnej schemy ArchiCAD - trzeba napisac wlasna walidacje (wymagane tagi: BuildingInformation/Classification/System/Items/Item, wymagane pola Item: ID/Name/Description/Children, poprawnosc hierarchii, formaty ID); walidacja przed importem i po eksporcie
- [x] **Blokowanie XML (locking)** - plik `.lock` obok XML z session=PID, przycisk "Open for write" / "Close write", auto-release przy zamknieciu palety/ArchiCAD, bezpieczne przy wielu instancjach AC
- [x] **Changelog / sledzenie zmian** - dzienne pliki `changelog/YYYY-MM-DD.txt` z wpisami kto/co/kiedy przy kazdej akcji sync

## Do zrobienia (v0.5 - UX)
- [x] **Synchronizacja selekcji** - klik na konflikcie (srodkowe drzewko) automatycznie zaznacza odpowiedni item w drzewku Project i Server

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
