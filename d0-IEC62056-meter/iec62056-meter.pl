#!/usr/bin/perl -w
# Zaehlerabfrage fuer Zaehler nach Protokoll IEC 62056-21 / OBIS
# Ein Anfrage-Telegramm ist mit 300 Baud, 7 Bit, 1 Stoppbit
# und gerader Paritaet zu senden. Das ist der Initialmodus von Geraeten,
# die das Protokoll IEC 62056-21 implementieren.
# Ein Wechsel der Geschwindigkeit ist umgesetzt.
# Basis des Scripts von volkszaehler.org / Autor: Andreas Schulze & Bugfix: Eric Schanze
# DPT9 sub: makki / www.knx-user-forum.de
# Baudwechsel: panzaeron / www.knx-user-forum.de
# Erweiterung um RRD,KNX-Anbindung und gezielte Wertsuche auf Wiregate:
# JuMi2006 / www.knx-user-forum.de (DPT14 beta fixed: Frank0207)
# Version: 0.1.7 
# Datum: 14.02.2013

use warnings;
use strict;
use RRDs;
use feature "switch";
use EIBConnection;

### KONFIGURATION ###
my $eib_url = "local:/tmp/eib";     #for local eibd "local:/tmp/eib" for eibd in LAN: "ip:192.168.2.220:6720"
my $device = "/dev/ttyUSB1";        #Port ttyUSB1
my $rrdpath = "/home/kWh/rrd";      #Pfad fuer RRDs
my $counterid = "HZ";               #Grundname fuer RRDs
my $baudrate = "auto";              #Baudrate fuer Zaehlerauslesung
my @channels;                       #Obis-Zahl => Gruppenadresse
push @channels, {name => "1.7.0", ga =>"4/0/35", dpt => 9 };    #akt. Leistung
push @channels, {name => "1.9.0", ga =>"4/0/14", dpt => 9 };    #monatl. Verbrauch
push @channels, {name => "1.8.1", ga =>"4/0/1", dpt => 14 };    #Zaehlerstand gesamt

my @countermodes = (5,15,60,1440);    #Aufloesungen fuer COUNTER RRDs in Minuten (1440 = Tagesverbrauch)
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
my $id = qx(echo '\x2f\x3f\x21\x0d\x0a' | socat -T 1 - $device,raw,echo=0,b300,parenb=1,parodd=0,cs7,cstopb=0);
my $speedcode =  substr($id,4,1);
my $ack = $speedrate{$baudrate};

