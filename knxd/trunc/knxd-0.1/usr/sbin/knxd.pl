#!/usr/bin/perl -w
# KNX Daemon - handling WireGate-Plugins and EIB
# based on wiregated.pl by Michael Markstaller
#
# Cleanup and partially rewritten by M.Hirsch
# http://knx-user-forum.de -> JuMi2006
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

use strict;
use Math::Round;
use Math::BaseCalc;
use RRDs;
use Getopt::Std;
use Time::HiRes qw ( time alarm sleep usleep gettimeofday );
getopts("dp:c:e:o:", \my %opts);
use IO::Socket;
use IO::Select;
use FileHandle;
use Proc::PID::File;
# fork daemon
use Proc::Daemon;
Proc::Daemon::Init unless $opts{d};
#use xPL::Client;
use EIBConnection;
use feature "switch";
use File::Basename;
use DB_File;

no warnings 'uninitialized';

sub LOGGER;

# Options
while ((my $key, my $value) = each %opts) {
    print "opt $key = $value\n" if ($opts{d});
}

# PID File-handling
if ($opts{p} and Proc::PID::File->running()) {
    LOGGER("WARN","Already running");
    die();
}

# Config files
# global
my $config;
if ($opts{c}) {
    LOGGER("INFO","Config-File is: $opts{c}");
    $config = $opts{c};
} else {$config = "/etc/knxd/knxd.conf" }

# eibga
my $eib_config;
if ($opts{c}) {
    LOGGER("INFO","eibga-File is: $opts{e}");
    $eib_config = $opts{e};
} else {$eib_config = "/etc/knxd/eibga.conf" }

# Read config #
my %daemon_config;
my $lastreadtime = 0;
my %eibgaconf;
my $udp_sock;
my %ga_to_ow;
my @eibga_types = qw (temp hum moisthum present light volt curr ch0 ch1 ch2 ch3 ch4 ch5 ch6 ch7 countA countB );

if ((stat($config))[9] > $lastreadtime or (stat($eib_config))[9] > $lastreadtime) {
    if (-e $config) {
        LOGGER('INFO',"*** reading global config");
        my_read_cfg ($config, \%daemon_config);
        $lastreadtime = time();
    }
    if (-e $eib_config) {
        LOGGER('INFO',"*** reading eibga config");
        my_read_cfg ($eib_config,\%eibgaconf);
    }
} else {
    #LOGDIE("unable to read config $config");
    print "unabled to read config $config";
}


# Definitions

# config values

my $cycle = $daemon_config{''}{'cycle'};
my $sleeptime = $daemon_config{''}{'sleeptime_ms'};
my $plugin_cycle = $daemon_config{''}{'plugin_cycle'};
my $rrd_interval = $daemon_config{''}{'rrd_interval'};
my $rra1_row = $daemon_config{''}{'rra_1_interval_rows'};
my $rra5_row = $daemon_config{''}{'rra_5_interval_rows'};
my $rra15_row = $daemon_config{''}{'rra_15_interval_rows'};
my $rra180_row= $daemon_config{''}{'rra_180_interval_rows'};
my $eib_logging = $daemon_config{''}{'eib_logging'};
my $eib = $daemon_config{''}{'eib'};

my $mem_max = $daemon_config{''}{'mem_max'};
my $name = $daemon_config{''}{'name'};
my $eib_url = $daemon_config{''}{'eib_url'};
my $rrd_dir = $daemon_config{''}{'rrd_dir'};
my $eiblogfile = $daemon_config{''}{'eiblogfile'};

my $plugin_logfile = $daemon_config{''}{'plugin_logfile'};
my $plugin_infofile = $daemon_config{''}{'plugin_infofile'};
my $plugin_db_name = $daemon_config{''}{'plugin_db_name'};
my $plugin_db_path = $daemon_config{''}{'plugin_db_path'};

my $ramdisk = $daemon_config{''}{'ramdisk'};
my $alive = $daemon_config{''}{'alive'};
my $plugin_path = $daemon_config{''}{'plugin_path'};
my $init_script = $daemon_config{''}{'init_script'};

my $knxdebug = 0;

# Plugins

my %plugin_info;
my %plugin_subscribe;
my %plugin_subscribe_write;
my %plugin_subscribe_read;
my $plugin_initflag=0;
my %plugin_socket_subscribe;
my %plugin_cache=(); # non-persistent but faster memory for plugins
my %plugin_code=(); # here, the plugins are stored as closures (unnamed subs)

# global definitions

my $conv2bin = new Math::BaseCalc(digits => 'bin'); #Binary
my $telegram_count = 0;
my $telegram_bytes;
my %msg;
my $fh;

my @socket;
my $socksel;

my $looptime = time();
my $looptimeavg = 0;
my $laststatesaveime = 0;
my $lastdirtime = 0;
my $lastlooptime = 0;
my $eib_tps = 50;
my $telegram_count_check = 100;
my $sendtime_last = 0;
my $running = 1;
my $thr_eiblisten_timeout = 0;
my $thr_eiblisten_cause = "";

sub getISODateStamp;


### MAIN ###

system ("touch ".$alive); # avoid timeout for init

LOGGER('INFO',"Started with PID: $$ \n");

$cycle=60 unless (defined $cycle and $cycle > 29); # minimum cycle 30 to avoid insane settings
$sleeptime = 500 unless defined $sleeptime;
$plugin_cycle= 60 unless defined $plugin_cycle;
$eib_logging= 1 unless defined $eib_logging;
$rrd_interval = 300 unless defined $rrd_interval;
$rra1_row = 2160 unless defined $rra1_row;
$rra5_row = 2016 unless defined $rra5_row;
$rra15_row = 2880 unless defined $rra15_row;
$rra180_row = 8760 unless defined $rra180_row;

my $rrd_lastupdate = time()-($rrd_interval-+1);
my $rrd_syslastupdate = time()-($rrd_interval-+1);

# Copy  Plugin-DB to ramdisk
if (! -e $ramdisk.$plugin_db_name) {
    `cp $plugin_db_path$plugin_db_name $ramdisk$plugin_db_name`; }
