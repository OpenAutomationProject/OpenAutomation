#!/usr/bin/perl
#perl /root/make_ebus_config.pl -i /root/ebus2/ebusd/vaillant.csv -o /tmp/test2.csv
# makes config from ebusd cvs-files
# M.Hirsch
# 2013

use warnings;
use strict;
use Getopt::Std;

getopts('o:i:h:', \my %opts) or die "Bitte Pfade für Config-File angeben!\n -i select ebusd-configfile (input) \n-o select plugin output-file (output) \n";

if ($opts{i}) {print "Input: $opts{i}\n"};
my $config = $opts{i};
if ($opts{o}) {print "Output $opts{o}\n"};
my $final_cfg = $opts{o};

if ($opts{h}) {print "Make config-file \n -h This page \n -i select ebusd-configfile (input) \n-o select plugin output-file (output) \n"};

### READ CONFIG
my @cmds;
open (GETCFG,"<$config") || die $!;
while (<GETCFG>){
if ($_ =~ /\#/) {
    #if ($debug){plugin_log($plugname,"skipped line");}
    } else {
    #if ($debug){plugin_log($plugname,"line $_");}

my @array = split (/;/,$_);
my $type = $array[0];
my $class = $array[1];
my $comment = $array[3];
my $elements = $array[9];
my $cmd;
if ($elements > 1) {
    my $i=0;
    while ($i < $elements){
        $i++;
        if ($i==1) {
        $cmd = $class." ".$array[2]." ".$array[10];
        push @cmds,{type => $type, cmd => $cmd, comment => $comment};
        }
        if ($i>1) {
        $cmd = $class." ".$array[2]." ".$array[2+($i*8)];
        #print $type." ".$cmd." ".$comment."\n";
        push @cmds,{type => $type, cmd => $cmd, comment => $comment};
        }}
    } else {
    
    $cmd = $class." ".$array[2];
    #print $type." ".$cmd." ".$comment."\n";
    push @cmds,{type => $type, cmd => $cmd, comment => $comment};
    }
}
}
close GETCFG;

open (WRITECFG,'>',$final_cfg) || die "Can not open file $final_cfg: $!";;
print
"\#\#\#\#\#\#\#\#\#\#\#\#\#\#\#
\#\#\# I N F O \#\#\#
\#\#\#\#\#\#\#\#\#\#\#\#\#\#\#\n";
print "All commands are disabled through \"\#\" by default\n";

print "\#GA;DPT;RRD_TYPE;RRD_STEP;TYPE;CMD;COMMENT\n";
print WRITECFG "\#GA;DPT;RRD_TYPE;RRD_STEP;TYPE;CMD;COMMENT\n";
foreach my $cmd (@cmds){
    print WRITECFG "\#;;;;$cmd->{type};$cmd->{cmd};$cmd->{comment} \n";
    print "\#;;;;$cmd->{type};$cmd->{cmd};$cmd->{comment} \n";
    }
close WRITECFG;