### ZŠhlerkennung auswerten - Geschwindigkeit ermitteln ###
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
my @buffer = qx(echo $ack | socat -T 1 - $device,raw,echo=0,b300,parenb=1,parodd=0,cs7,cstopb=0; socat -T 1 - $device,raw,echo=0,b$baudrate,parenb=1,parodd=0,cs7,cstopb=0);
if ($debug==1){print (@buffer,"\n")};
### AUSWERTUNG ###
foreach (@buffer)
{
    foreach my $obis (@channels)
    {
        my $obiskey = $obis->{name}."\(";
        if ($_ =~ /\Q$obiskey\E/)
        {
            $_  =~ m/[^(]+\(([^*]+)\*([^)]+)/;
            my $value = $1;
            my $unit = $2;
            my $ga = $obis->{ga};
            if ($debug==1){print ($obis->{name},"\n")};
            if ($debug==1){print ($value,"\n")};
            if ($debug==1){print ($unit,"\n")};
            if ($debug==1){print ($ga,"\n")};
            if ($unit =~ /\Qh\E/)
            {
                &rrd_counter ($obis->{name},$value)
            }
            else
            {
                &rrd_gauge ($obis->{name},$value)
            }
            &knx_write ($ga,$value,$obis->{dpt});
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

sub knx_write {
    my ($dst,$value,$dpt,$response,$dbgmsg) = @_;
    my $bytes;
    my $apci = ($response) ? 0x40 : 0x80; # 0x40=response, 0x80=write
    #     DPT 1 (1 bit) = EIS 1/7 (move=DPT 1.8, step=DPT 1.7)
    #     DPT 2 (1 bit controlled) = EIS 8
    #     DPT 3 (3 bit controlled) = EIS 2
    #     DPT 4 (Character) = EIS 13
    #     DPT 5 (8 bit unsigned value) = EIS 6 (DPT 5.1) oder EIS 14.001 (DPT 5.10)
    #     DPT 6 (8 bit signed value) = EIS 14.000
    #     DPT 7 (2 byte unsigned value) = EIS 10.000
    #     DPT 8 (2 byte signed value) = EIS 10.001
    #     DPT 9 (2 byte float value) = EIS 5
    #     DPT 10 (Time) = EIS 3
    #     DPT 11 (Date) = EIS 4
    #     DPT 12 (4 byte unsigned value) = EIS 11.000
    #     DPT 13 (4 byte signed value) = EIS 11.001
    #     DPT 14 (4 byte float value) = EIS 9
    #     DPT 15 (Entrance access) = EIS 12
    #     DPT 16 (Character string) = EIS 15
    # $dpt = $eibgaconf{$dst}{'DPTSubId'} unless $dpt; # read dpt from eibgaconf if existing
    given ($dpt) {
        when (/^12/)             { $bytes = pack ("CCL>", 0, $apci, $value); }  #EIS11.000/DPT12 (4 byte unsigned)
        when (/^13/)             { $bytes = pack ("CCl>", 0, $apci, $value); }
        when (/^14/)             { $bytes = pack ("CCf>", 0, $apci, $value); }
        when (/^16/)             { $bytes = pack ("CCa14", 0, $apci, $value); }
        when (/^17/)             { $bytes = pack ("CCC", 0, $apci, $value & 0x3F); }
        when (/^20/)             { $bytes = pack ("CCC", 0, $apci, $value); }
        when (/^\d\d/)           { return; } # other DPT XX 15 are unhandled
        when (/^[1,2,3]/)        { $bytes = pack ("CC", 0, $apci | ($value & 0x3f)); } #send 6bit small
        when (/^4/)              { $bytes = pack ("CCc", 0, $apci, ord($value)); }
        when ([5,5.001])         { $bytes = pack ("CCC", 0, $apci, encode_dpt5($value)); } #EIS 6/DPT5.001 1byte
        when ([5.004,5.005,5.010]) { $bytes = pack ("CCC", 0, $apci, $value); }
        when (/^5/)              { $bytes = pack ("CCC", 0, $apci, $value); }
        when (/^6/)              { $bytes = pack ("CCc", 0, $apci, $value); }
        when (/^7/)              { $bytes = pack ("CCS>", 0, $apci, $value); }
        when (/^8/)              { $bytes = pack ("CCs>", 0, $apci, $value); }
        when (/^9/)              { $bytes = pack ("CCCC", 0, $apci, encode_dpt9($value)); } #EIS5/DPT9 2byte float
        default                  { LOGGER('WARN',"None or unsupported DPT: $dpt sent to $dst value $value"); return; }
    }
    my $leibcon = EIBConnection->EIBSocketURL($eib_url) or return("Error opening con: $!");
    if ($leibcon->EIBOpenT_Group(str2addr($dst),1) == -1) { return("Error opening group: $!"); }
    my $res=$leibcon->EIBSendAPDU($bytes);
    $leibcon->EIBClose();
    return $res;
    
    # str2addr: Convert an EIB address string in the form "1/2/3" or "1.2.3" to an integer
    sub str2addr {
        my $str = $_[0];
        if ($str =~ /(\d+)\/(\d+)\/(\d+)/) { # logical address
            return ($1 << 11) | ($2 << 8) | $3;
        } elsif ($str =~ /(\d+)\.(\d+)\.(\d+)/) { # physical address
            return ($1 << 12) | ($2 << 8) | $3;
        } else {
            #bad
            return;
        }
    }
    
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