my $plugindb = tie %plugin_info, "DB_File", $ramdisk.$plugin_db_name, O_RDWR|O_CREAT, 0666, $DB_HASH
or LOGGER('WARN',"Cannot open file 'plugin_info': $!");


# GO
&eiblisten_thread;

#######################################
#### K N O W N   S U B S #####
#######################################
sub eiblisten_thread { # MAIN SUB
    $SIG{TERM} = sub {
        # -> Doesn't work when blocked by I/O!!
        LOGGER('DEBUG',"Thread eiblisten Caught TERM, exiting:". $thr_eiblisten_cause);
        system ("touch $alive"); # avoid timeout for init
        if($eib_logging) { close FILE_EIBLOG; }
        $plugindb->sync(); # write out
        `cp $ramdisk$plugin_db_name $plugin_db_path$plugin_db_name`;
        exit();
    };
    $SIG{KILL} = sub {
        # ende aus finito
        LOGGER('DEBUG',"Thread eiblisten Caught KILL, exiting:" .$thr_eiblisten_cause);
        system ("touch $alive"); # avoid timeout for init
        if($eib_logging) { close FILE_EIBLOG; }
        $plugindb->sync(); # write out
        `cp $ramdisk$plugin_db_name $plugin_db_path$plugin_db_name`;
        exit();
    };
    
    #FIXME: add plugin-timeout again with SIGALRM, currently we rely on monit (1 minute)
    
    if($eib_logging) {
        open FILE_EIBLOG, ">>$eiblogfile";
        print "Muss jetzt eibloggen";
    }
    
    # make it hot & define local vars - prevents Highload for eib = 0
    my $retryinfo;
    my $lastplugintime;
    my $select = IO::Select->new();
    $socksel = IO::Select->new();
    my $eibpoll;
    my $eibsrc;
    my $eibdst;
    my $eibbuf2;
    
    if ($eib == 1){
        select((select(FILE_EIBLOG), $|=1)[0]);
        $select = IO::Select->new();
        $socksel = IO::Select->new();
        $eibpoll = 1;
        $eibsrc=EIBConnection::EIBAddr();
        $eibdst=EIBConnection::EIBAddr();
        $eibbuf2=EIBConnection::EIBBuffer();
    }
    
    while ($running) {
        my $eibcon2;
        if ($eib == 1){
            $eibcon2 = EIBConnection->EIBSocketURL($eib_url);
            if (defined($eibcon2)) {
                $eibcon2->EIB_Cache_Enable();
                my $busmon = $eibcon2->EIBOpenVBusmonitor_async();
                LOGGER('INFO',"connected to eibd $eib_url");
            } else {
                LOGGER('INFO',"Cannot connect to eibd $eib_url") unless $retryinfo;
            }
        }
        
        while (($eibpoll and defined($eibcon2)) or ($eib == 0))  #try to disable eib
        {
            if ($eib ==1){
                $select->add($eibcon2->EIB_Poll_FD);
                while ($select->can_read(.1)) { # EIB Packet processing - patched by Fry
                    if ($eibpoll=$eibcon2->EIB_Poll_Complete) {
                        my $msglen=$eibcon2->EIBGetBusmonitorPacket($eibbuf2);
                        if ($msglen>1) {
                            %msg = decode_vbusmonitor($$eibbuf2);
                            $fh=undef;
                            $retryinfo = 0;
                            # case ReadRequest for configured owsensor-value
                            # if ($msg{'apci'} eq "A_GroupValue_Read" and $ga_to_ow{$msg{'dst'}}) {
                            # LOGGER('DEBUG',"---> Send reply for $msg{'dst'} $ga_to_ow{$msg{'dst'}}");
                            # send_sensor_response($msg{'dst'});
                            # }
                            # # case Write for PIO
                            # if ($msg{'apci'} eq "A_GroupValue_Write" and $ga_to_ow{$msg{'dst'}}) {
                            # set_sensor_value($msg{'dst'},$msg{'data'});
                            # }
                            
                            # Log it
                            print FILE_EIBLOG getISODateStamp.",$msg{'apci'},$msg{'src'},$msg{'dst'},$msg{'data'},$msg{'value'}"
                            .",$eibgaconf{$msg{'dst'}}{'DPT_SubTypeName'},$eibgaconf{$msg{'dst'}}{'DPTSubId'},$msg{'repeated'}"
                            .",$msg{'class'},$msg{'rcount'},$msg{'tpdu_type'},$msg{'sequence'}\n" if($eib_logging);
                            # Check EIB-connectivity and configchange every 100 telegrams?
                            $telegram_count++;
                            
                            # Traffic on Bus in bytes (8+Nutzdaten)+ACK?
                            $telegram_bytes += 9+(length(join('',split / /,$msg{'data'}))/2);
                            #LOGGER('DEBUG',"Anz $telegram_count, Bytes $telegram_bytes");
                            
                            if ($msg{'apci'} eq "A_GroupValue_Read" and ($msg{'dst'} eq $daemon_config{''}{'sendtime_ga_time'}))  {
                                # send time response
                                eibsend_time_resp($daemon_config{''}{'sendtime_ga_time'});
                            }
                            if ($msg{'apci'} eq "A_GroupValue_Read" and ($msg{'dst'} eq $daemon_config{''}{'sendtime_ga_date'}))  {
                                # send time response
                                eibsend_date_resp($daemon_config{''}{'sendtime_ga_date'});
                            }
                            # check subscribed plugin
                            if ($plugin_subscribe{$msg{'dst'}}) {
                                #while( my ($k, $v) = each($plugin_subscribe{$msg{'dst'}}) ) {
                                for my $k ( keys %{$plugin_subscribe{ $msg{'dst'} }} ) {
                                    LOGGER('DEBUG',"Running Plugin $k subscribed to $msg{'dst'}");
                                    $thr_eiblisten_timeout = time(); # set timeout
                                    check_generic_plugins($k);
                                }
                            }
                            # check subscribed plugin for READ
                            if ($plugin_subscribe_read{$msg{'dst'}} && $msg{'apci'} eq "A_GroupValue_Read") {
                                #while( my ($k, $v) = each($plugin_subscribe{$msg{'dst'}}) ) {
                                for my $k ( keys %{$plugin_subscribe_read{ $msg{'dst'} }} ) {
                                    LOGGER('DEBUG',"Running Plugin $k subscribed to $msg{'dst'}");
                                    $thr_eiblisten_timeout = time(); # set timeout
                                    check_generic_plugins($k);
                                }
                            }
                            # check subscribed plugin for WRITE
                            if ($plugin_subscribe_write{$msg{'dst'}} && $msg{'apci'} eq "A_GroupValue_Write") {
                                #while( my ($k, $v) = each($plugin_subscribe{$msg{'dst'}}) ) {
                                for my $k ( keys %{$plugin_subscribe_write{ $msg{'dst'} }} ) {
                                    LOGGER('DEBUG',"Running Plugin $k subscribed to $msg{'dst'}");
                                    $thr_eiblisten_timeout = time(); # set timeout
                                    check_generic_plugins($k);
                                }
                            }
                            
                            #possible this prevents a memleak
                            %msg = ();
                            undef %msg;
                            #maybe !?
                        } elsif ($msglen==414) {
                            # process other?
                        }
                    }
                }
                # EIB Packet processing end, back in main loop
                
                #possible this prevents a memleak
                $select->remove($eibcon2->EIB_Poll_FD);
                #maybe !?
            }
            ###LOG continious Memory if DEBUG
            #if ($opts{d}) {
            #    my @statinfo = get_stat_info($$);
            #    LOGGER('DEBUG',"BOSS utime: " . $statinfo[0]/100 ." stime " . $statinfo[1]/100 ." rss: $statinfo[5]");
            #}
            #FIXME: output log-warning on memleak
            system ("touch $alive");
            
            # check plugins
            if (time()-$lastplugintime > 1) { # at most every 1secs for now
                $thr_eiblisten_timeout = time(); # set timeout
                %msg=(); $fh=undef;
                check_generic_plugins();
                $lastplugintime = time();
                # case update rrds
                check_global_rrd_update();
                #Cleanup %plugin_subscribe from deleted!!!
            }
            if ($rrd_syslastupdate + $rrd_interval < time()) {
                my @statinfo = get_stat_info($$);
                update_rrd($name."_mem","",$statinfo[5]);
                update_rrd($name."_stime","",$statinfo[0],"COUNTER");
                update_rrd($name."_utime","",$statinfo[1],"COUNTER");
                update_rrd($name."_cutime","",$statinfo[2],"COUNTER");
                update_rrd($name."_cstime","",$statinfo[3],"COUNTER");
                $rrd_syslastupdate = time();
                
                # save plugin_db from ramdisk
                $plugindb->sync(); # write out
                `cp $ramdisk$plugin_db_name $plugin_db_path$plugin_db_name`;
                
                # if own memory is above mem_max -> restart via init.d
                if ($statinfo[5] >= $mem_max){
                    LOGGER('INFO',"Process should be restarted: Memory is $statinfo[5] Mb - Limit is $mem_max Mb.");
                    system ("$init_script restart");}
                
            }
            
            # Check if socket-handle is in select - or do this in plugin?
            if (my @sock_ready = $socksel->can_read(0.1)) {
                # process sockets ready to read
                for my $fhi (@sock_ready) {
	            $fh=$fhi; # important! different lexical scoping of $fh and $fhi!
                    if ($plugin_socket_subscribe{$fh}) {
                        LOGGER('DEBUG',"Call plugin $plugin_socket_subscribe{$fh} subscribed for $fh");
                        $thr_eiblisten_timeout = time(); # set timeout
                        %msg=();
                        check_generic_plugins($plugin_socket_subscribe{$fh});
                    } else {
                        #else if - no plugin subscribed, remove select
                        $socksel->remove($fh);
                        LOGGER('DEBUG',"Removed orphaned socksel $fh");
                    }
                }
            }
            # case cyclic time
            if ($daemon_config{''}{'sendtime_cycle'} and $daemon_config{''}{'sendtime_ga_time'} and $daemon_config{''}{'sendtime_ga_date'} and (time() - $sendtime_last > $daemon_config{''}{'sendtime_cycle'}) ) {
                # send time cyclic
                eibsend_date_time($daemon_config{''}{'sendtime_ga_date'},$daemon_config{''}{'sendtime_ga_time'});
                $sendtime_last = time();
            }
        }
        
        if ($eib == 1)
        {
            LOGGER('INFO',"eibd connection lost - retrying") unless $retryinfo;
            $retryinfo = 1;
            $eibpoll = 1;
            sleep 5;
        }
    }
    if($eib_logging) { close FILE_EIBLOG , ">>$eiblogfile"; }
}


