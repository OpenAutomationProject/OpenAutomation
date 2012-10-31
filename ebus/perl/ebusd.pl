#!/usr/bin/perl
#############################
### e B U S - D a e m o n ###
#############################
## Collect data via socat from SerialPort
## alpha 1 by M.Hirsch
## 2012-10
#############################

use warnings;
use strict;
use EBus;
use IO::Socket::INET;


my ($saw,$bufhex,$string,$string_format,$data,@data,$QQ,$ZZ,$PB,$SB,$DB,$DB1);
my $bufold = '';
my $buf;
my $bufsum;
my $bufnew;
my $STALL_DEFAULT=20;
my $timeout=$STALL_DEFAULT;

#############################################
#### e B U S - S o c k e t ##################
#### Reads and writes "hot" to SerialPort ###
#############################################
# socat /dev/ttyUSB0,raw,b2400,echo=0 udp-datagram:localhost:50110,bind=localhost:50111,reuseaddr

my $ebus_send_ip = "localhost"; # Sendeport (UDP, siehe in Socket-Einstellungen)
my $ebus_send_port = "50111"; # Sendeport (UDP, siehe in Socket-Einstellungen)
my $ebus_recv_ip = "localhost"; # Empfangsport (UDP, siehe in Socket-Einstellungen)
my $ebus_recv_port = "50110"; # Empfangsport (UDP, siehe in Socket-Einstellungen)

### Socket erstellen ###
my $ebus_socket = IO::Socket::INET->new(
	Proto => "udp",
	LocalPort => $ebus_recv_port,
	LocalAddr => $ebus_recv_ip,
	PeerPort  => $ebus_send_port,
	PeerAddr  => $ebus_send_ip,
	ReuseAddr => 1
    )or die "Can't bind: $@\n";


#########################################
#### P L U G I N - S o c k e t ##########
#### Writes collected Data to Plugin ####
#########################################
# socat - udp:localhost:50201 ###########

my $plugin_send_ip = "localhost"; # Sendeport (UDP, siehe in Socket-Einstellungen)
my $plugin_send_port = "50201"; # Sendeport (UDP, siehe in Socket-Einstellungen)

### Socket erstellen ###
my $plugin_socket = IO::Socket::INET->new(
	Proto => "udp",
	#LocalPort => $recv_port,
	#LocalAddr => $recv_ip,
	PeerPort  => $plugin_send_port,
	PeerAddr => $plugin_send_ip,
	ReuseAddr => 1
    )or die "Can't bind: $@\n";



#################
#### M A I N ####
#################


while ($timeout > 0) {                  # FIXME !!!

$saw = read($ebus_socket,$buf,512); 	# Liest 255 Zeichen je Durchlauf aus

my $bufhex = $buf;
$bufhex =~ s/(.)/ sprintf '%02X', ord $1 /seg;
$bufhex =~ s/aaa/a\n/gi;
$bufhex =~ s/aa/\n/gi;
$bufnew = $bufhex;
$bufsum = $bufold.$bufnew;

my @array = split("\n", $bufsum);
my $lastelement=$array[-1];         # letztes Element im Array suchen und nachher einfach wieder vorne anhaengen
$bufold = "\n".$lastelement;        # hier wird das letzte Element (evtl. nur ein Bruchstueck) vorn angehangen
pop @array;							# letztes Element aus dem Array entfernen *maybe incomplete
foreach my $element (@array)
{
&output($element);
chomp $element
}
my $packet = join("SYNC",@array);
syswrite($plugin_socket, ($packet."\n"));
	foreach my $string (@array)
	{
	#&check($string);           # only for debugging
	}
#}
}

#########################################
#### S U B  F O R  D E B U G G I N G ####
#########################################
sub output {
my $element = shift;
print ($element,"\n");
}


sub check {
$string = shift;
my @bytes = $string =~ /..?/sg; 
$QQ = $bytes[0];
$ZZ = $bytes[1];
$PB = $bytes[2];
$SB = $bytes[3];
$DB = $bytes[4];  #Anzahl DatenBytes
$DB1 = $bytes[5]; #Erstes Datenbyte zur Typunterscheidung von Telegrammen

$data = substr $string,8,(2*$DB); 
my @data_bytes = $data =~ /..?/sg;
#print (@data_bytes,"\n");
}