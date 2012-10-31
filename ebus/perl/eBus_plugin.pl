# Beta fuer eBus-Anbindung von Heizungen/Waermepumpen
# Diese Beta bezieht sich auf eine Vaillant VWS 82-3 Waermepumpe
# Fuer jeden Befehl wurde vorher eine sub erstellt die die Daten an die richtige Stelle setzt und
# richtig codiert. Dies laesst sich bestimmt noch schlanker gestalten in dem man die Konfigurationszeilen erweitert.
# Der generelle Aufbau eines eBus-Telegramms: QQ ZZ PB SB NN DB1...DBx CRC
# QQ = Quelladresse, ZZ = Zieladresse, PB = Primaerbefehl, SB = Sekundarbefehl, NN = Anzahl der Datenbytes, DB1...DBx = Datenbytes, CRC = Checksummenbyte.
# Teils liegen die Daten die es zu senden gibt an unterschiedlichen Stellen, in der Konfiguration sind diese dann leer (z.B. DB2="").
#
# Wiregte-Socket-Einstellungen:
# Socket 1: "-" und "Socket: /dev/ttyUSB0" (hier eigenen Adapter) und "Optionen: raw,b2400,echo=0"
# Socket 1: "udp-datagram" und "Socket: localhost:50110  und "Optionen: bind=localhost:50111,reuseaddr"
# aktiviert und bidirektional
#
# Das Perl-Modul EBus.pm muss installiert sein, vergleiche hierzu:
# http://knx-user-forum.de/253728-post64.html
#
# Ein separater eBus-Daemon ist erforderlich !!!
#
# 29.10.2012
# JuMi2006 (http://knx-user-forum.de)
# Version 0.2
#return;

$plugin_info{$plugname.'_cycle'} = 300;
use warnings;
use strict;
use EBus;

my ($string,$data,@data,$QQ,$ZZ,$PB,$SB,$DB,$DB1);
my $debug = 2; # 0=zero log messages / 1=all log messages / 2=only value messages

################################
#### Vaillant Konfiguration ####
################################
# QQ = Quelle ; ZZ = Ziel ; PB = Primarbefehl ; SB = Sekundaerbefehl ; DB = Anzhal Datenbytes ; DB1 = Datenbyte 1 ff.
# name = Name der Sub in der die Befehle entschlŸsselt werden
# FIXME !!! Nicht fŸr jeden Befehl eine neue Sub erstellen, alle Daten in der Konfiguration ablegen

my @sets;
push @sets, { name => "RT_soll", 	GA => "0/5/101", 	QQ => "FF", ZZ => "50", PB => "B5", SB => "05", DB => "02", DB1 => "01", DB2 => "" };
push @sets, { name => "RT_min", 	GA => "0/5/102", 	QQ => "FF", ZZ => "50", PB => "B5", SB => "05", DB => "02", DB1 => "0A", DB2 => "" };

push @sets, { name => "HK_mode",	GA => "0/5/110", 	QQ => "FF", ZZ => "50", PB => "B5", SB => "05", DB => "02", DB1 => "02", DB2 => "" };
push @sets, { name => "HK_party", 	GA => "0/5/111", 	QQ => "FF", ZZ => "50", PB => "B5", SB => "05", DB => "02", DB1 => "05", DB2 => "" };
push @sets, { name => "HK_spar", 	GA => "0/5/112", 	QQ => "FF", ZZ => "50", PB => "B5", SB => "05", DB => "02", DB1 => "07", DB2 => "" };

push @sets, { name => "WW_mode", 	GA => "0/5/120", 	QQ => "FF", ZZ => "25", PB => "B5", SB => "09", DB => "04", DB1 => "0E", DB2 => "2B", DB3 => "00", DB4 => ""};
push @sets, { name => "WW_soll", 	GA => "0/5/121", 	QQ => "FF", ZZ => "25", PB => "B5", SB => "09", DB => "05", DB1 => "0E", DB2 => "82", DB3 => "00", DB4 => "", DB5 => "" };
push @sets, { name => "WW_min", 	GA => "0/5/122", 	QQ => "FF", ZZ => "25", PB => "B5", SB => "09", DB => "05", DB1 => "0E", DB2 => "82", DB3 => "00", DB4 => "", DB5 => "" };
push @sets, { name => "WW_load", 	GA => "0/5/123", 	QQ => "10", ZZ => "FE", PB => "B5", SB => "05", DB => "02", DB1 => "06", DB2 => "" };

