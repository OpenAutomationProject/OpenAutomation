#!/usr/bin/php
<?php
# holt aus der eibga.conf die GAs mit Namen, GrAdr und Dtp
# und legt sie als Array in eibga.js ab
# braucht nur zu install oder bei aenderung der eibga.conf aufgerufen werden 
$zeilen = file ('/etc/wiregate/eibga.conf');
echo "zeilen=" . count($zeilen) . "\n";
$ii = 0;
$neu_str[$ii++] = "var eibga = {};"; 
$neu_str[$ii++] = "eibga.results = [";
for($i=0; $i < count($zeilen); $i++){
    # echo $zeilen[$i];
	$first = substr($zeilen[$i], 0,1);
    $last  = substr($zeilen[$i],-1,1);		
	if($first == '[' && $last = ']'){
	   $zeilen[$i] = str_replace(array("[", "]", "\r\n", "\r", "\n"), '', $zeilen[$i]);
	   $gradr = $zeilen[$i];
	   $zeilen[$i+2] = str_replace(array("\r\n", "\r", "\n"), '', $zeilen[$i+2]);
	   $teile = explode("=", $zeilen[$i+2]);
	   if($teile[0] == 'DPTSubId'){
		$dpt = $teile[1];
	   }
	   $zeilen[$i+4] = str_replace(array("\r\n", "\r", "\n"), '', $zeilen[$i+4]);
	   $teile = explode("=", $zeilen[$i+4]);
	   if($teile[0] == 'name'){
		$name = $teile[1];
	   }
	   $ii++;
       $neu_str[$ii] = "{id:" . "'" . $ii . "',name:'" . $name . " ; " . $gradr  . " ; " . $dpt . " ;'},";
	}
}	
$neu_str[$ii] = substr($neu_str[$ii], 0, -1); # letztes Zeichen entfernen
$ii++;
$neu_str[$ii] = "];";
$ii++;
$neu_str[$ii] = "eibga.total = eibga.results.length;";
# print_r($neu_str);
#
$fp = fopen('eibga.js', 'w');
foreach($neu_str as $values) fputs($fp, $values."\n");
fclose($fp);
?>