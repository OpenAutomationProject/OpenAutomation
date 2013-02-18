#return;
use warnings;
use strict;
use Net::Telnet ();

### READ PLUGIN-CONF
my ($config,$ip,$port,$base_time,$debug);
&readConf;
$debug = 1;

$plugin_info{$plugname.'_cycle'} = 60;
$plugin_info{$plugname.'_number'} = 0 unless (defined $plugin_info{$plugname.'_number'});
my (@gets,@sets,$answer);

### READ CONFIG
### FIXME! try to read config only once until changed
### Where to save the arrays ? Maybe global in knxd?
my $id = 0;
open (CFG,'<',$config) || die $!;
while (<CFG>){
if ($_ =~ /\#/){
#if ($debug){plugin_log($plugname,"skipped line");}
}
else
{
chomp $_;
#if ($debug){plugin_log($plugname,"line $_");}
my @array = split (/;/,$_);
my ($ga,$dpt,$rrd_type,$rrd_step,$type,$short,$comment) = @array;

#possible this prevents a memleak
@array = ();
undef @array;
#maybe !?

###define @gets### includes cyclic
if ($type ne "set" && ($ga or $rrd_type)){
    $id++;
    push @gets,{ga => $ga, dpt => $dpt, rrd_type => $rrd_type, rrd_step => $rrd_step, type => $type, short => $short, comment => $comment, id => $id};
}
###define @sets###
if ($type eq "set" && $ga){
    push @sets,{ga => $ga, dpt => $dpt, rrd_type => $rrd_type, rrd_step => $rrd_step, type => $type, short => $short, comment => $comment, id => $id};
}
}
}
close CFG;

### CREATE DYNAMIC PLUGIN-CYCLE
$plugin_info{$plugname.'_cycle'} = $base_time/(@gets+1);
my $commands = @gets;
if ($debug){plugin_log($plugname,"Anzahl GET-Komandos: $commands");}

#### SET ###
foreach my $set(@sets){
$plugin_subscribe{$set->{ga}}{$plugname} = 1;
if ($debug){plugin_log($plugname,"$set->{ga} subscribed \n")};
}

foreach my $set(@sets){
if ($msg{'apci'} eq "A_GroupValue_Write" && $msg{'dst'} eq $set->{ga} && defined $msg{'value'}){
	my $send_set = $set->{type}." ".$set->{short}." ".$msg{'value'} ;
	plugin_log($plugname,$send_set);
	$answer = send_ebusd ($send_set);
	chomp $answer;	
	$answer =~ s!\s!!g;
	plugin_log($plugname,"$set->{type} $set->{short} $msg{'value'} $answer");
	
	### Check for Response
	foreach my $get (@gets){
		if ($get->{short} eq $set->{short}){
			my $send_get = $get->{type}." ".$get->{short};
			$answer = send_ebusd ($send_get);	
			$answer =~ s!\s!!g;
			plugin_log($plugname,"$get->{type} $get->{short} $answer");
			if ($answer =~ m/^[-+]?\d+(?:\.\d*)?(?:[eE][-+]?\d+(?:\.\d*)?)?$/ ) # check if $answer ist any number
			{knx_write($get->{ga},$answer,$get->{dpt});
			plugin_log($plugname,"$get->{ga} $answer $get->{dpt}");
			}
			#print $sock ("quit \n");
			#return;
			}}
	return;
	}
}

####Response
####FIXME !
#### No surprise ... ists to slow ... but working
#foreach my $get (@gets){
#$plugin_subscribe{$get->{ga}}{$plugname} = 1;
#if ($msg{'apci'} eq "A_GroupValue_Read" && $msg{'dst'} eq $get->{ga}){
#	plugin_log($plugname,"Response $msg{'apci'}");
#	my $send_get = $get->{type}." ".$get->{short};
#	$answer = send_ebusd ($send_get);
#	chomp $answer;	
#	$answer =~ s!\s!!g;
#	plugin_log($plugname,"Response $get->{short} $answer");
#	if ($answer =~ m/^[-+]?\d+(?:\.\d*)?(?:[eE][-+]?\d+(?:\.\d*)?)?$/ ) # check if $answer ist any number
#		{knx_write($get->{ga},$answer,$get->{dpt},0x40)}
#}
#}

#### GET ###

$plugin_info{$plugname.'_number'}++; #increase number to read next value

foreach my $get (@gets){
if ($plugin_info{$plugname.'_number'} > $commands) {$plugin_info{$plugname.'_number'} = 1}
if ($get->{id} == $plugin_info{$plugname.'_number'}){
	if ($debug){plugin_log($plugname,"ID: $get->{id}");}
	if ($debug){plugin_log($plugname,"$get->{short}")};	
	my $send_get = $get->{type}." ".$get->{short};
        $answer = send_ebusd ($send_get);
	chomp $answer;	
	$answer =~ s!\s!!g;
	plugin_log($plugname,"$get->{type} $get->{short} $answer");
	if ($answer =~ m/^[-+]?\d+(?:\.\d*)?(?:[eE][-+]?\d+(?:\.\d*)?)?$/ ) # check if $answer ist any number
		###SEND KNX###
        	{knx_write($get->{ga},$answer,$get->{dpt})}
        
        ###FILL/CREATE RRD###
        if ($get->{rrd_type} eq "c"){   #matches COUNTER
            $get->{short} =~ s/ /_/g;   #replace spaces
            update_rrd ("eBus_".$get->{short},"",$answer,"COUNTER",$get->{rrd_step})
            }
        if ($get->{rrd_type} eq "g"){   #matches GAUGE
            $get->{short} =~ s/ /_/g;   #replace spaces
            update_rrd ("eBus_".$get->{short},"",$answer)
            }      

   # print $sock ("quit \n");
    #close $sock;
}
}

sub send_ebusd{
    my $cmd = shift;
    my $t = new Net::Telnet (Timeout => 10,
                    port => $port,
                    Prompt => '/\n/');
    $t->open($ip);
    if ($debug){plugin_log($plugname,"Sende:$cmd")};
    my @answer = $t->cmd($cmd);
    $answer = $answer[0];
    $t->close;
    return $answer;
}

####possible this prevents a memleak
# @sets = (); 
# @gets = ();
# $sock = ();

# undef @gets;
# undef @sets;
# undef $sock;
####maybe !?

return sprintf("%.2f",$plugin_info{$plugname.'_meminc'})." mb lost";

### READ CONF ###
sub readConf
{
 my $confFile = '/etc/knxd/eBus_plugin.conf';
 if (! -f $confFile) {
  plugin_log($plugname, "no conf file [$confFile] found."); 
 } else {
  #plugin_log($plugname, "reading conf file [$confFile]."); 
  open(CONF, $confFile);
  my @lines = <CONF>;
  close($confFile);
  my $result = eval("@lines");
  #($result) and plugin_log($plugname, "conf file [$confFile] returned result[$result]");
  if ($@) {
   plugin_log($plugname, "ERR: conf file [$confFile] returned:");
   my @parts = split(/\n/, $@);
   plugin_log($plugname, "--> $_") foreach (@parts);
  }
 }
}