push @sets, { name => "HK_curve", 	GA => "0/5/131", 	QQ => "FF", ZZ => "50", PB => "B5", SB => "05", DB => "03", DB1 => "0B", DB2 => "", DB3 => "00" };
push @sets, { name => "AT_off", 	GA => "0/5/132", 	QQ => "FF", ZZ => "50", PB => "B5", SB => "09", DB => "04", DB1 => "0E", DB2 => "36", DB3 => "00", DB4 => "", DB5 => "00" };
push @sets, { name => "HK_int", 	GA => "0/5/133", 	QQ => "10", ZZ => "08", PB => "B5", SB => "05", DB => "05", DB1 => "0E", DB2 => "7C", DB3 => "00", DB4 => "", DB5 => "FF" };

push @sets, { name => "HOL_temp", 	GA => "0/5/140", 	QQ => "FF", ZZ => "FE", PB => "B5", SB => "05", DB => "02", DB1 => "2A", DB2 => ""};
push @sets, { name => "HOL_date", 	GA => "0/5/141", 	QQ => "FF", ZZ => "FE", PB => "B5", SB => "0B", DB => "08", DB1 => "01", DB2 => "Nr", DB3 => "ST", DB4 => "SM", DB5 => "SJ", DB6 => "ET", DB7 => "EM", DB8 => "EJ"};###FIXME DATATYP

push @sets, { name => "ZH_hk", 	        GA => "0/5/151", 	QQ => "FF", ZZ => "08", PB => "B5", SB => "09", DB => "04", DB1 => "0E", DB2 => "63", DB3 => "00", DB4 => "" };
push @sets, { name => "ZH_ww", 	        GA => "0/5/152", 	QQ => "FF", ZZ => "08", PB => "B5", SB => "09", DB => "04", DB1 => "0E", DB2 => "64", DB3 => "00", DB4 => "" };
push @sets, { name => "ZH_int", 	GA => "0/5/153", 	QQ => "FF", ZZ => "08", PB => "B5", SB => "09", DB => "04", DB1 => "0E", DB2 => "63", DB3 => "00", DB4 => "" }; #FIXME
push @sets, { name => "ZH_hys", 	GA => "0/5/154", 	QQ => "FF", ZZ => "08", PB => "B5", SB => "09", DB => "05", DB1 => "0E", DB2 => "7E", DB3 => "00", DB4 => "", DB5 => "00" }; #FIXME
### Ende Vaillant Konfiguration ###

################################
#### Konfiguration einlesen ####
################################
foreach my $set (@sets) {
$plugin_subscribe{$set->{GA}}{$plugname} = 1;   # An Gruppenadresse anmelden

if ($msg{'apci'} eq "A_GroupValue_Write" && $msg{'dst'} eq $set->{GA} && defined $msg{'value'})   # Auf eintreffendes KNX-Telegramm reagiern + anhand der GA filtern

{	
my $val = $msg{'value'};                                		# Wert aus Telegramm filtern
my $subname = $set->{name};                             		# $subname bekommt den Namen der entsprecheden sub
my $subref = \&$subname;                                		# jetzt wird $subref die entsprechende sub zugewiesen
plugin_log($plugname, "Befehlsgruppe: $set->{name} Value: $val"); 	# Logging der Befehlsgruppe
my $command = addCRC(&$subref($val));                   		# Befehls-Sub ausführen und CRC anhängen

&send ($command."AA");   # Befehl senden
#&send ($command);   # Befehl senden
}
}

#########################################
#### P L U G I N - S o c k e t ##########
#### Writes collected Data to Plugin ####
#########################################
# socat - udp:localhost:50201 ###########

my $socknum = 1000; 
my $daemon_recv_ip = "localhost"; # Empfangsport (UDP, siehe in Socket-Einstellungen)
my $daemon_recv_port = "50201"; # Empfangsport (UDP, siehe in Socket-Einstellungen)

