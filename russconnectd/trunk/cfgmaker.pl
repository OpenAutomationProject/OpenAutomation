#!/usr/bin/perl
use strict;

my $ZONES_PER_CONTROLLER = 6; #static
my @gfuncnames = qw(SetAllZonesPower WRITEONLYACT NA3 NA4 NA5 AllZonesPowerState);
my @funcnames = qw(SetPower SetSource SetVol SetBass SetTreble SetLoudness SetBalance SetParty SetDnD SetOnVol SourceSetCMD SourceKeypadCMD SetVolRelative);
my @statenames = qw(PowerState SourceSelected VolumeState BassState TrebleState LoudnessState BalanceState PartyState DnDState OnVolState);
my $NamePrefix = "Multiroom";
my @SourceNames = qw(UKW-Tuner vdr/mpd Denon-WZ WG1 WG2 Hobby);
### End config - Don't change below ###
#######################################

use File::Copy; 
use Config::Tiny;
#print "install libconfig-tiny-perl!";
use Getopt::Std;
use Data::Dumper;
use XML::Simple;
getopts("a:z:n:f:", \my %opts);
my $conf = Config::Tiny->new;
 
my @funcdpts =     qw (1.001 5.010 5.001 6.001 6.001 1.001 6.001 1.001 1.001 5.001 5.010 5.010 1.008);
my @funcdpts_sub = qw (DPT_Switch DPT_Value_1_Ucount DPT_Scaling DPT_Percent_V8 DPT_Percent_V8 DPT_Switch DPT_Percent_V8 DPT_Switch DPT_Switch DPT_Scaling DPT_Value_1_Ucount  DPT_Value_1_Ucount  DPT_UpDown);
my @statedpts = qw(1.001 5.010 5.001 6.001 6.001 1.001 6.001 1.001 1.001 5.001);
my @statedpts_sub = qw (DPT_Switch DPT_Value_1_Ucount DPT_Scaling DPT_Percent_V8 DPT_Percent_V8 DPT_Switch DPT_Percent_V8 DPT_Switch DPT_Switch DPT_Scaling);

my %EISmap = (
     1.001 => "EIS 1 'Switching' (1 Bit)",
     1.008 => "EIS 1 'Switching' (1 Bit)",
     5.001 => "Uncertain (1 Byte)",
     5.010 => "Uncertain (1 Byte)",
     6.001 => "Uncertain (1 Byte)" );
     
if (!$opts{a} or !$opts{z} or !$opts{n} or $opts{h}) {
	print("\n$0 Tool to create GA-config\nUsage: $0 -a <startaddress> -z <zones> -n <name1,name2,...>\n"
	. "Example: $0 -a 10/1/0 -z 6 -n Küche,Bad,Zone3,Zone4,Zone5,Zone6 >> /etc/wiregate/eibga.conf\n"
	. "Optional to write new config in-place: -f <path-to-eibga.conf> - Example: -f /etc/wiregate/eibga.conf\n");
	exit();
}

if (-e $opts{f}) {
    $conf = Config::Tiny->read($opts{f});
    print STDERR "read config $opts{f}\n";
}

my @zonenames = split(",",$opts{n});
my $startga = str2addr($opts{a},1);

# ESF output for poor guys
open ESF, ">", "export.esf" or die $!;
open XML, ">:encoding(UTF-8)", "linknx.xml" or die $!;

# some visu_config snipplet for CometVisu - :encoding(UTF-8)?? 
open VISU1, ">", "visu_snipplet_rooms.xml"  or die $!;
open VISU2, ">", "visu_snipplet_room_pages.xml"  or die $!;
open VISU3, ">", "visu_snipplet_room_pages_all.xml"  or die $!;

print VISU1 "    <page name=\"$NamePrefix\">\n";

