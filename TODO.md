# TODO - ClassSync Add-On

## Zrealizowane
- [x] Toolset v142 zainstalowany w VS 2022
- [x] Minimalny add-on (PlantSync) - buduje sie, laduje w ArchiCAD 28
- [x] MDID zarejestrowany (860318800, 1954174874)
- [x] Odczyt klasyfikacji z projektu (ACAPI)
- [x] Modal dialog z TreeView - dziala
- [x] deploy.cmd z UAC
- [x] Rename PlantSync -> ClassSync
- [x] Modeless palette (DG::Palette) z 3 panelami
- [x] Parsowanie XML z dysku (XmlReader)
- [x] Algorytm diff (porownanie projekt vs serwer)
- [x] Kolorowanie diff w TreeViews (Gfx::Color)
- [x] Panel Conflicts (srodkowy) z podsumowaniem

## Do zrobienia
- [ ] Zbudowac i przetestowac ClassSync (pierwsza kompilacja)
- [ ] Naprawic ewentualne bledy kompilacji
- [ ] Usunac stary PlantSync.apx z Dodatki
- [ ] Resize handling (grow) - proportional 3-column layout
- [ ] File picker dla sciezki XML (zamiast hardcoded)
- [ ] Menu checkmark (zaznaczenie gdy paleta otwarta)
- [ ] Import brakujacych klasyfikacji do projektu (ACAPI_Classification_Import)
- [ ] Export nowych klasyfikacji na serwer (zapis XML)
- [ ] Obsluga properties (import/export definicji)
- [ ] Auto-odswiezanie po zmianach w projekcie (obserwatory notyfikacji)
- [ ] SVN integration (zamiast statycznej sciezki do pliku)

## Znane wyzwania
- ID klasyfikacji nie sa unikalne miedzy projektami - matchowanie po ID string
- Polskie znaki w XML (UTF-8 -> GS::UniString via CC_UTF8)
- Enum properties maja GUID jako klucz - rozne GUID w roznych projektach
- ACAPI_Classification_Import() przyjmuje GS::UniString, nie const char*
- DG::Palette wymaga C++ observer pattern (nie C-style callback)