if (!$socket[1000]) { # socket erstellen
        $socket[1000] = IO::Socket::INET->new(
				Proto => "udp",
				LocalAddr => $daemon_recv_ip,
				LocalPort => $daemon_recv_port,
				reuseaddr => 1
                                )
    or return ("open of $daemon_recv_ip : $daemon_recv_port failed: $!");

	$socksel->add($socket[1000]); # add socket to select
	$plugin_socket_subscribe{$socket[1000]} = $plugname; # subscribe plugin
	plugin_log($plugname,'Socket verbunden. Daemon-Plugin Socket Nummer: ' . 1000);
	#return "opened Socket 1000";   
}

#############################################
#### e B U S - S o c k e t ##################
#### Reads and writes "hot" to SerialPort ###
#############################################
# socat /dev/ttyUSB0,raw,b2400,echo=0 udp-datagram:localhost:50110,bind=localhost:50111,reuseaddr

my $socknum = 999; 
my $send_ip = "localhost"; # Sendeport (UDP, siehe in Socket-Einstellungen)
my $send_port = "50111"; # Sendeport (UDP, siehe in Socket-Einstellungen)
my $recv_ip = "localhost"; # Empfangsport (UDP, siehe in Socket-Einstellungen)
my $recv_port = "50110"; # Empfangsport (UDP, siehe in Socket-Einstellungen)

### Socket einrichten ###

if (!$socket[999]) { # socket erstellen        
$socket[999] = IO::Socket::INET->new(
                                  Proto => "udp",
                                  PeerAddr  => $send_ip,
                                  PeerPort  => $send_port,
                                  LocalAddr => $recv_ip,
                                  LocalPort => $recv_port,
                                  ReuseAddr => 1
                                   )
   or return ("open of $recv_ip : $recv_port failed: $!");

    $socksel->add($socket[999]); # add socket to select
#
#    $plugin_socket_subscribe{$socket[$socknum]} = $plugname; # subscribe plugin
    plugin_log($plugname,'Socket verbunden. eBus-Socket Nummer: ' . 999);
#    return "opened Socket $socknum";   
}
###

###############
### M A I N ###
###############

###############################
#### R E C I V E  D A T A #####
###############################

if ($fh)				# Auf ankommende UDP-Daten reagieren (socket_subscribe)
{    
my $recived = <$fh>;			# Einlesen Zeilenweile

	# DEBUG-MESSAGES-START
	if ($debug == "1"){
	my $len = length $recived;			# Laenge feststellen (DEBUG)
	plugin_log($plugname,"Length of FH: $len");	# LŠnge ausgeben
	plugin_log($plugname,"$recived")}		# empfangene Daten ausgeben
	# DEBUG-MESSAGES-END
		
my @strings = split(/SYNC/, $recived); 	# Datenpacket in Array packen. Separator is "SYNC" -> comes from daemon !!!

foreach my $string (@strings)		# Jedes Telegram aus dem Array durchlaufen
{		
		# DEBUG-MESSAGES-START 
		if ($debug == "1")
		{plugin_log($plugname,"$string");		# Telegramm ausgeben (QQZZPBSB....)
		my $string_format =~ s/(.{2})/$1/g;		# Telegramm formatieren
		plugin_log($plugname,"Telegramm: $string")	# Telegramm formatiert ausgeben (QQ ZZ PB SB ...)
		}
		# DEBUG-MESSAGES-END	
	
	&check($string);		# Telegramm an die Sub zum auswerten (&check) uebergeben
}
}

###################
### S E N D E N ###
###################

sub send {
if ($debug = 1) {plugin_log($plugname, "Sub Senden erreicht !");}
my $cmd = shift;
my $sync = 0;
        while ($sync < 2)
                {
                        my $buf;
                        recv($socket[999],$buf,1,0); # unbuffered read from socket
                        #recv($socket[$socknum],$buf,1,0); # unbuffered read from socket
                        #read($socket[$socknum],$buf,1);
                        my $bufhex = $buf;
                        $bufhex =~ s/(.)/sprintf("%.2X",ord($1))/eg;
                        plugin_log($plugname, "Buffer: $bufhex");
                        #if ($debug == "2") { plugin_log($plugname, "Buffer: $bufhex");}
                        
                                if ($bufhex eq "AA")    # AA = SYNC ... hier warten wir jetzt auf das SYNC Zeichen und senden dann
                                {
                                $sync++;
                                plugin_log($plugname, "Sync Zeichen gelesen");
                                #if ($debug == "2") { plugin_log($plugname, "Sync Zeichen gelesen");}
                                my $raw = $cmd;
                                $raw =~ s/([0-9a-f]{2})/chr( hex( $1 ) )/gie;      # !!! Umwandlung des Hex-Strings
                                plugin_log($plugname, "send: $cmd");
                                #if ($debug == "2") { plugin_log($plugname, "send: $cmd");}
                                syswrite($socket[999], $raw);
                                select(undef, undef, undef, 1.5);
                                #last;   # Nun aber raus aus der Schleife
                                }
                                else
                                {redo;}
                }
             
}


