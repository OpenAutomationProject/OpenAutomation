Features:
- Copy&Paste
- raw/base64 senden
- gr�sstenteils GUI-konform, Plattform-portabel (Win,*nix,OSX,Maemo etc.)
- schneller
- CSV-Log

Bugs:
- FIXED: Kein automatischer reconnect
- zu langsam/resourcenintensiv
- progress-bar hat eher dekortiven charakter
- sortieren nach "Log" geht nicht (richtig)
- Liveupdate von sortierung/Filter teilw. fehlerhaft (Performance/konfigurierbar?)
- Sortieren nach ver�nderbaren Werten (Count/Time/Value/Length) prinzipbedingt langsam
- Anzeige unsch�n bei Wert mit Zeilenumbr�chen (\n)
- senden von raw 0x00 (geht by Design nicht weil das das Trennzeichen f�r HS-KO ist)
- Editieren der Werte in der Anzeige - geht, sollte nat�rlich nicht so sein, hat aber keine Auswirkungen
- selektieren der Werte zum loggen geht nicht mit <Leertaste>
- das Live-Log ist nicht limitiert und wird wohl seine (Speicher)grenzen haben; ->abschalten zum Langzeit-protokollieren
- ordentliche Fehlermeldung bei Verbindungsproblemen
- Anzeige/Eingabe Dezimaltrenner (.,) gem�ss locale

Missing:
- Step+-/List+-
- Multilingual, wurde in Englisch gehalten, weil es banal ist..
- Log XML

Changelog:
0.92 - 2010-03-12
- Fix erlaubte l�nge 14Byte/iKO

0.93 - 2010-03-13
- Ausgabe detailierter XML-Parserfehler
- Richtige Sortierung nach count
- Toolbar-Button/Men� zum toggeln der Log-Auswahl (+ Strg+A)

0.94 - 2010-05-02
- XML-parserfehler ignorieren, nur Messagebox-Popup
- Layout�nderungen der DockWidgets werden vollst�ndig gespeichert&wieder geladen
- Wert wird mit RETURN im Value-Feld sofort gesendet
- Neues Feld Logfilter - Workaround zum filtern selektierter Zeilen
