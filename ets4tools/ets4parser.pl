#!/usr/bin/perl -w
# a simple ETS4 project-parser, just a skeleton playing with and printing out some data
#
# (C) 2011-2012 Michael Markstaller <devel@wiregate.de> / Elaborated Networks GmbH
# All Rights Reserved
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

use XML::Simple;
use Data::Dumper;
use POSIX;
binmode(STDOUT, ":utf8");

no warnings qw(uninitialized);

if (! glob("P-*/0.xml")) { # unzip
	my $projectfile = glob("*.knxproj");	
	`unzip -o $projectfile`;
	print "unzipped $projectfile\n";
}

my $file0 = glob("P-*/0.xml");
open(my $fh, $file0) || die "can't open file: $!";
open(my $knxmfh, 'knx_master.xml') || die "can't open file: $!";

#special case: only one MG/GA etc. fixed with forcearray
my $proj = XMLin($fh, ForceArray => ['GroupRange','GroupAddress','Area','Line','DeviceInstance','ComObjectInstanceRef','Send','Receive']);
my $knxmaster = XMLin($knxmfh, KeyAttr => "Id");
$Data::Dumper::Indent = 1; 
#print Dumper($proj->{Project}->{Installations}->{Installation}->{GroupAddresses});
#print Dumper($knxmaster);

my %groupaddresses;

for my $HG ( @{ $proj->{Project}->{Installations}->{Installation}->{GroupAddresses}->{GroupRanges}->{GroupRange} } ) {
   print floor($HG->{RangeStart}/2048);  
	print " $HG->{Name}\n";
	for my $MG ( @{$HG->{GroupRange}} ) {
		print "    ";
 	   print floor(($MG->{RangeEnd}-$HG->{RangeStart})/256);  
		print " $MG->{Name}\n";
		for my $GA ( @{$MG->{GroupAddress}} ) {
			print "        ";
	 	   print $GA->{Address};  
			print " $GA->{Name} $GA->{Id}\n";
			$groupaddresses{$GA->{Id}}{'Address'} = $GA->{Address};
			$groupaddresses{$GA->{Id}}{'Name'} = $GA->{Name};
		}
	}
}

print "\nTopology\n\n";

for my $Area ( @{ $proj->{Project}->{Installations}->{Installation}->{Topology}->{Area} } ) {
	print "Area $Area->{Address} $Area->{Name} \n";
	parseDevices($Area);
	for my $Line ( @{ $Area->{Line} } ) {
		print "Line $Area->{Address}.$Line->{Address} $Line->{Name} \n";
		#Parse devices
		parseDevices($Line);
	}
}

sub parseDevices {
	$line = shift;
	return unless defined $line->{DeviceInstance};
	for my $Device ( @{ $line->{DeviceInstance} } ) {
		print "   Device $Device->{Address} $Device->{Description} \n";
		for my $Comobj ( @{$Device->{ComObjectInstanceRefs}->{ComObjectInstanceRef}} ) {
			# if $Comobj->{DatapointType} is undef we could now look on ProductRefId="M-0001_H-hp.5F00199-1_P-5WG1.20141.2D1AB01" Hardware2ProgramRefId="M-0001_H-hp.5F00199-1_HP-8027-01-79C2" 
			# open M-xxxx/Hardware.xml - get the real product-xml, open M-0001_A-8027-01-79C2.xml
			# and get at least the size from there: ComObjectTable->ComObject->ObjectSize
			for my $Conn ( @{ $Comobj->{Connectors}->{Send} } ) {
				$groupaddresses{$Conn->{GroupAddressRefId}}{'DatapointType'} = $Comobj->{DatapointType} if defined $Comobj->{DatapointType};
				print "     GA ".addr2str($groupaddresses{$Conn->{GroupAddressRefId}}{Address},1) ." :". $Comobj->{DatapointType} ."\n";
			}
			for my $Conn ( @{ $Comobj->{Connectors}->{Receive} } ) {
				$groupaddresses{$Conn->{GroupAddressRefId}}{'DatapointType'} = $Comobj->{DatapointType} if defined $Comobj->{DatapointType};
				print "     GA ".addr2str($groupaddresses{$Conn->{GroupAddressRefId}}{Address},1) ." :". $Comobj->{DatapointType} ."\n";
			}
		}
	}
}

# dump resulting GA-Hash
for (keys %groupaddresses) {
	print "GA " . addr2str($groupaddresses{$_}{Address},1);
	print "\t " . $groupaddresses{$_}{Name};
	# DatapointType can be DPT or DPST(subtype)
	if ($groupaddresses{$_}{DatapointType} =~ m/^DPT/) {
		printf("\t %d.000 \t",$knxmaster->{MasterData}->{DatapointTypes}->{DatapointType}->{$groupaddresses{$_}{DatapointType}}->{Number});
		print $knxmaster->{MasterData}->{DatapointTypes}->{DatapointType}->{$groupaddresses{$_}{DatapointType}}->{Text};
	} elsif ($groupaddresses{$_}{DatapointType} =~ m/^DPST/) {
		my ($dummy,$parentDPT,$subDPT) = split(/-/,$groupaddresses{$_}{DatapointType},3);
		printf("\t %d.%03d \t",$parentDPT,$subDPT);
		print $knxmaster->{MasterData}->{DatapointTypes}->{DatapointType}->{"DPT-".$parentDPT}->{DatapointSubtypes}->{DatapointSubtype}->{$groupaddresses{$_}{DatapointType}}->{Name};
		print "\t" . $knxmaster->{MasterData}->{DatapointTypes}->{DatapointType}->{"DPT-".$parentDPT}->{DatapointSubtypes}->{DatapointSubtype}->{$groupaddresses{$_}{DatapointType}}->{Text};
	} else { #guess default
		print "\tis unknown";		
		# assume default 1.001
	}
	print "\n";
	# fill own config etc here
}

print "ETS: ". $proj->{ToolVersion} ."\n";

#Version-check 4.0.1387.12605
my @etsversion = split(/\./, $proj->{ToolVersion});
# changed from 4.0 to 4.1 > 4.1.2 (Build 3013)
if ($etsversion[1] < 1) {
	print "error ETS " . $proj->{ToolVersion} . " too old - must be 4.1 or higher\n";
	exit;
}


# addr2str: Convert an integer to an EIB address string, in the form "1/2/3" or "1.2.3"
sub addr2str {
    my $a = $_[0];
    my $b = $_[1] || 0;  # 1 if local (group) address, else physical address
    my $str ;
    if ($b == 1) { # logical address used
        $str = sprintf "%d/%d/%d", ($a >> 11) & 0xf, ($a >> 8) & 0x7, $a & 0xff;
    }
    else { # physical address used
        $str = sprintf "%d.%d.%d", $a >> 12, ($a >> 8) & 0xf, $a & 0xff;
    }
    return $str;
}

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

