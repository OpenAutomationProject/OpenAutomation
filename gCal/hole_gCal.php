#!/usr/bin/php
<?php
# Holt aus den Google Kalender (Schaltkalender) 20 Termine
# (es werden nur Termine aus der Zukunft geladen)
# und speichert sie in die plugin.conf ab,
# wird von google_schaltuhr Plugin alle 10min aufgerufen 
# Die $feedURL muss an den eigenen Kalender angepasst werden
# by NetFritz 03/2013
#
# $feedURL = "https://www.google.com/calendar/feeds/$userid/private-$magicCookie/full";
# Privatadresse unter Kalender-Einstellungen , unten XML Private, basic muss gegen full getausch werden
$feedURL = "https://www.google.com/calendar/feeds/myhouse544%40gmail.com/private-171a1c5c824ed7f07cda35b63852e31e/full";
$feedParams = "?singleevents=true&max-results=20&orderby=starttime&start-min=".urlencode(date("c"))."&sortorder=a";
$sxml = simplexml_load_file($feedURL.$feedParams);
# echo $sxml;
$today = 0;
$date = "";
foreach ($sxml->entry as $entry) {
    $title = stripslashes(utf8_decode($entry->title));
    $gd = $entry->children('http://schemas.google.com/g/2005');

    $startTime = '';
    if ( $gd->when ) {
        $startTime = $gd->when->attributes()->startTime;
    } elseif ( $gd->recurrence ) {
        $startTime = $gd->recurrence->when->attributes()->startTime;
    }
    $startTime = strtotime( $startTime );
    
    if(date("d.m.y") == date("d.m.y", $startTime))
    {
        $today++;
    } else {
       if($today > 0)
       {
          $today = 0;
          $date .= "\n";
       }
    }
    
    ###try End
    
    $endTime = '';
    if ( $gd->when ) {
        $endTime = $gd->when->attributes()->endTime;
    } elseif ( $gd->recurrence ) {
        $startTime = $gd->recurrence->when->attributes()->endTime;
    }
    $endTime = strtotime( $endTime );
    
    if(date("d.m.y") == date("d.m.y", $endTime))
    {
        $today++;
    } else {
       if($today > 0)
       {
          $today = 0;
          $date .= "\n";
       }
    }


    $where = utf8_decode($gd->where->attributes()->valueString);
    if(strlen($where) > 0) {
        #$where = " (".$where.")";
        $where = $where;
    }
    
    list ($ga, $dpt, $on, $off, $extra) = split(";", $where);
    #echo "Gruppenadresse $ga DPT $dpt AN $on AUS $off";
    
    $date .= date("d.m.Y H:i", $startTime ).";".$title.";".$ga.";".$dpt.";".$on."\n";
    
    if (isset($off)) {
    $date .= date("d.m.Y H:i", $endTime ).";".$title.";".$ga.";".$dpt.";".$off."\n";
    }

}

#echo $date;
# schreibt die Schalttermine in die plugin.conf
$conf="/etc/wiregate/plugin/generic/conf.d/google_schaltuhr.conf";
$datei = fopen($conf,"w");
  fwrite($datei, $date);
fclose($datei);

?>