sub getISODateStamp { # Timestamps for Logs
    my ($eseconds,$msec) = gettimeofday();
    my ($second, $minute, $hour, $dayOfMonth, $month, $yearOffset, $dayOfWeek, $dayOfYear, $daylightSavings) = localtime($eseconds);
    return sprintf "%04d-%02d-%02d %02d:%02d:%02d.%03d", ($yearOffset+1900),($month+1),$dayOfMonth,$hour,$minute,$second,$msec/1000;
}


sub check_generic_plugins {#check Plugins
    my $runname = shift;
    my $plugin_max_errors = 5;
    $thr_eiblisten_cause = "checking plugins";

    my @plugins = <$plugin_path*>;
    for my $plugfile (@plugins) 
    {
      my $plugname = basename($plugfile);
      next if defined $runname && $plugname ne $runname;

      $plugin_info{$plugname.'_lastsaved'} = ((stat($plugfile))[9]); 
      $plugin_info{$plugname.'_cycle'} = $plugin_cycle unless defined $plugin_info{$plugname.'_cycle'};

        # Parse, compile and cache plugins upon initialization or modification
        if( $plugin_info{$plugname.'_lastsaved'} > $plugin_info{$plugname.'_last'} || !$plugin_initflag) 
        {
            # reset error counters
            $plugin_info{$plugname.'_timeout_err'}=0;
            $plugin_info{$plugname.'_meminc'}=0;
            $plugin_info{$plugname.'_memstart'}=0;
            $plugin_info{$plugname.'_ticks'}=0;

            # parse and cache the code
            open FILE, "<$plugfile";
            my $source=join '', <FILE>;
            close FILE;

            if($source=~/COMPILE_PLUGIN/)
            {
                LOGGER('DEBUG',"Compiling PLUGIN $plugname");

                my $stime=time();
                eval "\$plugin_code{\$plugname}=sub {\n $source \n};\n";
                
                if ($@ || !$plugin_code{$plugname}) 
                {
                    LOGGER('DEBUG',"Compile-time ERROR in $plugname $@");
                    $plugin_info{$plugname.'_result'} = 'Compile-time error: '.$@;
                    delete $plugin_code{$plugname};
                }
                else
                {
                    plugin_log($plugname, sprintf("compiled in %.1fs",time()-$stime)); 	
                }
            }
        }
        
        # Execute plugins that are due, or on a direct call from a subscribed GA/socket (via plugin_subscribe)
        if( $plugin_info{$plugname.'_lastsaved'} > $plugin_info{$plugname.'_last'}
            || time() > $plugin_info{$plugname.'_last'} + $plugin_info{$plugname.'_cycle'} && $plugin_info{$plugname.'_cycle'} 
            || $runname eq $plugname || !$plugin_initflag)
        {
            LOGGER('DEBUG',"Running PLUGIN $plugname");
#           plugin_log($plugname, "RUNNING"); 

            if ($plugin_info{$plugname.'_timeout_err'}<$plugin_max_errors) {
                $thr_eiblisten_cause = "TIMEOUT running PLUGIN ".$plugname;
                $plugindb->sync(); # write out AFTER setting reason BEFORE err++
                $plugin_info{$plugname.'_timeout_err'}++;
                my @statinfo = get_stat_info($$);
                my $before_mem = $statinfo[5];
                my $before_ticks = $statinfo[6];
                my $starttime = time();
                
                if($plugin_code{$plugname})
                {
                    eval { $plugin_info{$plugname.'_result'} = $plugin_code{$plugname}->(); };
                
                    if ($@) 
                    {
                	LOGGER('DEBUG',"ERROR in $plugname $@");
                	$plugin_info{$plugname.'_result'} = 'Run-time error: '.$@;
                    }
                }		    
                else
                {
                    open FILE, "<$plugfile";
                    my $source=join '', <FILE>;
                    close FILE;
                
                    $plugin_info{$plugname.'_result'} = eval $source;
                    
                    if ($@) 
                    {
                	LOGGER('DEBUG',"ERROR in $plugname $@");
                	$plugin_info{$plugname.'_result'} = $@;
                    }
                }

                $plugin_info{$plugname.'_last'} = time() unless ($runname eq $plugname); #only reset cycle on non-direct call
                @statinfo = get_stat_info($$);
                if (!$plugin_initflag) {
                    $plugin_info{$plugname.'_memstart'}= $statinfo[5] - $before_mem;
                } else {
                    $plugin_info{$plugname.'_meminc'} += $statinfo[5] - $before_mem;
                }
                $plugin_info{$plugname.'_ticks'} += $statinfo[6] - $before_ticks;
                $plugin_info{$plugname.'_timeout_err'}=0;
                $thr_eiblisten_cause = "normally";
                $plugin_info{$plugname.'_runtime'} = nearest(.3,time() - $starttime);
                if ($plugin_info{$plugname.'_result'} or $@) {
                    plugin_log($plugname,"$plugin_info{$plugname.'_result'},$plugin_info{$plugname.'_runtime'}s,$@");
                }
                $plugindb->sync(); # write out AFTER setting reason
            }
            if ($@) {
                LOGGER('DEBUG',"ERROR in $plugname $@");
                $plugin_info{$plugname.'_result'} = $@;
            }
            
        }
    }
    # check for orphaned/old values in tied hash here!
    $plugin_initflag = 1;
    
    #possible this prevents a memleak
    @plugins = ();
    undef @plugins;
    #maybe !?
}