##################################
#### V A I L L A N T  S U B S ####
##################################
#lassen sich bestimmt noch optimieren in dem man die Konfiguration etwas erweitert

### Sparen bis ...  30min = 0,5 Stunden (21:30Uhr = 21,5) ###
sub HK_spar
{
foreach my $set(@sets){
if ($set->{name} eq "HK_spar")
{
my $input = $_[0];
my $val = (sprintf "%02d",$input); 				
$val = encode_d1c ($val);
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$val;
return $message;
}
}
}
###

### Ferientemperatur ###
sub HOL_temp
{
foreach my $set(@sets){
if ($set->{name} eq "HOL_temp")
{
my $input = $_[0];
my $val = (sprintf "%02d",$input); 				
$val = encode_d1b ($val);
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$val;
return $message;
}
}
}
###

### Raumtemperatur Soll ###
sub RT_soll
{
foreach my $set(@sets){
if ($set->{name} eq "RT_soll")
{
my $input = $_[0];
my $val = (sprintf "%02d",$input); 				
$val = encode_d1b ($val);
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$val;
return $message;
}
}
}
###

### Raumtemperatur Absenkung ###
sub RT_min
{
foreach my $set(@sets){
if ($set->{name} eq "RT_min")
{
my $input = $_[0];
my $val = (sprintf "%02d",$input); 				
$val = encode_d1b ($val);
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$val;
return $message;
}
}
}
###

### 1-Heizen, 2-Aus, 3-Auto, 4-Eco, 5-Absenken ###
sub HK_mode
{
foreach my $set(@sets){
if ($set->{name} eq "HK_mode")
{
my $input = $_[0];
my $val = (sprintf "%02d",int($input)); 
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$val;
return $message;
}
}
}
###

### Heizkurve 0,20 etc ###
sub HK_curve
{
foreach my $set(@sets){
if ($set->{name} eq "HK_curve")
{
my $input = $_[0];
$input *= 100;
my $val = (sprintf "%02d",$input); 
$val = encode_d1b($val);
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$val.$set->{DB3};
return $message;
}
}
}
###

### 0-Partymodus aus, 1-Partymodus an ###
sub HK_party
{
foreach my $set(@sets){
if ($set->{name} eq "HK_party")
{
my $input = $_[0];
my $val = (sprintf "%02d",$input);
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$val;
return $message;
}
}
}
###

### Betriebsmodus WW 1-aus, 2-an, 3-auto ###
sub WW_mode
{
foreach my $set(@sets){
if ($set->{name} eq "WW_mode")
{
my $input = $_[0];
my $val = (sprintf "%02d",$input);
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$set->{DB2}.$set->{DB3}.$val;
return $message;
}
}
}
###

### 0-Speicherladung abbrechen, 1-Speicherladung ###
sub WW_load
{
foreach my $set(@sets){
if ($set->{name} eq "WW_load")
{
my $input = $_[0];
my $val = (sprintf "%02d",$input);
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$val;
return $message;
}
}
}
###


### Energieintegral setzen ###
sub HK_int
{
foreach my $set(@sets){
if ($set->{name} eq "HK_int")
{
my $input = $_[0];
my $val = (sprintf "%02d",$input);
$val = encode_d1b($val);
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$set->{DB2}.$set->{DB3}.$val.$set->{DB3};
return $message;
}
}
}
###

### Außentemperatur Abschaltgrenze ###
sub AT_off
{
foreach my $set(@sets){
if ($set->{name} eq "AT_off")
{
my $input = $_[0];
my $val = (sprintf "%02d",$input);
$val = encode_d1b($val);
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$set->{DB2}.$set->{DB3}.$val.$set->{DB3};
return $message;
}
}
}
###

