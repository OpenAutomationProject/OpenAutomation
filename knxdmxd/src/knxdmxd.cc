/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * knxdmxd.c
 * Copyright (C) Jan N. Klug 2012 <jan.n.klug@rub.de>
 * Daemon skeleton by Michael Markstaller 2011 <devel@wiregate.de>
 * 
 * knxdmxd is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * knxdmxd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <json/json.h>

#include <ola/Logging.h>
#include <ola/StreamingClient.h>
#include <ola/StringUtils.h>

#include <eibclient.h>

#include <iostream>
#include <fstream>
#include <map>

#include <knxdmxd.h>
#include <log.h>

#include <fixture.h>
#include <cue.h>
#include <dmxsender.h>
#include <trigger.h>

#define DEBUG 1
#define DAEMON_NAME "knxdmxd"
#define USAGESTRING "\n"\
  "\t-d               Run as daemon/No debug output\n"\
  "\t-p <pidfile>     PID-filename\n"\
  "\t-u <eib url>     URL to contact eibd like local:/tmp/eib or ip:192.168.0.101\n"\
  "\t-c <config-file> Config-File\n"
#define RETRY_TIME 5
#define BUFLEN 1024
#define POLLING_INTERVAL 10


template<class T>
std::string t_to_string(T i) {
    std::stringstream ss;
    ss << i;
    return ss.str();
}

void daemonShutdown();

eibaddr_t readgaddr (const std::string addr) {
	int a, b, c;
	char *s = (char *)addr.c_str();
   
	if (sscanf (s, "%d/%d/%d", &a, &b, &c) == 3)
		return ((a & 0x01f) << 11) | ((b & 0x07) << 8) | ((c & 0xff));
	if (sscanf (s, "%d/%d", &a, &b) == 2)
	    return ((a & 0x01f) << 11) | ((b & 0x7FF));
	if (sscanf (s, "%x", &a) == 1)
		return a & 0xffff;
	std::clog << kLogWarning << "invalid group address format " << addr << std::endl;
	daemonShutdown();
	return 0;
}


std::string eibd_url = "local:/tmp/eib";
std::string conf_file = "knxdmxd.conf";
int pidFilehandle;
std::string pidfilename = "/var/run/knxdmxd.pid";
knxdmxd::TriggerList triggerList;
knxdmxd::DMXSender sender;


void daemonShutdown() {
	//FIXME: clean exit pthread_exit(NULL); pthread_cancel(..);
	std::clog << kLogInfo << DAEMON_NAME <<  " daemon exiting" << std::endl;
    fprintf(stderr, "%s daemon exiting", DAEMON_NAME);	
    sender.Terminate();
	close(pidFilehandle);
	unlink((char *)pidfilename.c_str());
	exit(EXIT_SUCCESS);
}

void signal_handler(int sig) {
    switch(sig) {
        case SIGHUP:
            std::clog << kLogWarning << "Received SIGHUP signal." << std::endl;
            break;
        case SIGTERM:
            std::clog << kLogWarning << "Received SIGTERM signal." << std::endl;
			daemonShutdown();
			break;
        case SIGINT:
            std::clog << kLogWarning << "Received SIGINT signal." << std::endl;
			daemonShutdown();
			break;
        default:
            std::clog << kLogWarning << "Unhandled signal (" << sig << ") " << strsignal(sig) << std::endl;
            break;
    }
}

void olahandler() { // thread for OLA connection
  std::clog << kLogDebug << "OLA thread started" << std::endl;

  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);

  while (1) { // retry forever
    if (!sender.Init()) {
      std::clog << kLogWarning << "OLA: Client setup failed" << std::endl;    
      sleep(RETRY_TIME);
      continue;
    }
    
    sender.Start(); // Start the sender

    while (1) { // loop forever, should be used for monitoring the client
      if (!sender.Running())
        break;
      usleep(20000);
    }    
  }

}

knxdmxd::Trigger *json_get_trigger(struct json_object *trigger, const int type) {

  if (!trigger) {
    return NULL;
  }

  struct json_object *trigger_knx = json_object_object_get(trigger, "knx");
  struct json_object *trigger_value = json_object_object_get(trigger, "value");

  if (!trigger_knx) {
    return NULL;
  }

  int knx = readgaddr(json_object_get_string(trigger_knx));
  int val = (trigger_value) ? json_object_get_int(trigger_value) : -1;

  return new knxdmxd::Trigger(type, knx, val);
}

