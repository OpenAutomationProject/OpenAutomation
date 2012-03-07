#!/usr/bin/perl
#
# Holt die Daten vom SML-Zaehler Easymeter Q3C
# es wird die obere optische Schnittstelle ausgelesen
# dort liefert der Zaehler alle 2sec. einen Datensatz
# wird von CRON jede Minute aufgerufen
# http://wiki.volkszaehler.org/software/sml
# 03.2012 by NetFritz
#
use Device::SerialPort;
use RRDs;
#
my $port = Device::SerialPort->new("/dev/usbserial-2-4") || die $!;
$port->databits(8);
$port->baudrate(9600);
$port->parity("none");
$port->stopbits(1);
$port->handshake("none");
$port->write_settings;
$port->dtr_active(1);
$port->purge_all();
$port->read_char_time(0);     # don't wait for each character
$port->read_const_time(1000); # 1 second per unfulfilled "read" call
#
# OBIS-Kennzahl und Anzahl der Zeichen von Anfang OBIS bis Messwert, Messwertwertlaenge immer 8 Zeichen 
%obis_len = (
'01010801FF' => 42, # Aktueller Zählerstand T1 +  Bezug                                                
'02020805FF' => 42, # Aktueller Zählerstand T5 -  Abgabe
'00010700FF' => 24  # Momentane Summe der Leistung +-
); 
my $pos = 0;
my $hexval;
my @hexdec;
while ($i < 3) { # wenn 540 chars gelesen werden wird mit last abgebrochen, wenn nicht wird Schleife 2 mal widerholt
        my ($count,$saw)=$port->read(540);   # will read 540 chars
        if ($count == 540) {                 # wurden 540 chars gelesen ?       
                my $x=uc(unpack('H*',$saw)); # nach hex wandeln
                # print  "$count <> $x\n ";  # gibt die empfangenen Daten in Hex aus
                #
                foreach $key(keys%obis_len){
                      $pos=index($x,$key);                              # pos von OBIS holen
                      $value = substr($x,$pos+$obis_len{$key},8);       # Messwert
                      $value{$key} = hexstr_to_signed32int($value)/100; # von hex nach 32bit integer
                }
                      last; # while verlassen
        }
        else { 
               $i++;  
               redo; # bei Fehler while nochmal               
        }
}
#
@hexdec = encode_dpt9($value{'00010700FF'});
$hexval = sprintf("%x", $hexdec[0]) . " " . sprintf("%x", $hexdec[1]);
system("groupwrite ip:127.0.0.1 5/0/12 $hexval");
#
@hexdec = encode_dpt9($value{'01010801FF'});
$hexval = sprintf("%x", $hexdec[0]) . " " . sprintf("%x", $hexdec[1]);
system("groupwrite ip:127.0.0.1 5/0/13 $hexval");
#
@hexdec = encode_dpt9($value{'02020805FF'});
$hexval = sprintf("%x", $hexdec[0]) . " " . sprintf("%x", $hexdec[1]);
system("groupwrite ip:127.0.0.1 5/0/14 $hexval"); 
#
RRDs::update("/var/www/rrd/Zaehler_Leistung.rrd", "N: $value{'00010700FF'}"); 
#
#
# ---  Sub Konvertiert hex string to 32 bit signed integer ----------
sub hexstr_to_signed32int {
    my ($hexstr) = @_;
    die "Invalid hex string: $hexstr"
        if $hexstr !~ /^[0-9A-Fa-f]{1,8}$/;
    my $num = hex($hexstr);
    return $num >> 31 ? $num - 2 ** 32 : $num;
}
# ---  Sub  2byte signed float --------------------------------------
sub encode_dpt9 { 
    my $state = shift;
    my $data;

    my $sign = ($state <0 ? 0x8000 : 0);
    my $exp  = 0;
    my $mant = 0;

    $mant = int($state * 100.0);
    while (abs($mant) > 2047) {
        $mant /= 2;
        $exp++;
    }
    $data = $sign | ($exp << 11) | ($mant & 0x07ff);
    return $data >> 8, $data & 0xff;
}
# Ende