### Solltemperatur WW-Speicher ### 
sub WW_soll
{
foreach my $set(@sets){
if ($set->{name} eq "WW_soll")
{
my $input = $_[0];
my $val = (sprintf "%02d",$input);
$val = encode_d2c($val);
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$set->{DB2}.$set->{DB3}.$val;
return $message;
}
}
}
###

### Min.-Temperatur WW-Speicher ### 
sub WW_min
{
foreach my $set(@sets){
if ($set->{name} eq "WW_min")
{
my $input = $_[0];
my $val = (sprintf "%02d",$input); 	
$val = encode_d2c($val);
my $message = $set->{QQ}.$set->{ZZ}.$set->{PB}.$set->{SB}.$set->{DB}.$set->{DB1}.$set->{DB2}.$set->{DB3}.$val;
return $message;
}
}
}
###


############################
#### S U B  F I L T E R ####
############################
# Prueft die UDP-Daten auf verwertbare DatensŠtze

sub check {
$string = shift;
chomp $string;
my @bytes = $string =~ /..?/sg; 	# Telegramm in einzelne Bestandteile zerlegen -> Array
if ($debug == "1"){plugin_log($plugname,"String: @bytes")}
$QQ = $bytes[0];	# Quelladresse
$ZZ = $bytes[1];	# Zieladresse
$PB = $bytes[2];	# Primaerbefehl
$SB = $bytes[3];	# Sekundaerbefehl
$DB = $bytes[4]; 	# Anzahl DatenBytes
$DB1 = $bytes[5]; 	# Erstes Datenbyte zur Typunterscheidung von Telegrammen

$data = substr $string,8,((2*$DB)+2);	# Datenbytes extrahieren
my @data_bytes = $data =~ /..?/sg;	# Datenbytes in Array ablegen $data_bytes[0] = Anzahl der Datenbytes


### Read Values ###
# Datum/Uhrzeit 10FEB516080000(58)(08)(28)(10)(07)(12)C7
if ($SB eq 16 && $DB == 8){
        my $ss = $data_bytes[2];
        my $mm = $data_bytes[3];
	my $hh = $data_bytes[4];
	my $dd = $data_bytes[5];
	my $mon = $data_bytes[6];
	my $yy = $data_bytes[8];
	my $wday = $data_bytes[7];
if ($debug == "1"){plugin_log($plugname,"Datenbytes:@data_bytes")};
if ($debug == "2" or $debug == "1"){plugin_log($plugname,"datetime: $dd.$mon.$yy $hh:$mm:$ss")};
}


# Vorlauf SOLL: 1008B510090002(32)000000000002 DATA1c
if ($SB == 10 && $DB == 9){
        my $val = decode_d1c($data_bytes[3]);
if ($debug == "1"){plugin_log($plugname,"Datenbytes:@data_bytes")}
if ($debug == "2" or $debug == "1"){plugin_log($plugname,"Vorlauf SOLL: $val Grad")}
knx_write("0/5/202", $val, 9.001);  # GA,Wert,DPT
}

# Vorlauf K‰ltekreis IST: 1008B5042700(15)00 DATA1b
if ($SB == 5 && $DB == 4 && $DB1 == 27){
        my $val = decode_d1b($data_bytes[3],$data_bytes[4]);
if ($debug == "1"){plugin_log($plugname,"Datenbytes:@data_bytes")}
if ($debug == "2" or $debug == "1"){plugin_log($plugname,"Kaeltekreis Vorlauf: $val Grad")}
knx_write("0/5/203", $val, 9.001);  # GA,Wert,DPT
}

# WW / Heizen: 1008B50427(00)1500
if ($SB == 5 && $DB == 4 && $DB1 == 27){

        my $val = $data_bytes[2];
if ($debug == "1"){plugin_log($plugname,"Datenbytes:@data_bytes")}
if ($val == 0)
{
if ($debug == "2" or $debug == "1"){plugin_log($plugname,"Betriebsart: Heizbetrieb")}
}
else
{
if ($debug == "2" or $debug == "1"){plugin_log($plugname,"Betriebsart: Warmwasser")}
}}

# Aussentemperatur: 10FE070009(F009)043813161001 DATA2b
if ($PB == 7 && $SB == 0 && $DB == 9){
        
        my $val = decode_d2b($data_bytes[1],$data_bytes[2]);
if ($debug == "1"){plugin_log($plugname,"Datenbytes:@data_bytes")}
if ($debug == "2" or $debug == "1"){plugin_log($plugname,"Aussentemperatur: $val Grad")}
knx_write("0/5/200", $val, 9.001);  # GA,Wert,DPT
}

###SEARCH FOR INFO###
# 1008B509040E....
if ($SB == 9 && $DB == 4){
plugin_log($plugname,"search B50904$data_bytes[1]: $data_bytes[2] $data_bytes[3] $data_bytes[4]")
}

###SEARCH FOR INFO###
# 1008B5090329....
if ($SB == 9 && $DB == 3 && $DB1 == 29){
plugin_log($plugname,"search B50903$data_bytes[1]: $data_bytes[2] $data_bytes[3]")
}

###SEARCH FOR INFO###
# Befehl auf dem Bus ???
if ($QQ eq "FF"){
plugin_log($plugname,"Befehl gelesen: $string")
}

### End Read Values ####

#my $telegrams = plugin_info($plugname,"eBus")#;
#my @known = split(/\n/, $telegrams); #Array aus $telegrams erstellen/auslesen##
#
#if ( grep /^$string$/, @known ) {
#   # vorhanden
#}
#else {
#   push @known,$string;
#}
#
#$telegrams = join ("\n",@known);
#plugin_info($plugname,"eBus") = $telegrams;
#plugin_log($plugname,"$telegrams");
###LOG to FILE
#my $file='/root/temp.log';
#open (in,"<$file") || die $!;
#while (<in>){
#$_= @lines;
#}
#close in;




}


