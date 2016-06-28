<?php
/**
* Anpassung an mehrere googlecalendar zum Darstellen der events in der cometvisu
* parst die ics-Versionen der Kalender
* PHP Version 5
* based on: ics-parser ->
* @author Martin Thoma <info@martin-thoma.de>
* @license http://www.opensource.org/licenses/mit-license.php MIT License
* @link http://code.google.com/p/ics-parser/
* @example $ical = new ical('MyCal.ics');
* print_r( $ical->get_event_array() );
*/

require 'class.iCalReader.php';

// Hier die GoogleKalender eintragen
// ReadCalendar(Userid, MagicCookie, Tage die abgerufen werden, ShortCut für farbige Sortierung)
ReadCalendar('xxxxxxxxxxxxxxxxxxxxxxxxxx@group.calendar.google.com', 'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx', 7, '1'); 	# Blau - xxxx ersetzen
ReadCalendar('yyyyyyyyyyyyyyyyyyyyyyyyyy@group.calendar.google.com', 'yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy', 7, '3'); 	# Rot - yyyy ersetzen
ReadCalendar('rr7mkkascn405ksj67ttoh524aioaijm@import.calendar.google.com', '', 7, '5'); 	# Feiertage Sachsen

//Hier die Farbe für die ShortCuts angeben
$SC_Farbe[1] = "#6E6E6E"; # grau
$SC_Farbe[2] = "#FF8000"; # orange
$SC_Farbe[3] = "#CD0000"; # dunkelrot
$SC_Farbe[4] = "#6CA602"; # gruen
$SC_Farbe[5] = "#BFEFFF"; # hellblau

//Hier den Style wählen
$blinkToday = false; // (true/false) blinken des heutigen Kalendereintrages An/Aus

/******** Farbnamen, RGB-Formatierung, Hex-Zahlen möglich *********/
$StyleText[1] = "#E6E6E6"; // Textfarbe Datum
$StyleText[2] = "grey"; // Textfarbe Wochentag

/*** xx-small, x-small, small, normal, large, x-large, xx-large ***/
/************** oder Angabe in pt - z.B. "12pt" *******************/
$StyleText[3] = "5.5mm"; // Textgröße Datum
$StyleText[4] = "5.5mm"; // Textgröße Wochentag
$StyleText[5] = "5.5mm"; // Textgröße Eintrag



/****************** Ab hier wieder die Finger lassen **************/
// Wochentage auf Deutsch
$tag[0] = "Sonntag";
$tag[1] = "Montag";
$tag[2] = "Dienstag";
$tag[3] = "Mittwoch";
$tag[4] = "Donnerstag";
$tag[5] = "Freitag";
$tag[6] = "Samstag";
$deco = "";
if($blinkToday)$deco = "blink";


function DateCompare($a, $b) {
    if ( $a['Datum'] == $b['Datum'] ) return 0;
    if ( $a['Datum'] < $b['Datum'] )  return -1;
    return 1;
}

function ReadCalendar($userid, $magicCookie, $maxDays, $shortCut) {
    global $calcData;
   
    if ($magicCookie != '') {$feedURL = "https://www.google.com/calendar/ical/$userid/private-$magicCookie/basic.ics";}
    else                     $feedURL = "https://www.google.com/calendar/ical/$userid/public/basic.ics";

    $date = "";

    $ical = new ICal($feedURL);
    $events = $ical->eventsFromRange(strtotime('NOW'),strtotime('+'.$maxDays.' day'));
    
    foreach ($events as $event) {
        $title = $event['SUMMARY'];
        $startTime = '';
        $endTime = '';

        $startTime = $event['DTSTART'];
        $endTime = $event['DTEND'];

        $startTime = strtotime( $startTime );
        $endTime = strtotime( $endTime );
        $thisData['Datum'] = $startTime;

        //$where = $event['LOCATION'];
        //if(strlen($where) > 0){$where = " (".$where.")";}
		$where = "";

        $thisData['DatumTxt'] = date("d.m.Y", $startTime);
        $thisData['EndDatumTxt'] = date("d.m.Y", $endTime);
        $thisData['ZeitTxt'] = date("H:i", $startTime);
        $thisData['EndZeitTxt'] = date("H:i", $endTime);
        $thisData['shortCut'] = $shortCut;
        $thisData['Bezeichnung'] = $title.$where;
        if($thisData['ZeitTxt'] == "00:00" && $thisData['EndZeitTxt'] == "00:00")
        {
		$thisData['EndDatumTxt'] = date("d.m.Y", strtotime($thisData['EndDatumTxt']."-1 day"));
		}


        $calcData[] = $thisData;
    }
}