print VISU2 "<!-- Change SourceNames in script and insert this on top between <mappings> and </mappings> -->\n";
print VISU2 "      <mapping name=\"RussoundSRC\">\n"
."        <entry value=\"0\">$SourceNames[0]</entry>\n"
."        <entry value=\"1\">$SourceNames[1]</entry>\n"
."        <entry value=\"2\">$SourceNames[2]</entry>\n"
."        <entry value=\"3\">$SourceNames[3]</entry>\n"
."        <entry value=\"4\">$SourceNames[4]</entry>\n"
."        <entry value=\"5\">$SourceNames[5]</entry>\n"
."      </mapping>\n\n\n";

# Globals
$conf->{addr2str($startga+1,1)}->{'name'} = $NamePrefix . " " . $gfuncnames[0];
$conf->{addr2str($startga+1,1)}->{'DPTSubId'} = 1.001;
$conf->{addr2str($startga+1,1)}->{'DPTId'} = 1;
#$conf->{addr2str($startga+1,1)}->{'DPT_SubTypeName'} = 'DPT_Switch';
print ESF "Multiroom-export\n";
print ESF "$NamePrefix.Controller." . addr2str($startga+1,1) . "\t$NamePrefix $gfuncnames[0]\t" . $EISmap{'1.001'} . "\tLow\n";

print XML "#<?xml version=\"1.0\" ?>\n<config>\n\t<objects>\n";
print XML "\t\t<object id=\"$NamePrefix"."_$gfuncnames[0]\" gad=\"" . addr2str($startga+1,1) . "\" type=\"1.001\">$NamePrefix $gfuncnames[0]</object>\n";