knxdmxd::Cue *json_get_cue(struct json_object *cue, const int cuenum, const int type) {

  // check link first
  struct json_object *link = json_object_object_get(cue, "link"); 
  if (link) { // is link
    knxdmxd::Cue *new_cue = new knxdmxd::Cue(json_object_get_string(link), true);
    return new_cue;
  }

  struct json_object *name = json_object_object_get(cue, "name");
  std::string c_name = (name) ? json_object_get_string(name) : "_c_"+t_to_string(cuenum);
  knxdmxd::Cue *new_cue = new knxdmxd::Cue(c_name);
     
  // get channels
  struct json_object *channels = json_object_object_get(cue, "channels");
  if (!channels) {
    std::clog << kLogInfo << "Skipping cue '" << c_name << "' (no channels defined)" << std::endl;
    delete new_cue;
    return new_cue;
  }
    
  for (int j=0; j<json_object_array_length(channels); j++) { // read all
    // get channel
    struct json_object *channel = json_object_array_get_idx(channels, j);

    struct json_object *fixt = json_object_object_get(channel, "fixture"); 
    struct json_object *chan = json_object_object_get(channel, "channel"); 
    struct json_object *value = json_object_object_get(channel, "value"); 

    if ((!fixt) || (!chan) || (!value)) {
      std::clog << kLogInfo << "Skipping errorneous channel def " << j << " in cue '" << c_name << "'" << std::endl;
      continue;
    }    
  
    knxdmxd::cue_channel_t channeldata;
    channeldata.fixture = sender.GetFixture(json_object_get_string(fixt));
    channeldata.name = json_object_get_string(chan);
    channeldata.value = json_object_get_int(value);
    
    // add
    new_cue->AddChannel(channeldata);
  }

  // fading
  
  struct json_object *fading = json_object_object_get(cue, "fading");
  if (fading) {
    struct json_object *fading_time = json_object_object_get(fading, "time");  
    if (!fading_time) {
      struct json_object *fading_time_in = json_object_object_get(fading, "in"); 
      struct json_object *fading_time_out = json_object_object_get(fading, "out"); 
   
      if ((!fading_time_in) || (!fading_time_out)) {
        std::clog << kLogInfo << "Skipping errorneous fading def in cue '" << c_name << "'" << std::endl;
      } else {
         float in = json_object_get_double(fading_time_in);
         new_cue->SetFading(in, json_object_get_double(fading_time_out));
      }
    } else {
      new_cue->SetFading(json_object_get_double(fading_time));
    }
  }  

  // waittime
    
  struct json_object *waittime = json_object_object_get(cue, "waittime");
  if (waittime) {
    new_cue->SetWaittime(json_object_get_double(waittime));
  }

  struct json_object *delay = json_object_object_get(cue, "delay");
  if (delay) {
    new_cue->SetDelay(json_object_get_double(delay));
  }


  if (type==knxdmxd::kScene) {
    struct json_object *triggers = json_object_object_get(cue, "trigger");
    if (!triggers) {
      std::clog << kLogInfo << "Skipping cue '" << c_name << "' (trigger required in scene definition)" << std::endl;
      delete new_cue;
      return new_cue;
    }
    
    struct json_object *go = json_object_object_get(triggers, "go");
    knxdmxd::Trigger *go_trigger = json_get_trigger(go, knxdmxd::kTriggerGo);
    if (go_trigger) {
      triggerList.Add(*go_trigger, new_cue);
    }
  } 
  
  return new_cue;
}