sub check_global_rrd_update { # here eib_traffic and daemon-stats - also: CHECK for MEMLEAK !!!
    
    if ($rrd_lastupdate + $rrd_interval < time()) {
        # check/create rrd
        if (!-d $rrd_dir) { mkdir($rrd_dir); }
        
        if (!-e $rrd_dir.'eib_traffic.rrd' && $eib_logging == 1) {
            # Create RRD
            my $heartbeat = $rrd_interval * 3;
            RRDs::create($rrd_dir.'eib_traffic.rrd',
            '--step' => $rrd_interval,
            'DS:eib_tps:COUNTER:'.$heartbeat.':0:50','DS:eib_bps:COUNTER:'.$heartbeat.':0:9830400',
            'RRA:AVERAGE:0.5:1:'.$rra1_row,'RRA:AVERAGE:0.5:5:'.$rra5_row,'RRA:AVERAGE:0.5:15:'.$rra15_row,'RRA:AVERAGE:0.5:180:'.$rra180_row,
            'RRA:MIN:0.5:1:'.$rra1_row,'RRA:MIN:0.5:5:'.$rra5_row,'RRA:MIN:0.5:15:'.$rra15_row,'RRA:MIN:0.5:180:'.$rra180_row,
            'RRA:MAX:0.5:1:'.$rra1_row,'RRA:MAX:0.5:5:'.$rra5_row,'RRA:MAX:0.5:15:'.$rra15_row,'RRA:MAX:0.5:180:'.$rra180_row);
            if (RRDs::error) {
                LOGGER('INFO',"Create RRDs failed:".RRDs::error);
            } else {
                LOGGER('INFO',"Created RRD eib-traffic");
            }
        }
        
        #Now do the updates
        if ($eib_logging == 1){
            RRDs::update($rrd_dir.'eib_traffic.rrd','N:'.$telegram_count.':'.$telegram_bytes);
        }
        if (RRDs::error) {
            LOGGER('INFO',"Update of RRDs eib-traffic failed:".RRDs::error." -> deleted");
            unlink($rrd_dir.'eib_traffic.rrd');
        } else {
            LOGGER('DEBUG',"Updated RRD eib-traffic");
        }
        my @statinfo = get_stat_info($$);
        update_rrd($name."_mem","",$statinfo[5]);
        update_rrd($name."_stime","",$statinfo[0],"COUNTER");
        update_rrd($name."_utime","",$statinfo[1],"COUNTER");
        update_rrd($name."_cutime","",$statinfo[2],"COUNTER");
        update_rrd($name."_cstime","",$statinfo[3],"COUNTER");
        $rrd_lastupdate = time();
    }
}

