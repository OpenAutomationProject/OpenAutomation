Google-Schaltkalender f�r das Wiregate 
by NetFritz 04/2013
Der Google-Schaltkalender besteht aus 2 Teilen.
Einmal aus dem google-schaltuhr.pl Plugin mit
dem PHP-Schript hole_gCal.php
und aus dem 2.Teil zum eintagen von Schaltterminen

1.Teil

Das Plugin muss unter Plugins auf den WG angelegt werden
google_schaltuhr.pl

Unter Google Kalender muss ein neuer Kalender z.B. Schaltungen erstellt werden.
In diesem Kalender werden die Schalttermine so eingetragen.

Termin-Name: Test
Start: 30.3.2013 12:05
Ende: 30.3.2013 12:06
Wo: 1/2/10;1;1;0

GrpAdr = 1/2/10
DPT = 1
Wert zum Anfang = 1
Wert zum Ende = 0

ergibt folgende Schaltzeiten im Plugin:

30.03.2013 12:05;Test;1/2/10;1;1
30.03.2013 12:06;Test;1/2/10;1;0

Der Ausschaltzeitpunkt ist optional.

Wenn man nur eine Schalthandlung haben will l�sst man den Parameter einfach weg:

1/2/10;1;1;0 = Einschalten zum Terminanfang und Ausschalten zum Terminende
1/2/10;1;1 = Nur Einschalten zum Terminanfang
1/2/10;1;0 = Nur Ausschalten zum Terminanfang


Das Plugin �berpr�ft alle 20sec. ob eine Schaltung ansteht und f�hrt sie dann aus.
Es wird alle 10min ein PHP-Script augerufen, das holt 20 aktuelle Termine aus dem 
Google-Kalender und speichtert diese Termine in der google_schaltuhr.conf ab.

Das Sript kann z.Z. nur hier runtergeladen werden.
Beitrag 11 von JuMi2006
http://knx-user-forum.de/code-schnipsel/26061-google-schaltuhr-2.html

Ich habe dieses PHP-Script auf meinem WG unter /var/www/myhouse/gCal/hole_gCal.php abgelegt.
Dieser Speicherort muss im Plugin google_schaltuhr.pl ewtl. angepasst werden.

Im PHP-Script muss noch die Privatadresse des Kalenders unter "$feedURL" angepasst werden.
Diese Adresse bekommt man wenn man im Google Kalender auf der linken Seite den Schaltkalender anklickt.
Dann �ffnet eine Drop Down Liste, wo man Kalender-Einstellungen ausw�hlt.
Dort kommt man zu den Kalender Details.
Ganz unten unter Privatadresse den Button "XML" anklicken , dann �ffnet sich ein Popup mit der Privatadresse.
Diese Privatadresse kopiert man ins PHP-Script unter "$feedURL" 
https://www.google.com/calendar/feeds/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxgroup.calendar.google.com/private-xxxxxxxxxxxxxxxxxxxxxxxxxxxxx/basic
Die Endung "basic" wird entfernt und durch "full" ersetzt.

Das PHP-Script k�nnte auch durch Perl-Script ersetzt werden.
Es k�nnte dann in das Plugin �bernommen werden.
Ich bin schon damit angefangen, habe aber die Lust verloren.

---------------------------------------------------------------------------
2.Teil

Da das ablegen der Termine Fehlertr�chtig ist habe ich auch eine Webseite mit einer Eingabemaske erstellt

Auf dieser Webseite index.php muss auch noch die Adresse vom Google-Schaltkalemder eingetragen werden.

Diese Adresse bekommt man wenn man im Google Kalender auf der linken Seite den Schaltkalender anklickt.
Dann �ffnet eine Drop Down Liste, wo man Kalender-Einstellungen ausw�hlt.
Dort kommt man zu den Kalender Details.

Dort wird unter "Diesen Kalender einbetten" der Code f�r das einbetten des IFRAMEs angezeigt.

Das kopiert man in die index.php und �ndert "wigth" und "height" so ab wie in der index.php.

Zu testen der Webseite kann man sie dann aufrufen.
http://wiregatexxx/myhouse/gCal/index.php

Als erstes muss man in der Eingabemaske unten den Button lade eibga anklicken.
Dadurch werden aus der eibga.db die GA Namen GrAdr und DPT geholt und in die Datei eibga.js abgelegt.

Dann kann man im Feld Name;GrpAdr,Dpt; auswaehlen sich die GA raussuchen
die man schalten m�chte.

Hat man alle Felder ausgew�hlt sendet man mit dem Button SAVE Termin,
diesen Termin zum Google Schaltkalender.

F�r die event_to_gCal.php muss man noch folgendes installieren
ben�tigte Funktionen/Klassen
http://framework.zend.com/manual/1.12/de/zend.gdata.html
Downloaden und entpacken https://packages.zendframework.com/releases/ZendGdata-1.12.3/ZendGdata-1.12.3.zip
Den Inhalt vom Verzeichnis Zend welches unter library liegt hier unter Zend ablegen.


Viel Spass
Gruss NetFritz


 