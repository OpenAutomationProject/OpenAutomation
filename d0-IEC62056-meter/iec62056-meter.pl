#!/usr/bin/perl
# Zaehlerabfrage fuer Zaehler nach Protokoll IEC 62056-21 / OBIS
# Ein Anfrage-Telegramm ist mit 300 Baud, 7 Bit, 1 Stoppbit
# und gerader Paritaet zu senden. Das ist der Initialmodus von Geraeten,
# die das Protokoll IEC 62056-21 implementieren.
# Ein Wechsel der Geschwindigkeit ist umgesetzt.
# Basis des Scripts von volkszaehler.org / Autor: Andreas Schulze & Bugfix: Eric Schanze
# DPT9 sub: makki / www.knx-user-forum.de
# Baudwechsel: panzaeron / www.knx-user-forum.de
# Erweiterung um RRD,KNX-Anbindung und gezielte Wertsuche auf Wiregate: 
# JuMi2006 / www.knx-user-forum.de
# Version: 0.1.6
# Datum: 16.08.2012

use warnings;
use strict;
use RRDs;

### KONFIGURATION ###

my $device = "/dev/Zaehler_HZ";	#Port
my $rrdpath = "/var/www/rrd";		#Pfad fuer RRDs
my $counterid = "test";			#Grundname fuer RRDs
my $baudrate = "auto";			#Baudrate fuer Zaehlerauslesung "auto" oder 300,600,etc
my %channels = (			#Obis-Zahl => Gruppenadresse
		"16.7"=>"12/1/1",	#akt. Leistung
		"32.7"=>"12/1/11",	#Spannung L1
		"52.7"=>"12/1/21",	#Spannung L2
		"31.7"=>"12/1/10",	#Stromstaerke L1
		"51.7"=>"12/1/20",	#Stromstaerke L2
		"71.7"=>"12/1/30",	#Stromstarke L3
		"72.7"=>"12/1/32",	#Spannung L3
		"1.8.1"=>"12/1/0"	#Zaehlerstand gesamt
				);
my @countermodes = (5,15,60,1440);	#Aufloesungen fuer COUNTER RRDs in Minuten (1440 = Tagesverbrauch)
my $debug = 0;

### ENDE KONFIGURATION ###
#Je nach Geschwindigkeit andere Befehle an Zaehler senden

my %speedrate = (			
		"300"=>"'\x06\x30\x30\x30\x0d\x0a'",
		"600"=>"'\x06\x30\x31\x30\x0d\x0a'",
		"1200"=>"'\x06\x30\x32\x30\x0d\x0a'",
		"2400"=>"'\x06\x30\x33\x30\x0d\x0a'",
		"4800"=>"'\x06\x30\x34\x30\x0d\x0a'",
		"9600"=>"'\x06\x30\x35\x30\x0d\x0a'"
		);
my %speedrate_auto = (			
		"0"=>"'\x06\x30\x30\x30\x0d\x0a'",
		"1"=>"'\x06\x30\x31\x30\x0d\x0a'",
		"2"=>"'\x06\x30\x32\x30\x0d\x0a'",
		"3"=>"'\x06\x30\x33\x30\x0d\x0a'",
		"4"=>"'\x06\x30\x34\x30\x0d\x0a'",
		"5"=>"'\x06\x30\x35\x30\x0d\x0a'"
		);
my %baud = (			
		"0"=>"300",
		"1"=>"600",
		"2"=>"1200",
		"3"=>"2400",
		"4"=>"4800",
		"5"=>"9600"
		);

### DATEN EMPFANGEN ###

### Anfrage Senden ###
my $id = qx(echo '\x2f\x3f\x21\x0d\x0a' | socat -T1 - $device,raw,echo=0,b300,parenb=1,parodd=0,cs7,cstopb=0);
my $speedcode =  substr($id,4,1);
my $ack = $speedrate{$baudrate};

