use EBus;
use warnings;
use strict;

#my $deinString = "70515014074100050017005A"; # CRC = 0D

#my $string = "7051501407410005001A005A"; # CRC = F3

print "Enter eBus-Message: ";
my $string = <STDIN>;
chomp $string;
#$deinString =~ s/([a-fA-F0-9][a-fA-F0-9])/chr(hex($1))/eg; ### geht!
my $deinString = pack("H*","$string"); ### geht auch!

my $crc = new EBus();
my $check = $crc->calcCrcExpanded($deinString);

my $crcfinal = uc(sprintf("%02x", $check));
my $finalStr = $string.$crcfinal;

print ("CRC-Byte = $crcfinal","\n"); ### CRC-Byte
print ("Sendecode = $finalStr","\n"); ### Endausgabe
