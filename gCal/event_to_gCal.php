<?php
# benötigte Funktionen/Klassen
# http://framework.zend.com/manual/1.12/de/zend.gdata.html
# https://packages.zendframework.com/releases/ZendGdata-1.12.3/ZendGdata-1.12.3.zip
# herunterladen entpacken und das Verzeichnis Zend was unter library liegt hier ablegen
#
require_once 'Zend/Loader.php';
Zend_Loader::loadClass('Zend_Gdata');
Zend_Loader::loadClass('Zend_Gdata_AuthSub');
Zend_Loader::loadClass('Zend_Gdata_ClientLogin');
Zend_Loader::loadClass('Zend_Gdata_Calendar');
#
    // Parameter für die ClientAuth Authentifizierung
    $service = Zend_Gdata_Calendar::AUTH_SERVICE_NAME;
#------- muss angepasst werden ----------------------------	
    $user = "myhouse544@gmail.com"; # gmail Adresse
    $pass = "fidi_bums";            # Passwort
# ---------------------------------------------------------
#
# POST Variable auslesen und aufbereiten
$time_start = $_GET["time_start"];
$time_end = $_GET["time_end"];
$title = $_GET["title"];
# 
$str_date = substr($time_start,0,16);
$date = date_parse($str_date);
$startDate = $date["year"] . "-" . str_pad($date["month"], 2 ,'0', STR_PAD_LEFT) . "-" . str_pad($date["day"], 2 ,'0', STR_PAD_LEFT);

$startTime = trim(substr($time_start,16,5));

#
$str_date = substr($time_end,0,16);
$date = date_parse($str_date);
$endDate = $date["year"] . "-" . str_pad($date["month"], 2 ,'0', STR_PAD_LEFT) . "-" . str_pad($date["day"], 2 ,'0', STR_PAD_LEFT);

$endTime = trim(substr($time_end,16,5));

#
    list ($name, $ga, $dpt, $on, $off, $extra) = split(";", $title);
#

     
    // Erstellt einen authentifizierten HTTP Client
    $client = Zend_Gdata_ClientLogin::getHttpClient($user, $pass, $service);     
    // Erstellt eine Instanz des Kalender Services
    $service = new Zend_Gdata_Calendar($client);
    // Erstellt einen neuen Eintrag und verwendet die magische Factory
    // Methode vom Kalender Service
    $event= $service->newEventEntry();     
    // Gibt das Event bekannt mit den gewünschten Informationen
    // Beachte das jedes Attribu als Instanz der zugehörenden Klasse erstellt wird
    $event->title = $service->newTitle($name);
    $event->where = array($service->newWhere($ga .";". $dpt .";". $on .";". $off .";". $extra)); # Wo
    $event->content = $service->newContent("2"); # Beschreibung    
    // Setze das Datum und verwende das RFC 3339 Format.
    # $startDate = "2013-03-21";
    # $startTime = "10:00";
	#------------ endDate endTime 1Std -----------------------
    #	list($year, $month, $day, ) = split('[-]', $startDate); 
    #	list($hour, $minute) = split('[:]', $startTime); 
    #	$timestamp=mktime($hour, $minute,0, $month, $day, $year);
    #	$timestamp = $timestamp + 3600;
    #	$endDate = date("Y-m-d", $timestamp);
    #	$endTime = date("H:i", $timestamp);
	#--------------------------------------------------------
    # $endDate = "2013-03-22";
    # $endTime = "11:00";
    $tzOffset = "+01";     
    $when = $service->newWhen();
    $when->startTime = "{$startDate}T{$startTime}:00.000{$tzOffset}:00";
    $when->endTime = "{$endDate}T{$endTime}:00.000{$tzOffset}:00";
    $event->when = array($when);     
    // Das Event an den Kalender Server hochladen
    // Eine Kopie des Events wird zurückgegeben wenn es am Server gespeichert wird
    $newEvent = $service->insertEvent($event);
?> 