### Zählerkennung auswerten - Geschwindigkeit ermitteln ###
if ($baudrate eq "auto")
{
$ack = $speedrate_auto{$speedcode};
$baudrate = $baud{$speedcode};
}
else
{
$ack = $speedrate{$baudrate};
}

if ($debug==1){print ($id,"\n")};
if ($debug==1){print ($baudrate,"\n")};

select(undef, undef, undef, 1); 
### Abfrage starten ###
my @buffer = qx(echo $ack | socat -t1 - $device,raw,echo=0,b300,parenb=1,parodd=0,cs7,cstopb=0; socat -T 1 - $device,raw,echo=0,b$baudrate,parenb=1,parodd=0,cs7,cstopb=0 );
if ($debug==1){print (@buffer,"\n")};
### AUSWERTUNG ###
foreach (@buffer)
{
	foreach my $obis(%channels)
	{
	my $obiskey = $obis."\(";
	if ($_ =~ /\Q$obiskey\E/)
	{
	$_  =~ m/[^(]+\(([^*]+)\*([^)]+)/;
	my $value = $1;
	my $unit = $2;
	my $ga = $channels{$obis};
	if ($debug==1){print ($obis,"\n")};
	if ($debug==1){print ($value,"\n")};
	if ($debug==1){print ($unit,"\n")};
	if ($debug==1){print ($ga,"\n")};
	if ($unit =~ /\Qh\E/)
	{
	&rrd_counter ($obis,$value)
	}
	else
	{
	&rrd_gauge ($obis,$value)
	}
	&knx_write ($ga,$value);
	}
	}
}

### SUBS ###

sub rrd_counter
{
if ($debug==1){print ("COUNTER","\n")};
foreach (@countermodes)
{
my $obisname = $_[0];
my $value = $_[1];
$obisname =~ tr/./-/;
my $rrdname = $counterid."_".$obisname."_".$_."\.rrd";
if ($debug==1){print ($rrdname,"\n")};
my $rrdfile = $rrdpath."\/".$rrdname;
unless (-e $rrdfile)
{
RRDs::create ($rrdfile,"DS:value:COUNTER:".(($_*60)+600).":0:10000000000","RRA:AVERAGE:0.5:1:365","RRA:AVERAGE:0.5:7:300","-s ".($_*60));
}
my $countervalue = int($value*$_*60);
RRDs::update("$rrdfile", "N:$countervalue");
}
}

sub rrd_gauge
{
if ($debug==1){print ("GAUGE","\n")};
my $obisname = $_[0];
my $value = $_[1];
$obisname =~ tr/./-/;
my $rrdname = $counterid."_".$obisname."\.rrd";
if ($debug==1){print ($rrdname,"\n")};
my $rrdfile = $rrdpath."\/".$rrdname;
unless (-e $rrdfile)
{
RRDs::create ($rrdfile,"DS:value:GAUGE:900:0:10000000000","RRA:AVERAGE:0.5:1:2160","RRA:AVERAGE:0.5:5:2016","RRA:AVERAGE:0.5:15:2880","RRA:AVERAGE:0.5:60:8760");
}
RRDs::update("$rrdfile", "N:$value");
}

sub knx_write
{
### Wert in DPT9 umwandeln und in Konsole ausgeben
my @hexdec = encode_dpt9($_[1]);
my $hexval = sprintf("%x", $hexdec[0]) . " " . sprintf("%x", $hexdec[1]);
#print ($hexval,"\n");
system("groupwrite ip:localhost $_[0] $hexval");
}

sub encode_dpt9 
{
# 2byte signed float
my $state = shift;
my $data;
my $sign = ($state <0 ? 0x8000 : 0);
my $exp  = 0;
my $mant = 0;
$mant = int($state * 100.0);
while (abs($mant) > 2047)
{
$mant /= 2;
$exp++;
}
$data = $sign | ($exp << 11) | ($mant & 0x07ff);
return $data >> 8, $data & 0xff;
}