############################################
### D A T E N K O N V E R T I E R U N G ####
############################################


### BCD ###
#FIX ME !!!!!

#sub decode_bcd {
#    return (unpack "H*", pack "C*",hex($_[0]));
#	#unpack "H*", $_[0]; ####?????
#}

sub decode_bcd {
        my $z = $_[0];
        my $high = hex($z) >> 4;
        my $low  = hex($z) & 15;
        return $high * 10 + $low;
}

sub encode_bcd {
	return pack 'H*', join '', $_[0];
}

### DATA1b ###

sub decode_d1b{         #1byte signed 
    my $val = hex(shift);
    return $val > 127 ? $val-256 : $val;
}

sub encode_d1b {        #### F I X M E !!!!!
    my $y = shift;
    $y *= 256;
    $y = $y & 0xffff if ($y < 0);
    my $hb = int $y/256;
    return (sprintf("%0.2X", $hb));
}


### DATA1c ###

sub decode_d1c {
my $y = hex ($_[0])/2;
return $y;
}

sub encode_d1c {
    return sprintf "%x",(($_[0])*2);
}


### DATA2b ###

sub decode_d2b { 
	return unpack("s", pack("s", hex($_[1].$_[0]))) / 256; 
}

sub encode_d2b {
	my ($hb, $lb) = unpack("H[2]H[2]", pack("s", $_[0] * 256));
	return $lb.$hb;
}

### alternativ
#sub decodex_d2b{
#    my $hb = hex($_[0]);
#    my $lb = hex($_[1]);
#
#   if ($hb & 0x80) {
#      return -( (~$hb & 255) + ((~$lb & 255) + 1)/256 );
#    }
#   else {
#        return $hb + $lb/256;
#    }
#}
#
#sub encodex_d2b {
#    my $y = shift;
#    $y *= 256;
#    $y = $y & 0xffff if ($y < 0);
#    my $hb = int $y/256;
#    my $lb = $y % 256;
#    return (sprintf("%0.2X", $hb), sprintf("%0.2X", $lb));
#}


### DATA2c ###

sub decode_d2c{
my $high = $_[1];
my $low = $_[0];
return unpack("s",(pack("H4", $low.$high)))/16;
}

sub encode_d2c{
my $val = $_[0];
my $temp_hex = unpack("H4", pack("n", ($val)*16));
# change lowbyte/highbyte -> lowbyte first
return substr($temp_hex,2,4).substr($temp_hex,0,2);
}


####################
### A D D  C R C ###
####################

### CRC hinzufuegen
sub addCRC
{
my $string = $_[0];
my $temp_string = pack("H*","$string"); ### geht auch!
my $crc = new EBus();
my $check = $crc->calcCrcExpanded($temp_string);
my $crcfinal = uc(sprintf("%02x", $check));
my $finalStr = $string.$crcfinal;
}
###

return;