void load_config() {

  struct json_object *config;

  config = json_object_from_file((char *)conf_file.c_str());
 
  /*
   * fixtures
  */
  
  struct json_object *fixtures = json_object_object_get(config, "fixtures");
  int fixturenum = json_object_array_length(fixtures);
  std::clog << "Trying to import " << fixturenum << " fixture(s)" << std::endl;

  for (int i=0; i<fixturenum; i++) { // read all
    // get fixture
    struct json_object *fixture = json_object_array_get_idx(fixtures, i);

    // get name & create
    struct json_object *name = json_object_object_get(fixture, "name");
    std::string fname = (name) ? json_object_get_string(name) : "_f_"+t_to_string(i);
    knxdmxd::Fixture* f = new knxdmxd::Fixture(fname);
 
    // get channels & patch them
    struct json_object *channels = json_object_object_get(fixture, "channels");
    if (!channels) {
      std::clog << kLogInfo << "Skipping fixture '" << fname << "' (no channels defined)" << std::endl;
      continue;
    }
    
    for (int j=0; j<json_object_array_length(channels); j++) { // read all
      // get channel
      struct json_object *channel = json_object_array_get_idx(channels, j);
      
      // channel name, default is _c_<num>
      struct json_object *name = json_object_object_get(channel, "name"); 
      std::string cname = (name) ? json_object_get_string(name) : "_c_"+t_to_string(j);

      // dmx is required
      struct json_object *dmx = json_object_object_get(channel, "dmx");
      if (!dmx) {
        std::clog << kLogInfo << "Skipping channel '" << cname << "' in fixture '" << fname << "' (missing dmx)" << std::endl;
        continue;
      }
      std::string cdmx(json_object_get_string(dmx));
      
      // knx is optional
      struct json_object *knx = json_object_object_get(channel, "knx"); // knx is optional
      std::string cknx = (knx) ? json_object_get_string(knx) : "";
      
      // patch
      f->AddChannel(cname, cdmx, cknx);
    }
    
    // get fading
    struct json_object *fading = json_object_object_get(fixture, "fading");
    float ftime = (fading) ? json_object_get_double(json_object_object_get(fading, "time")) : 0;
    f->SetFadeTime(ftime);

    sender.AddFixture(f);
  }

  /*
   * scenes
  */
   
  struct json_object *scenes = json_object_object_get(config, "scenes");
  int scenenum = json_object_array_length(scenes);
  std::clog << "Trying to import " << scenenum << " scene(s)" << std::endl;
  
  for (int i=0; i<scenenum; i++) { // read all
    struct json_object *scene = json_object_array_get_idx(scenes, i);

    // get name & create
    struct json_object *name = json_object_object_get(scene, "name");
    std::string sname = (name) ? json_object_get_string(name) : "_s_"+t_to_string(i);
    
    knxdmxd::Cue *s = json_get_cue(scene, i, knxdmxd::kScene);
  }
  
  /* 
   * cuelists
  */
  
  struct json_object *cuelists = json_object_object_get(config, "cuelists");
  int cuelistnum = json_object_array_length(cuelists);
  std::clog << "Trying to import " << cuelistnum << " cuelist(s)" << std::endl;
  
  for (int i=0; i<cuelistnum; i++) { // read all
    struct json_object *cuelist = json_object_array_get_idx(cuelists, i);

    // get name & create
    struct json_object *name = json_object_object_get(cuelist, "name");
    std::string cname = (name) ? json_object_get_string(name) : "_c_"+t_to_string(i);
    knxdmxd::Cuelist *c = new knxdmxd::Cuelist(cname);

    // get cues
    struct json_object *cues = json_object_object_get(cuelist, "cues");
    for (int i=0; i<json_object_array_length(cues); i++) { // read all
      struct json_object *cue = json_object_array_get_idx(cues, i);
      c->AddCue(*json_get_cue(cue, i, knxdmxd::kCue));
    }
    
    // trigger is required

    struct json_object *triggers = json_object_object_get(cuelist, "trigger");
    if (!triggers) {
      std::clog << kLogInfo << "Skipping cuelist '" << name << "' (trigger required in cuelist definition)" << std::endl;
      continue;
    }

    knxdmxd::Trigger *trigger;

    trigger = json_get_trigger(json_object_object_get(triggers, "go"), knxdmxd::kTriggerGo);
    if (trigger) {
      triggerList.Add(*trigger, c);
    }

    trigger = json_get_trigger(json_object_object_get(triggers, "halt"), knxdmxd::kTriggerHalt);
    if (trigger) {
      triggerList.Add(*trigger, c);
    }

    trigger = json_get_trigger(json_object_object_get(triggers, "direct"), knxdmxd::kTriggerDirect);
    if (trigger) {
      triggerList.Add(*trigger, c);
    }
  }
 
  return;
}

class KNXHandler : public ola::thread::Thread {
  public:
    KNXHandler()
        : Thread(),
          m_mutex() {
    }
    ~KNXHandler() {}


    void knxhandler() {
  	  std::clog << "KNX thread started" << std::endl;
	  int len;
	  EIBConnection *con;
	  eibaddr_t dest;
	  eibaddr_t src;
	  unsigned char buf[255], val;
	
	  while (1) { //retry infinite
		con = EIBSocketURL ((char *)eibd_url.c_str());
		if (!con) {
          std::clog << kLogWarning << "eibd: Open failed" << std::endl;
		  sleep(RETRY_TIME);
		  continue;
		}

		if (EIBOpen_GroupSocket (con, 0) == -1) {
		  std::clog << kLogWarning << "eibd: Connect failed" << std::endl;
		  sleep(RETRY_TIME);
		  continue;
		}

		while (1) {
		  len = EIBGetGroup_Src (con, sizeof (buf), buf, &src, &dest);
		  if (len == -1) {
    	    std::clog << kLogWarning << "eibd: Read failed" << std::endl;
			sleep(RETRY_TIME);
			break;
		  }
			
	      if (len < 2) {
    	    std::clog << kLogWarning << "eibd: Invalid Packet" << std::endl;
			break;
		  }
			
		  if (buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0) {
      		std::clog << kLogWarning << "eibd: Unknown APDU from "<< src << " to " << dest << std::endl;
			break;
		  } else {
            switch (buf[1] & 0xC0) {
              case 0x00:
                //    sendKNXresponse (dest,zone+(controller*ZONES_PER_CONTROLLER),func);
	            break;
              case 0x40:
                //FIXME: response dunno
	            break;
              case 0x80:
	            if (buf[1] & 0xC0) {
                  val = (len==2) ? buf[1] & 0x3F : buf[2];
                  knxdmxd::Trigger trigger(knxdmxd::kTriggerAll, dest, val);
                  sender.Process(trigger);
                  triggerList.Process(trigger);
                }
            	break;
            }
		  }
		}
		
     	std::clog << kLogWarning << "eibd: Closed connection" << std::endl; //break in read-loop
		EIBClose (con);
	  }
  
    }

