Features:
- Copy&Paste
- raw/base64 senden
- grösstenteils GUI-konform, Plattform-portabel (Win,*nix,OSX,Maemo etc.)
- schneller
- CSV-Log

Bugs:
- FIXED: Kein automatischer reconnect
- zu langsam/resourcenintensiv
- progress-bar hat eher dekortiven charakter
- sortieren nach "Log" geht nicht (richtig)
- Liveupdate von sortierung/Filter teilw. fehlerhaft (Performance/konfigurierbar?)
- Sortieren nach veränderbaren Werten (Count/Time/Value/Length) prinzipbedingt langsam
- Anzeige unschön bei Wert mit Zeilenumbrüchen (\n)
- senden von raw 0x00 (geht by Design nicht weil das das Trennzeichen für HS-KO ist)
- Editieren der Werte in der Anzeige - geht, sollte natürlich nicht so sein, hat aber keine Auswirkungen
- selektieren der Werte zum loggen geht nicht mit <Leertaste>
- das Live-Log ist nicht limitiert und wird wohl seine (Speicher)grenzen haben; ->abschalten zum Langzeit-protokollieren
- ordentliche Fehlermeldung bei Verbindungsproblemen
- Anzeige/Eingabe Dezimaltrenner (.,) gemäss locale

Missing:
- Step+-/List+-
- Multilingual, wurde in Englisch gehalten, weil es banal ist..
- Log XML

Changelog:
0.92 - 2010-03-12
- Fix erlaubte länge 14Byte/iKO

0.93 - 2010-03-13
- Ausgabe detailierter XML-Parserfehler
- Richtige Sortierung nach count
- Toolbar-Button/Menü zum toggeln der Log-Auswahl (+ Strg+A)

0.94 - 2010-05-02
- XML-parserfehler ignorieren, nur Messagebox-Popup
- Layoutänderungen der DockWidgets werden vollständig gespeichert&wieder geladen
- Wert wird mit RETURN im Value-Feld sofort gesendet
- Neues Feld Logfilter - Workaround zum filtern selektierter Zeilen