if($calcData != ''){usort($calcData, 'DateCompare');

$calDataTxt = "<table style='border-spacing:0px; width:100%; font-family: 'Dosis', sans-serif'>"; // Starte Tabellenansicht
$check_date = "";

foreach($calcData as $thisData)
{
    if($thisData['EndDatumTxt'] != date("d.m.Y", strtotime("yesterday")))
    {
        if($check_date != "" and $thisData['DatumTxt'] != $check_date)$calDataTxt .= "</table></tr>";
        if($thisData['DatumTxt'] != $check_date)
        {
            if($thisData['DatumTxt'] == date("d.m.Y"))$headerTxt = "<span style='font-weight:800;'>Heute:</span>";
            elseif($thisData['DatumTxt'] == date("d.m.Y", strtotime("+1 day")))$headerTxt = "Morgen:";
            else $headerTxt = $thisData['DatumTxt'].":";


            $calDataTxt .= "<tr><td style='padding4px;'>
                                    <span style='color:".$StyleText[1].";font-weight:200;font-size:".$StyleText[3]."'>"
                                    .$headerTxt.
                                    "</span></td><td>".
                            "<table style='border-spacing:0px; width:100%;padding:4px; border:0px solid #000; box-shadow: 0px 0px 0px #111;'>";
            $check_date = $thisData['DatumTxt'];
            $calDataTxt .= "<td style='float:left; padding-left:4px'>".SetEintrag($thisData)."&nbsp;&nbsp;<span style='color:silver;'>&#10032;</span></td>";
        }
        else
        {
            $calDataTxt .= "<td style='float:left; padding-left:4px'>".SetEintrag($thisData)."&nbsp;&nbsp;<span style='color:silver;'>&#10032;</span></td>";
        }
    }
}
$calDataTxt .= "</table></td></table>"; // Tabelle schließen
}
/*# Ausgabe als html ########################################################################################################################################################################################*/

echo "<html>
        <head>
            <link href='https://fonts.googleapis.com/css?family=Dosis:500' rel='stylesheet' type='text/css'>
            <meta name='language' content='de'>
            <meta http-equiv='content-type' content='text/html; charset=UTF-8'>
                <style>
                body 
                    {
                    font-family: 'Dosis', sans-serif;
                    font-size: 5.5mm;
                    text-shadow:   0   /*{a-body-shadow-x}*/   1px   /*{a-body-shadow-y}*/   1px   /*{a-body-shadow-radius}*/   #111   /*{a-body-shadow-color}*/;
                    }
                </style>
        </head>
        <body>".$calDataTxt."</body>
    </html>";

/*# Subroutinen ############################################################################################################################################################################################*/

function SetEintrag($thisData)
{
    global $tag, $SC_Farbe, $deco, $StyleText;
    if($thisData['ZeitTxt'] == "00:00")$thisData['ZeitTxt']="";
    else $thisData['ZeitTxt']= "(".$thisData['ZeitTxt']." Uhr)";

    if($thisData['DatumTxt'] == date("d.m.Y"))

    {
        return "<span style='font-weight:normal;font-size:".$StyleText[5].";text-decoration:".$deco.";;color:".$SC_Farbe[$thisData['shortCut']]."'>".$thisData['Bezeichnung']."&nbsp;&nbsp;".$thisData['ZeitTxt']."</span>";
    }
    else
    {
        return "<span style='font-weight:normal;font-size:".$StyleText[5].";color:".$SC_Farbe[$thisData['shortCut']]."'>".$thisData['Bezeichnung']."&nbsp;&nbsp;".$thisData['ZeitTxt']."</span>";
    }

}

?>