sub trim($){# Perl trim function to remove whitespace from the start and end of the string
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}

sub ltrim($){# Left trim function to remove leading whitespace
    my $string = shift;
    $string =~ s/^\s+//;
    return $string;
}

sub rtrim($){# Right trim function to remove trailing whitespace
    my $string = shift;
    $string =~ s/\s+$//;
    return $string;
}


sub rstr{# right $len chars
    my $s = shift;
    my $len = shift;
    # printf("DEBUG %s %i\n",$s,$len);
    return substr($s, length($s) - $len, $len);
}

sub get_stat_info { #reads info from deamon file
    my $error = '';
    my @ret;
    
    ### open and read the main stat file
    if( ! open(_INFO,"</proc/$_[0]/stat") ){
        $error = "Couldn't open /proc/$_[0]/stat [$!]";
        return @ret;
    }
    my @info = split(/\s+/,<_INFO>);
    close(_INFO);
    ### get the important ones
    # utime,stime,cutime,cstime,vsize,rss (in MB), cummulated *time for convinience
    @ret = ($info[13],$info[14],$info[15],$info[16],$info[22]/1024,nearest(.1,$info[23] * 4/1024),$info[13]+$info[14]+$info[15]+$info[16]);
    return @ret;
    
    ### these are all the props (skip some)
    # pid(0) comm(1) state ppid pgrp session tty
    # tpgid(7) flags minflt cminflt majflt cmajflt
    # utime(13) stime cutime cstime counter
    # priority(18) timeout itrealvalue starttime vsize rss
    # rlim(24) startcode endcode startstack kstkesp kstkeip
    # signal(30) blocked sigignore sigcatch wchan
    ###
}
sub my_read_cfg{ # read config files
    my $cfgfile=shift;
    my $cfghash=shift;
    %{$cfghash}=();
    
    open CONF, "<$cfgfile" || die "Could not open $cfgfile for reading";
    $/="\n";
    
    my $record={};
    my $key='';
    
    while(<CONF>)
    {
        unless ($_ =~ /\#/){ #checks for uncommented
        # reads eibga only
        if(/^\s*\[\s*(.+?)\s*\]\s*$/)
        {
            my $newkey=$1;
            
            if(defined $key) # flush last record
            {
                if(defined $record->{name} && $cfgfile=~/eibga/)
                {
                    $record->{ga}=$key;
                    $record->{name}=~/^\s*(\S+)/;
                    $cfghash->{$record->{name}}=$record;
                    #specific to my GA scheme (Fry)
                    $record->{short}=$1;
                    $record->{short}="ZV_$1" if $record->{name}=~/^Zeitversand.*(Uhrzeit|Datum)/; # short versions of "Zeitversand"
                    $cfghash->{$record->{short}}=$record;
                    # end of specific part
                }
                $cfghash->{$key}=$record;
            }
            # start new record
            $record={};
            $key=$newkey;
        }
        # reads pure config file
        elsif(/^\s*(.+?)\s*\=\s*(.*?)\s*$/)
        {
            #print $1 ." und ". $2;
            $record->{$1} = $2;
            $cfghash->{$key}=$record;
        }
        else {
            # empty line
        }
    }
}
close CONF;

if($key) # flush last record
{
    if(defined $record->{name} && $cfgfile=~/eibga/)
    {
        $record->{ga}=$key;
        $record->{name}=~/^\s*(\S+)/;
        $cfghash->{$record->{name}}=$record;
        # specific to my GA scheme (Fry)
        $record->{short}=$1;
        $record->{short}="ZV_$1" if $record->{name}=~/^Zeitversand.*(Uhrzeit|Datum)/; # short versions of "Zeitversand"
        $cfghash->{$record->{short}}=$record;
        # end of specific part
    }
    $cfghash->{$key}=$record;
}
}