for (my $zone=0;$zone<$opts{z};$zone++) {
	#sendKNXdgram (0x80,1,(knxstartaddress+30)+(zone*40)+(controller*256),val);
	my $ctrl = int($zone/$ZONES_PER_CONTROLLER);
	my $czone = int($zone%$ZONES_PER_CONTROLLER);
	my $basega = $startga+10 + ($czone*40) + ($ctrl*256);
	my $basegastr = addr2str($basega,1);
	print STDERR "Zone " . ($zone+1) . " is $zonenames[$zone] base: $basegastr\n";
	# funcs
	for (my $i=0;$i<$#funcnames+1;$i++) {
        $conf->{addr2str($basega+$i,1)}->{'name'} = "$NamePrefix C" . ($ctrl+1) . "-Z" . ($czone+1) . " $zonenames[$zone] $funcnames[$i]";
        $conf->{addr2str($basega+$i,1)}->{'DPTSubId'} = $funcdpts[$i];
        $conf->{addr2str($basega+$i,1)}->{'DPTId'} = split(".",$funcdpts[$i],1);
        #$conf->{addr2str($basega+$i,1)}->{'DPT_SubTypeName'} = $funcdpts_sub[$i];
        print ESF "$NamePrefix.Controller$ctrl." . addr2str($basega+$i,1) . "\t" . "$NamePrefix $zonenames[$zone] C" . ($ctrl+1) . "-Z" . ($czone+1) . " $funcnames[$i]" . "\t" . $EISmap{$funcdpts[$i]} . "\tLow\n";
        print XML "\t\t<object id=\"$zonenames[$zone]_C" . ($ctrl+1) . "_Z" . ($czone+1) . "_" . "$funcnames[$i]\" gad=\"" . addr2str($basega+$i,1) . "\" type=\"$funcdpts[$i]\">" . "$NamePrefix $zonenames[$zone] (C" . ($ctrl+1) . "/Z" . ($czone+1) . ") $funcnames[$i]" . "</object>\n";
    }
	#states
	for (my $i=0;$i<$#statenames+1;$i++) {
        $conf->{addr2str($basega+$i+20,1)}->{'name'} = "$NamePrefix C" . ($ctrl+1) . "-Z" . ($czone+1) . " $zonenames[$zone] $statenames[$i]";
        $conf->{addr2str($basega+$i+20,1)}->{'DPTSubId'} = $statedpts[$i];
        $conf->{addr2str($basega+$i+20,1)}->{'DPTId'} = split(".",$statedpts[$i],1);
        #$conf->{addr2str($basega+$i+20,1)}->{'DPT_SubTypeName'} = $statedpts_sub[$i];
        print ESF "$NamePrefix.Controller$ctrl." . addr2str($basega+$i+20,1) . "\t" . "$NamePrefix $zonenames[$zone] C" . ($ctrl+1) . "-Z" . ($czone+1) . " $statenames[$i]" . "\t" . $EISmap{$statedpts[$i]} . "\tLow\n";
        print XML "\t\t<object id=\"$zonenames[$zone]_C" . ($ctrl+1) . "_Z" . ($czone+1) . "_" . "$statenames[$i]\" gad=\"" . addr2str($basega+$i+20,1) . "\" type=\"$statedpts[$i]\">" . "$NamePrefix $zonenames[$zone] (C" . ($ctrl+1) . "/Z" . ($czone+1) . ") $statenames[$i]" . "</object>\n";
    }
    print VISU1 '      <switch mapping="OnOff" styling="GreenRed">'
             ."\n        <label>$zonenames[$zone]</label>\n"
               .'        <address transform="DPT:1.001" readonly="false" type="">' . addr2str($basega,1) . "</address>\n"
               .'        <address transform="DPT:1.001" readonly="true" type="">' .  addr2str($basega+20,1) . "</address>\n"
               ."      </switch>\n";
    print VISU1 '      <slide min="0" max="100">'
             ."\n        <label>$zonenames[$zone] Vol</label>\n"
               .'        <address transform="DPT:5.001" readonly="false" type="">' . addr2str($basega+2,1) . "</address>\n"
               .'        <address transform="DPT:5.001" readonly="true" type="">' .  addr2str($basega+22,1) . "</address>\n"
               ."      </slide>\n";
               
    print VISU2 "    <page name=\"$NamePrefix $zonenames[$zone]\">\n";
    print VISU2 '      <switch mapping="OnOff" styling="GreenRed">'
             ."\n        <label>Power</label>\n"
               .'        <address transform="DPT:1.001" readonly="false" type="">' . addr2str($basega,1) . "</address>\n"
               .'        <address transform="DPT:1.001" readonly="true" type="">' .  addr2str($basega+20,1) . "</address>\n"
               ."      </switch>\n";
    print VISU2 '      <slide min="0" max="100">'
             ."\n        <label>Volume</label>\n"
               .'        <address transform="DPT:5.001" readonly="false" type="">' . addr2str($basega+2,1) . "</address>\n"
               .'        <address transform="DPT:5.001" readonly="true" type="">' .  addr2str($basega+22,1) . "</address>\n"
               ."      </slide>\n";
    #$SourceNames[0]
    print VISU2 '      <infotrigger uplabel="&gt;" upvalue="1" downlabel="&lt;" downvalue="-1" align="center" infoposition="1" change="absolute" min="0" max="5" mapping="RussoundSRC">'
#    <infotrigger uplabel="&gt;" upvalue="1" downlabel="&lt;" downvalue="-1" align="center" infoposition="1" change="absolute" min="0" max="5" mapping="RussoundSRC">
#      <label>Source</label>
#      <address transform="DPT:5.010" readonly="false" variant="">10/1/11</address>
#      <address transform="DPT:5.010" readonly="true" variant="">10/1/31</address>
#    </infotrigger>
             ."\n        <label>Quelle</label>\n"
               .'        <address transform="DPT:5.010" readonly="false">' . addr2str($basega+1,1) . "</address>\n"
               .'        <address transform="DPT:5.010" readonly="true">' .  addr2str($basega+21,1) . "</address>\n"
               ."      </infotrigger>\n";
    print VISU2 '      <switch mapping="OnOff" styling="GreenRed">'
             ."\n        <label>Loudness</label>\n"
               .'        <address transform="DPT:1.001" readonly="false" type="">' . addr2str($basega+5,1) . "</address>\n"
               .'        <address transform="DPT:1.001" readonly="true" type="">' .  addr2str($basega+25,1) . "</address>\n"
               ."      </switch>\n";
    print VISU2 '      <infotrigger button1label="+" button1value="1" button2label="-" button2value="0" align="center" infoposition="1" format="%.0f %% ">'
             ."\n        <label>Volume</label>\n"
               .'        <address transform="DPT:1.008" readonly="false" variant="button">' . addr2str($basega+12,1) . "</address>\n"
               .'        <address transform="DPT:5.001" readonly="true" variant="">' . addr2str($basega+2,1) . "</address>\n"
               .'        <address transform="DPT:5.001" readonly="true" variant="">' . addr2str($basega+22,1) . "</address>\n"
               ."      </infotrigger>\n";
    print VISU2 '      <infotrigger align="center" infoposition="1" upvalue="2" downvalue="-2" change="absolute" format="%.0f %%">'
             ."\n        <label>TurnOn Volume</label>\n"
               .'        <address transform="DPT:5.001" readonly="false" type="">' . addr2str($basega+9,1) . "</address>\n"
               .'        <address transform="DPT:5.001" readonly="true" type="">' .  addr2str($basega+29,1) . "</address>\n"
               ."      </infotrigger>\n";
#    print VISU2 '      <slide min="0" max="100">'
#             ."\n        <label>TurnOn Vol</label>\n"
#               .'        <address transform="DPT:5.001" readonly="false" type="">' . addr2str($basega+9,1) . "</address>\n"
#               .'        <address transform="DPT:5.001" readonly="true" type="">' .  addr2str($basega+29,1) . "</address>\n"
#               ."      </slide>\n";
    print VISU2 '      <infotrigger align="center" infoposition="1" upvalue="1" downvalue="-1" change="absolute" min="-10" max="10">'
             ."\n        <label>Bass</label>\n"
               .'        <address transform="DPT:6.001" readonly="false" type="">' . addr2str($basega+3,1) . "</address>\n"
               .'        <address transform="DPT:6.001" readonly="true" type="">' .  addr2str($basega+23,1) . "</address>\n"
               ."      </infotrigger>\n";
    print VISU2 '      <infotrigger align="center" infoposition="1" upvalue="1" downvalue="-1" change="absolute" min="-10" max="10">'
             ."\n        <label>Treble</label>\n"
               .'        <address transform="DPT:6.001" readonly="false" type="">' . addr2str($basega+4,1) . "</address>\n"
               .'        <address transform="DPT:6.001" readonly="true" type="">' .  addr2str($basega+24,1) . "</address>\n"
               ."      </infotrigger>\n";
    print VISU2 '      <infotrigger align="center" infoposition="1" upvalue="1" downvalue="-1" change="absolute" min="-10" max="10">'
             ."\n        <label>Balance</label>\n"
               .'        <address transform="DPT:6.001" readonly="false" type="">' . addr2str($basega+6,1) . "</address>\n"
               .'        <address transform="DPT:6.001" readonly="true" type="">' .  addr2str($basega+26,1) . "</address>\n"
               ."      </infotrigger>\n";
    print VISU2 '      <switch mapping="OnOff" styling="GreenRed">'
             ."\n        <label>Party</label>\n"
               .'        <address transform="DPT:1.001" readonly="false" type="">' . addr2str($basega+7,1) . "</address>\n"
               .'        <address transform="DPT:1.001" readonly="true" type="">' .  addr2str($basega+27,1) . "</address>\n"
               ."      </switch>\n";
    print VISU2 '      <switch mapping="OnOff" styling="GreenRed">'
             ."\n        <label>DnD</label>\n"
               .'        <address transform="DPT:1.001" readonly="false" type="">' . addr2str($basega+8,1) . "</address>\n"
               .'        <address transform="DPT:1.001" readonly="true" type="">' .  addr2str($basega+28,1) . "</address>\n"
               ."      </switch>\n";
    print VISU2 "    </page>\n";
}

close ESF;
print XML "\t</objects>\n</config>\n";
close XML;

print VISU1 "    </page>\n";
close VISU1;
close VISU2;
close VISU3;

#print "CONF:" . Dumper($conf);
if (-e $opts{f}) { # wite config in place
    copy($opts{f},$opts{f} . '.bak' );
    $conf->write( $opts{f} . '.new' );
    move( $opts{f} . '.new',$opts{f} );
} else { # dump to STDOUT
    $conf->write( '-' );
}

######
# END
######
### copied global funcs
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
