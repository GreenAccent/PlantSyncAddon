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

## Do zrobienia
- [ ] Resize handling (grow) - proportional 3-column layout
- [ ] Menu checkmark (zaznaczenie gdy paleta otwarta)
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
