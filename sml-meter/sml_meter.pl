#!/usr/bin/perl
# Autor: JuMi2006 / www.knx-user-forum.de
# knx_write sub: makki / www.knx-user-forum.de
# Version: 0.1
# Datum: 19.02.2013

use warnings;
use strict;
use Device::SerialPort;
use feature "switch";
use EIBConnection;
use RRDs;

#0701000F0700FF = 7.0       FIXME !!! [0F]0700
#070100010801FF = 1.8.1

my $eib_url = "local:/tmp/eib";     #for local eibd "local:/tmp/eib" for eibd in LAN: "ip:192.168.2.220:6720"
my $device = "/dev/ttyUSB-1-4";
my $rrdpath = "/var/www/rrd";

my @obis;
push @obis,{obis=>"1.8.0",  fact=>10000, ga =>"14/8/1", dpt => 14, rrd_name => "Zaehler_Verbrauch", rrd => "c"   }; #rrd: c=counter ; g=gauge
push @obis,{obis=>"7.0",    fact=>10,    ga =>"14/8/0", dpt => 9 , rrd_name => "Zaehler_Leistung",  rrd => "g" };
my @countermodes = (5,15,60,1440);    #Aufloesungen fuer COUNTER RRDs in Minuten (1440 = Tagesverbrauch)


my $debug = "";
my $port = Device::SerialPort->new($device) || die $!;
$port->databits(8);
$port->baudrate(9600);
$port->parity("none");
$port->stopbits(1);
$port->handshake("none");
$port->write_settings;
$port->dtr_active(1);
$port->purge_all();
$port->read_char_time(0);     # don't wait for each character
$port->read_const_time(2000); # 1 second per unfulfilled "read" call


my ($x,$data);
my $start = 0;
while ($start < 2) { # wait for second 1B1B1B1B01010101
       my ($count,$saw)=$port->read(512);   # will read 512 chars
            if ($count == 512) {       # wurden 512 chars gelesen ?       
            $x = uc(unpack('H*',$saw)); # nach hex wandeln
            $data .= $x;
            if ($data =~ /1B1B1B1B01010101/){$start ++};
       }
}

$data =~ m/1B1B1B1B01010101(.*?)B1B1B1/;
my $sml = $1;
#print $1."\n";

foreach my $obis (@obis)
{
$obis->{obis} =~ s/\./0/g;
$obis->{obis} .= "FF";
if ($debug==1){print $obis->{obis}." Obis\n";}
$sml =~ m/$obis->{obis}(.*?)0177/;
if ($debug==1){print $1." contains hex \n";}
my $sml_val = $1;
#extract value
$sml_val =~ s/^.*52FF//;
$sml_val = substr($sml_val,2);
print $sml_val." hex \n";
my $value = $sml_val;
my $dec_value = sprintf("%d", hex($value));
$dec_value /= $obis->{fact};
print $dec_value."<<<<---- Wert\n";

if ($obis->{rrd} eq "c")
            {
                &rrd_counter ($obis->{rrd_name},$dec_value)
            }
if ($obis->{rrd} eq "g")
            {
                &rrd_gauge ($obis->{rrd_name},$dec_value)
            }

&knx_write ($obis->{ga},$dec_value,$obis->{dpt});
print "GA:".$obis->{ga}." Wert:".$dec_value." DPT:".$obis->{dpt}."\n";
}


### SUBS ###

sub rrd_counter
{
    if ($debug==1){print ("COUNTER","\n")};
    foreach (@countermodes)
    {
        my $obisname = $_[0];
        if ($debug==1){print $obisname." obisname \n";}
        my $value = $_[1];
        if ($debug==1){print $value." value \n";}
        my $rrdname = $obisname."_".$_."\.rrd";
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
    if ($debug==1){print $obisname." obisname \n";}
    my $value = $_[1];
    if ($debug==1){print $value." value \n";}
    my $rrdname = $obisname."\.rrd";
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