sub my_write_cfg{ # write config files
    my $cfgfile=shift;
    my $cfghash=shift;
    
    open CONF, ">$cfgfile" || die "Could not open $cfgfile for writing";
    
    for my $s (keys %{$cfghash})
    {
        next if $cfgfile=~/eibga/ && $s!~/^[\'\/0-9]+$/;
        
        print CONF "[$s]\n";
        
        for my $k (keys %{$cfghash->{$s}})
        {
            print CONF "$k=$cfghash->{$s}{$k}\n";
        }
    }
    
    close CONF;
}




sub LOGGER($$){# LOG-sub to replace heavyweight Log4Perl/Syslog
    my $level = shift;
    my $msg = shift;
    
    if ($level eq "DEBUG") {
        return unless $opts{d};
    }
    if ($opts{d}) {
        print STDERR getISODateStamp . " $level - $msg\n";
    } else {
        `logger -t $0 -p $level "$level - $msg"`;
    }
}


sub decode_dpt9 {# encode DPT9.001/EIS 5
    my @data = @_;
    my $res;
    
    unless ($#data == 2) {
        ($data[1],$data[2]) = split(' ',$data[0]);
        $data[1] = hex $data[1];
        $data[2] = hex $data[2];
        unless (defined $data[2]) {
            return;
        }
    }
    my $sign = $data[1] & 0x80;
    my $exp = ($data[1] & 0x78) >> 3;
    my $mant = (($data[1] & 0x7) << 8) | $data[2];
    
    $mant = -(~($mant - 1) & 0x7ff) if $sign != 0;
    $res = (1 << $exp) * 0.01 * $mant;
    return $res;
}

sub encode_dpt9 { # 2byte signed float
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

sub decode_dpt2 { #2bit "signed bit"
    my $val=hex(shift) & 0x03;
    return $val > 1 ? $val-4 : $val;
}

sub decode_dpt3 { #4bit signed integer
    my $val=hex(shift) & 0x0f;
    return $val > 7 ? 8-$val : $val;
}

sub decode_dpt4 { #1byte char
    return sprintf("%c", hex(shift));
}

sub decode_dpt5 { #1byte unsigned percent
    return sprintf("%.1f", hex(shift) * 100 / 255);
}

sub decode_dpt510 { #1byte unsigned UChar
    return hex(shift);
}

sub decode_dpt6 { #1byte signed
    my $val = hex(shift);
    return $val > 127 ? $val-256 : $val;
}

sub decode_dpt7 { #2byte unsigned
    my @val = split(" ",shift);
    return (hex($val[0])<<8) + hex($val[1]);
}

sub decode_dpt8 { #2byte signed
    my @val = split(" ",shift);
    my $val2 = (hex($val[0])<<8) + hex($val[1]);
    return $val2 > 32767 ? $val2-65536 : $val2;
}

sub decode_dpt10 { #3byte time
    my @val = split(" ",shift);
    my @wd = qw(Null Mo Di Mi Do Fr Sa So);
    $val[0] = hex($val[0]);
    $val[1] = hex($val[1]);
    $val[2] = hex($val[2]);
    unless ($val[2]) { return; }
    my $day = ($val[0] & 0xE0) >> 5;
    my $hour    = $val[0] & 0x1F;
    my $minute  = $val[1];
    my $second  = $val[2];
    return sprintf("%s %02i:%02i:%02i",$wd[$day],$hour,$minute,$second);
}

sub decode_dpt11 { #3byte date
    my @val = split(" ",shift);
    my @wd = qw(Null Mo Di Mi Do Fr Sa So);
    $val[0] = hex($val[0]);
    $val[1] = hex($val[1]);
    $val[2] = hex($val[2]);
    unless ($val[2]) { return; }
    my $mday    = $val[0] & 0x1F;
    my $mon     = $val[1] & 0x0F;
    my $year    = $val[2] & 0x7F;
    $year = $year < 90 ? $year+2000 : $year+1900; # 1990 - 2089
    return sprintf("%04i-%02i-%02i",$year,$mon,$mday);
}

sub decode_dpt12 { #4byte unsigned
    my @val = split(" ",shift);
    return (hex($val[0])<<24) + (hex($val[1])<<16) + (hex($val[2])<<8) + hex($val[3]);
}

sub decode_dpt13 { #4byte signed
    my @val = split(" ",shift);
    my $val2 = (hex($val[0])<<24) + (hex($val[1])<<16) + (hex($val[2])<<8) + hex($val[3]);
    return $val2 >  2147483647 ? $val2-4294967296 : $val2;
}

sub decode_dpt14 { #4byte float
    #Perls unpack for float is somehow strange broken
    my @val = split(" ",shift);
    my $val2 = (hex($val[0])<<24) + (hex($val[1])<<16) + (hex($val[2])<<8) + hex($val[3]);
    my $sign = ($val2 & 0x80000000) ? -1 : 1;
    my $expo = (($val2 & 0x7F800000) >> 23) - 127;
    my $mant = ($val2 & 0x007FFFFF | 0x00800000);
    my $num = $sign * (2 ** $expo) * ( $mant / (1 << 23));
    return sprintf("%.4f",$num);
}

sub decode_dpt16 { # 14byte char
    my @val = split(" ",shift);
    my $chars;
    for (my $i=0;$i<14;$i++) {
        $chars .= sprintf("%c", hex($val[$i]));
    }
    return sprintf("%s",$chars);
}

sub encode_dpt5 {
    my $value = shift;
    $value = 100 if ($value > 100);
    $value = 0 if ($value < 0);;
    my $byte = sprintf ("%.0f", $value * 255 / 100);
    return($byte);
}

sub decode_dpt {
    my $dst = shift;
    my $data = shift;
    my $dpt = shift;
    my $value;
    my $dptid = $dpt || $eibgaconf{$dst}{'DPTSubId'} || 0;
    $data =~ s/\s+$//g;
    given ($dptid) {
        when (/^10/)      { $value = decode_dpt10($data); }
        when (/^11/)      { $value = decode_dpt11($data); }
        when (/^12/)      { $value = decode_dpt12($data); }
        when (/^13/)      { $value = decode_dpt13($data); }
        when (/^14/)      { $value = decode_dpt14($data); }
        when (/^16/)      { $value = decode_dpt16($data); }
        when (/^17/)      { $value = decode_dpt510($data & 0x3F); }
        when (/^20/)      { $value = decode_dpt510($data); }
        when (/^\d\d/)    { return; } # other DPT XX 15 are unhandled
        when (/^1/)       { $value = int($data); }
        when (/^2/)       { $value = decode_dpt2($data); }
        when (/^3/)       { $value = decode_dpt3($data); }
        when (/^4/)       { $value = decode_dpt4($data); }
        when ([5,'5.001'])  { $value = decode_dpt5($data); }
        when (['5.004','5.005','5.010']) { $value = decode_dpt510($data); }
        when (/^6/) { $value = decode_dpt6($data); }
        when (/^7/) { $value = decode_dpt7($data); }
        when (/^8/) { $value = decode_dpt8($data); }
        when (/^9/) { $value = decode_dpt9($data); }
        default   { return; } # nothing
    }
    return $value;
}

sub knx_read {
    my $dst = $_[0];
    my $age = $_[1] || 0; # read hot unless defined
    my $dpt = $_[2];
    my $src=EIBConnection::EIBAddr();
    my $buf=EIBConnection::EIBBuffer();
    my $hexbytes;
    my $leibcon = EIBConnection->EIBSocketURL($eib_url) or return("Error: $!");
    my $res=$leibcon->EIB_Cache_Read_Sync(str2addr($dst), $src, $buf, $age);
    if (!defined $res) { return; } # ("ReadError: $!");
    $leibcon->EIBClose();
    # $$src contains source PA
    
    my @data = unpack ("C" . bytes::length($$buf), $$buf);
    if ($res == 2) { # 6bit only
        return decode_dpt($dst,($data[1] & 0x3F),$dpt);
    } else {
        for (my $i=2; $i<= $res-1; $i++) {
            $hexbytes = $hexbytes . sprintf("%02X ", ($data[$i]));
        }
        return decode_dpt($dst,$hexbytes,$dpt);
    }
}

sub knx_write {
    if ($eib == 1){
        my ($dst,$value,$dpt,$response,$dbgmsg) = @_;
        my $bytes;
        my $apci = 0x80; # 0x40=response, 0x80=write, 0x00=non-blocking read
        $apci = ($response==2 ? 0 : ($response==1 ? 0x40 : ($response==0 ? 0x80 : $response))) if defined $response;

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
        $dpt = $eibgaconf{$dst}{'DPTSubId'} unless $dpt; # read dpt from eibgaconf if existing
        given ($dpt) {
            when (/^10/) {
                my %wd=(Mo=>1, Di=>2, Mi=>3, Do=>4, Fr=>5, Sa=>6, So=>7, Mon=>1, Tue=>2, Wed=>3, Thu=>4, Fri=>5, Sat=>6, Sun=>7);
                my $wdpat=join('|',keys %wd);
                my ($w,$h,$m,$s);
                return unless ($w,$h,$m,$s)=($value=~/^($wdpat)?\s*([0-2][0-9])\:([0-5][0-9])\:?([0-5][0-9])?\s*/si);
                return unless defined $h && defined $m;
                $w=$wd{$w} if defined $wd{$w};
                $h+=($w<<5) if $w;
                $s=0 unless $s;
                $bytes=pack("CCCCC",0,$apci,$h,$m,$s);
            }
            when (/^11/) {
                my ($y,$m,$d);
                return unless ($y,$m,$d)=($value=~/^([1-2][0-9][0-9][0-9])\-([0-1][0-9])\-([0-3][0-9])\s*/si);
                return if $y<1990 || $y>=2090;
                $y%=100;
                $bytes=pack("CCCCC",0,$apci,$d,$m,$y);
            }
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
        plugin_log("knx_write","KNX write DPT $dpt: $value ($bytes) to $dst ($dbgmsg)") if ($knxdebug);
        my $leibcon = EIBConnection->EIBSocketURL($eib_url) or return("Error opening con: $!");
        if ($leibcon->EIBOpenT_Group(str2addr($dst),1) == -1) { return("Error opening group: $!"); }
        my $res=$leibcon->EIBSendAPDU($bytes);
        $leibcon->EIBClose();
        return $res;
    }else{
        return;
    }}

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

sub update_rrd {
    my ($key,$suffix,$value,$valtype,$stephours,$rrasteps) = @_;
    return unless defined $value;
    $valtype = "GAUGE" unless $valtype;
    $stephours = 24 unless $stephours;
    
    if (!-d $rrd_dir) { mkdir($rrd_dir); }
    if (!-e $rrd_dir . $key . $suffix .'.rrd') {
        # Checkvalue-type for boundries?
        # Create RRD
        if ($valtype eq "GAUGE"){
            my $heartbeat = $rrd_interval * 3;
            $daemon_config{$key}{'rra_1_interval_rows'} = $rra1_row unless $daemon_config{$key}{'rra_1_interval_rows'};
            $daemon_config{$key}{'rra_5_interval_rows'} = $rra5_row unless $daemon_config{$key}{'rra_5_interval_rows'};
            $daemon_config{$key}{'rra_15_interval_rows'} = $rra15_row unless $daemon_config{$key}{'rra_15_interval_rows'};
            $daemon_config{$key}{'rra_180_interval_rows'} = $rra180_row unless $daemon_config{$key}{'rra_180_interval_rows'};
            RRDs::create($rrd_dir.$key.$suffix .'.rrd',
            '--step' => $rrd_interval,
            'DS:value:'.$valtype.':'.$heartbeat.':-55:255000',
            'RRA:AVERAGE:0.5:1:'.$daemon_config{$key}{'rra_1_interval_rows'},'RRA:AVERAGE:0.5:5:'.$daemon_config{$key}{'rra_5_interval_rows'},'RRA:AVERAGE:0.5:15:'.$daemon_config{$key}{'rra_15_interval_rows'},'RRA:AVERAGE:0.5:180:'.$daemon_config{$key}{'rra_180_interval_rows'},
            'RRA:MIN:0.5:1:'.$daemon_config{$key}{'rra_1_interval_rows'},'RRA:MIN:0.5:5:'.$daemon_config{$key}{'rra_5_interval_rows'},'RRA:MIN:0.5:15:'.$daemon_config{$key}{'rra_15_interval_rows'},'RRA:MIN:0.5:180:'.$daemon_config{$key}{'rra_180_interval_rows'},
            'RRA:MAX:0.5:1:'.$daemon_config{$key}{'rra_1_interval_rows'},'RRA:MAX:0.5:5:'.$daemon_config{$key}{'rra_5_interval_rows'},'RRA:MAX:0.5:15:'.$daemon_config{$key}{'rra_15_interval_rows'},'RRA:MAX:0.5:180:'.$daemon_config{$key}{'rra_180_interval_rows'});
            if (RRDs::error) {
                LOGGER('INFO',"Create RRDs failed for $key$suffix :".RRDs::error);
            } else {
                LOGGER('INFO',"Created RRD for $key$suffix");
            }
        }
        elsif ($valtype eq "COUNTER"){
            $rrasteps = 7 unless $rrasteps;
            RRDs::create ($rrd_dir.$key.$suffix .'.rrd',
            'DS:value:'.$valtype.':'.(($stephours*3600)+600).':0:10000000000',
            'RRA:AVERAGE:0.5:1:1826', 'RRA:AVERAGE:0.5:'.$rrasteps.':1300',
            'RRA:MIN:0.5:1:1826',     'RRA:MIN:0.5:'.$rrasteps.':1300',
            'RRA:MAX:0.5:1:1826',     'RRA:MAX:0.5:'.$rrasteps.':1300',
            '-s '.($stephours*3600));
            if (RRDs::error) {
                LOGGER('INFO',"Create RRDs failed for $key$suffix :".RRDs::error);
            } else {
                LOGGER('INFO',"Created RRD for $key$suffix");
            }
        }
    }
    
    # Update values
    $value = int($stephours*3600*$value) if ($valtype eq "COUNTER");
    RRDs::update($rrd_dir.$key.$suffix.'.rrd','N:'.$value);
    
    if (RRDs::error) {
        LOGGER('INFO',"Update of RRDs failed for $key$suffix/$value:".RRDs::error);
        # FIXME: Check if error comes from update-value or from rrd-file!
        #rename ($rrd_dir.$key.$suffix.'.rrd',$rrd_dir.$key.$suffix.'.rrd.old');
    } else {
        LOGGER('DEBUG',"Updated RRD for $key$suffix/$value");
    }
}


sub decode_vbusmonitor{
    my $buf = shift;
    my %msg;
    my @data;
    my ($type, $src, $dst, $drl, $bytes) = unpack("CnnCa*", $buf);
    @data = unpack ("C" . bytes::length($bytes), $bytes);
    
    my $apci;
    
    # For mapping between APCI symbols and values, not fully implemented!
    my @apcicodes = qw (A_GroupValue_Read A_GroupValue_Response A_GroupValue_Write
    A_PhysicalAddress_Write A_PhysicalAddress_Read A_PhysicalAddress_Response
    A_ADC_Read A_ADC_Response A_Memory_Read A_Memory_Response A_Memory_Write
    A_UserMemory A_DeviceDescriptor_Read A_DeviceDescriptor_Response A_Restart
    A_OTHER); # not fully implemented
    my @tpducodes = qw(T_DATA_XXX_REQ T_DATA_CONNECTED_REQ T_DISCONNECT_REQ T_ACK);
    
    $msg{'repeated'} = (($type & 0x20) >> 5) ^ 0b01;
    # priority class b00, b01, b10, b11
    my @prioclasses = qw(system alarm high low);
    $msg{'class'} = @prioclasses[($type & 0xC) >> 2];
    
    #DRL
    $msg{'rcount'} = ($drl & 0x70)>>4;
    $msg{'datalen'} = ($drl & 0x0F);
    # FIXME: this is crap!
    #$msg{'tpdu_type'} = $tpducodes[($data[0] & 0xC0)>>6];
    if (@data >= 1) {
        if (($data[0] & 0xFC) == 0) { 
            $msg{'tpdu_type'} = "T_DATA_XXX_REQ"; 
            $msg{'apci'} = @apcicodes[(($data[0] & 0x03)<<2) | (($data[1] & 0xC0)>>6)]; }
        if ($data[0] == 0x80) { $msg{'tpdu_type'} = "T_CONNECT_REQ"; }
        if ($data[0] == 0x81) { $msg{'tpdu_type'} = "T_DISCONNECT_REQ"; }
        if (($data[0] & 0xC3) == 0xC2) { $msg{'tpdu_type'} = "T_ACK"; }
        if (($data[0] & 0xC3) == 0xC3) { $msg{'tpdu_type'} = "T_NACK"; }
        if (($data[0] & 0xC0) == 0x40) { 
            $msg{'tpdu_type'} = "T_DATA_CONNECTED_REQ"; 
            $msg{'apci'} = @apcicodes[(($data[0] & 0x03)<<2) | (($data[1] & 0xC0)>>6)]; }
    }
    
    $msg{'sequence'} = ($data[0] & 0x3C)>>2;
    if ($msg{'tpdu_type'} eq "T_DATA_XXX_REQ" and $msg{'datalen'} == 1) { # 6bit only
        $msg{'data'} = sprintf("%02X", ($data[1] & 0x3F));
    } elsif ($msg{'tpdu_type'} eq "T_DATA_XXX_REQ" and $msg{'datalen'} > 1) {
        for (my $i=2; $i<= $msg{'datalen'}; $i++) {
            $msg{'data'} = $msg{'data'} . sprintf("%02X ", ($data[$i]));
        }
    } else {
        #LOGGER('DEBUG',"OTHER R:$msg{'repeated'} P:$msg{'class'} Hops:$msg{'rcount'} T:$msg{'tpdu_type'} Seq:$msg{'sequence'} A:$msg{'apci'} src: $msg{'src'} dst: $msg{'dst'} len: $msg{'datalen'} data: @data");
    } 
    $msg{'data'} = trim($msg{'data'});
    $msg{'src'} = addr2str($src);
    $msg{'dst'} = addr2str($dst, $drl>>7);
    $msg{'value'} = decode_dpt($msg{'dst'},$msg{'data'});
    LOGGER('DEBUG',"R:$msg{'repeated'} P:$msg{'class'} Hops:$msg{'rcount'} T:$msg{'tpdu_type'} Seq:$msg{'sequence'} A:$msg{'apci'} src: $msg{'src'} dst: $msg{'dst'} len: $msg{'datalen'} data: $msg{'data'} ($msg{'value'})");
    
    #$msg{'data'} = \@data;
    $msg{'buf'} = unpack ("H*", $buf);
    return %msg;
}


sub init_udpsock {
    if ($daemon_config{''}{'udp_ip'} and $daemon_config{''}{'udp_port'}) {
        $udp_sock = IO::Socket::INET->new(Proto=>"udp",PeerPort=>$daemon_config{''}{'udp_port'},PeerAddr=>$daemon_config{''}{'udp_ip'}) or LOGGER('WARN',"Can't create UDP socket to send: $@)");
    }
}



sub plugin_log ($$) {
    my $name = shift;
    my $msg = shift;
    LOGGER('DEBUG',"Plugin LOG $name -> $msg");
    open PLUGIN_LOG, ">>$plugin_logfile"or print "Can't write to file '$plugin_logfile' [$!]\n"; # writeout to log here
    print PLUGIN_LOG getISODateStamp .",$name,$msg\n";
    close PLUGIN_LOG;
}

=pod
 
 =head1 NAME
 
 WireGate Daemon
 
 =head1 SYNOPSIS
 
 daemon to handle gateway/communication-functions
 
 =head1 CREDITS
 
 Parts copyright by Martin Koegler from bcusdk/eibd (GPL)
 Parts copyright by various from misterhouse (GPL)
 
 =head1 HISTORY
 
 2008-12-09  Initial versions of wg-ow2eib.pl and wg-eiblis.pl
 see SVN
 
 =cut