    void *Run() {
      ola::thread::MutexLocker locker(&m_mutex);
      knxhandler();
      return NULL;
    }

  private:
    ola::thread::Mutex m_mutex;
};

class OLAThread : public ola::thread::Thread {
  public:
    OLAThread()
        : Thread(),
          m_mutex() {
    }
    ~OLAThread() {}

    void *Run() {
      ola::thread::MutexLocker locker(&m_mutex);
      olahandler();
      return NULL;
    }

  private:
    ola::thread::Mutex m_mutex;
};


int main(int argc, char **argv) {

	int daemonize = 0;
    int c;
	char pidstr[255];
	
	while ((c = getopt (argc, argv, "dp:u:c:")) != -1)
		switch (c) {
			case 'd':
				daemonize = 1;
				break;
			case 'p':
				pidfilename.assign(optarg);
				break;
			case 'u':
				eibd_url.assign(optarg);
				break;
			case 'c':
				conf_file.assign(optarg);
				break;
			case '?':
				//FIXME: check arguments better, print_usage
				fprintf (stderr, "Unknown option `-%c'.\nUsage: %s %s", optopt, argv[0], USAGESTRING);
				return 1;
			default:
			 abort ();
		}

	//FIXME: clean shutdown in sub-thread with signals?
	signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);

    if (!daemonize) {
	    setlogmask(LOG_UPTO(LOG_DEBUG));
		std::clog.rdbuf(new Log(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER));
        std::clog << "startup with debug; pidfile: " << pidfilename << ", eibd: " << eibd_url << std::endl;
	} else {
	    setlogmask(LOG_UPTO(LOG_DEBUG));
		std::clog.rdbuf(new Log(DAEMON_NAME, LOG_CONS, LOG_USER));
	}

    std::clog << kLogInfo << "using config-file " << conf_file << std::endl;
    
	pid_t pid, sid;
 
    if (daemonize) {
        std::clog << kLogInfo << "starting daemon" << std::endl;
 
        pid = fork();
        if (pid < 0) {
            exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }
        umask(0);
        sid = setsid();
        if (sid < 0) {
            exit(EXIT_FAILURE);
        }
        if ((chdir("/")) < 0) {
            exit(EXIT_FAILURE);
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

	//FIXME: output errors to stderr, change order
	pidFilehandle = open((char *)pidfilename.c_str(), O_RDWR|O_CREAT, 0600);
	if (pidFilehandle == -1 )
	{

		std::clog << kLogInfo << "Could not open pidfile " << pidfilename << ", exiting" << std::endl;
	    fprintf(stderr, "Could not open pidfile %s, exiting", (char *)pidfilename.c_str());	
		exit(EXIT_FAILURE);
	}
	if (lockf(pidFilehandle,F_TLOCK,0) == -1)
	{
		std::clog << kLogInfo << "Could not lock pidfile " << pidfilename << ", exiting" << std::endl;
	    fprintf(stderr, "Could not lock pidfile %s, exiting", (char *)pidfilename.c_str());	
		exit(EXIT_FAILURE);
	}
	sprintf(pidstr,"%d\n",getpid());
	c = write(pidFilehandle, pidstr, strlen(pidstr));

    load_config();

    KNXHandler knx;
    OLAThread ola;

    knx.Start();
    ola.Start();

	while (1) {
//		std::clog << DAEMON_NAME << " daemon running" << std::endl;
		sleep(POLLING_INTERVAL*1000);
	}
	std::clog << kLogInfo << DAEMON_NAME << " daemon exiting" << std::endl;
     // TODO: Free any allocated resources before exiting - we never get here though -> signal handler
    exit(EXIT_SUCCESS);
}
