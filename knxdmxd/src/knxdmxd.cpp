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
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <json/json.h>

#include <ola/Logging.h>
#include <ola/DmxBuffer.h>
#include <ola/StreamingClient.h>
#include <ola/StringUtils.h>

//#include <assert.h>

#include <arpa/inet.h>
//#include <netinet/in.h>
//#include <sys/socket.h>

#include <pthread.h>
#include <eibclient.h>

#include <iostream>
#include <fstream>
#include <map>

/*#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>*/

#define DEBUG 1
#define DAEMON_NAME "knxdmxd"
#define USAGESTRING "\n"\
  "\t-d               Run as daemon/No debug output\n"\
  "\t-p <pidfile>     PID-filename\n"\
  "\t-u <eib url>     URL to contact eibd like local:/tmp/eib or ip:192.168.0.101\n"\
  "\t-c <config-file> Config-File\n"
#define NUM_THREADS 4
#define MAX_ZONES 31
#define RETRY_TIME 5
#define BUFLEN 1024
#define POLLING_INTERVAL 10
#define FADING_INTERVAL 50000

using namespace std;
//namespace bpt = boost::property_tree;
typedef vector<string> svector;

void daemonShutdown();

eibaddr_t readgaddr (const char *addr) {
	int a, b, c;
	if (sscanf (addr, "%d/%d/%d", &a, &b, &c) == 3)
		return ((a & 0x01f) << 11) | ((b & 0x07) << 8) | ((c & 0xff));
	if (sscanf (addr, "%d/%d", &a, &b) == 2)
	    return ((a & 0x01f) << 11) | ((b & 0x7FF));
	if (sscanf (addr, "%x", &a) == 1)
		return a & 0xffff;
	syslog(LOG_WARNING,"invalid group address format %s", addr);
	daemonShutdown();
	return 0;
}

int readdaddr (const string addr) {
  int universe, channel;
  sscanf( (char*)addr.c_str(), "%d.%d", &universe, &channel);
  if (channel==-1) { // default universe is 1
    channel = universe;
    universe = 1;
  }
  return (universe << 9) + channel;
}

namespace knxdmxd {

class Fixture {
  public:
    Fixture() {};
    Fixture(const string str);
    
    void Patch(map<int, svector >& patchMap, const string channel, const int DMX, const int KNX);
    void Patch(map<int, svector >& patchMap, const string channel, const string DMX, const string KNX);
    void SetFadeTime(const float t);
    void PatchFadeTime(const int KNX);
    void Update(const int KNX, const int val);
    void Refresh(map<int, ola::DmxBuffer>& output);
  private:
    string name;
    map <string, int> channelKNX;
    map <string, int> channelDMX;
    map <string, int> channelValue;
    map <string, float> channelFloatValue;
    float fadeTime, fadeStep;
    int fadeTimeKNX;
};

Fixture::Fixture(const string str) {
  name=str;
  syslog(LOG_DEBUG, "Created Fixture '%s'", (char *)str.c_str());
}

void Fixture::Patch(map<int, svector>& patchMap, const string channel, const int DMX, const int KNX) {
  channelKNX[channel] = KNX;
  channelDMX[channel] = DMX;
  
  syslog(LOG_DEBUG, "Fixture '%s': Patched channel '%s' (KNX %d to DMX %d) ", (char *)name.c_str(), (char *)channel.c_str(), KNX, DMX);
  
  svector assignedChannels = patchMap[KNX]; // check if already patched
  for (vector<string>::iterator i = assignedChannels.begin(); i != assignedChannels.end(); ++i)  {
    if ((*i)==name) { // already patched;
      return;
    }
  }
  
  patchMap[KNX].push_back(name); // no, add to patchMap

}

void Fixture::Patch(map<int, svector>& patchMap, const string channel, const string DMX, const string KNX) {
   Patch(patchMap, channel, readdaddr(DMX), readgaddr((char *) KNX.c_str()));
}

void Fixture::SetFadeTime(const float t) {
  fadeTime = t;
  fadeStep = (t<=0) ? 256 :  256/(t*1e6/FADING_INTERVAL);
  syslog(LOG_DEBUG, "Fixture '%s': Set Fadetime to %.2f s'(%.2f steps/interval)", (char *)name.c_str(), fadeTime, fadeStep);
}

void Fixture::PatchFadeTime(const int KNX) {
  fadeTimeKNX = KNX;
}

void Fixture::Update(const int KNX, const int val) {
  for(std::map<string, int>::const_iterator i = channelKNX.begin(); i != channelKNX.end(); ++i) {
    if (i->second == KNX) {
      channelValue[i->first] = val;
      syslog(LOG_DEBUG, "Updated channel '%s' to %d", (char *)i->first.c_str(), val);
    }
  }
}

void Fixture::Refresh(map<int, ola::DmxBuffer>& output) {
  for(std::map<string, int>::const_iterator i = channelDMX.begin(); i != channelDMX.end(); ++i) {  
    int dmxuniverse = (int) (i->second / 512) , dmxchannel = i->second % 512;
    int oldValue = output[dmxuniverse].Get(dmxchannel);
    int newValue = channelValue[i->first];
    if (oldValue<newValue) {
      channelFloatValue[i->first] += fadeStep;
      if (channelFloatValue[i->first]>newValue) {
        channelFloatValue[i->first] = newValue;
      }
      output[dmxuniverse].SetChannel(dmxchannel, (int) channelFloatValue[i->first]);
    }
    if (oldValue>newValue) {
      channelFloatValue[i->first] -= fadeStep;
      if (channelFloatValue[i->first]<newValue) {
        channelFloatValue[i->first] = newValue;
      }
      output[dmxuniverse].SetChannel(dmxchannel, (int) channelFloatValue[i->first]);
    }
  }
}

}

pthread_mutex_t zonelock = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t initlock = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t standbylock = PTHREAD_MUTEX_INITIALIZER; 

string eibd_url = "local:/tmp/eib";
string conf_file = "knxdmxd.conf";
int pidFilehandle;
string pidfilename = "/var/run/dmxknxd.pid";

map<int, ola::DmxBuffer> dmxWriteBuffer;
map<string, knxdmxd::Fixture> fixtureList;
map<int, svector> patchMap;

void daemonShutdown() {
	//FIXME: clean exit pthread_exit(NULL); pthread_cancel(..);
	//close(udpSocket);		
	syslog(LOG_INFO, "%s daemon exiting", DAEMON_NAME);
    fprintf(stderr, "%s daemon exiting", DAEMON_NAME);	
	close(pidFilehandle);
	unlink((char *)pidfilename.c_str());
	exit(EXIT_SUCCESS);
}

void signal_handler(int sig) {
    switch(sig) {
        case SIGHUP:
            syslog(LOG_WARNING, "Received SIGHUP signal.");
            break;
        case SIGTERM:
            syslog(LOG_WARNING, "Received SIGTERM signal.");
			daemonShutdown();
			break;
        case SIGINT:
            syslog(LOG_WARNING, "Received SIGINT signal.");
			daemonShutdown();
			break;
        default:
            syslog(LOG_WARNING, "Unhandled signal (%d) %s", sig, strsignal(sig));
            break;
    }
}

/*void *sendKNXdgram(int type, int dpt, eibaddr_t dest, unsigned char val) {
	int len = 0;
	EIBConnection *con;
	unsigned char buf[255] = { 0, 0x80 };
	buf[1] = type; //0x40 response, 0x80 write
        syslog(LOG_DEBUG,"Send KNX dgram Type %d DPT %d dest %d val %d",type,dpt,dest,val);
	con = EIBSocketURL ((char *)eibd_url.c_str());
	if (!con)
		syslog(LOG_WARNING,"sendknxdgram: Open failed");
	if (EIBOpenT_Group (con, dest, 1) == -1)
		syslog(LOG_WARNING,"sendknxdgram: Connect failed");
	switch (dpt)
	{
	case 0:
	  len=1;
	  //for(i = 0; i < strlen(val)/2; i++)
	  //raw value: nothing for now
	  break;
	case 1:
	  buf[1] |= val & 0x3f;
	  len=2;
	  break;
	case 3:
	  // EIS2/DPT3 4bit dim
	  buf[1] |= val & 0x3f;
	  len=2;
	  break;
	case 5:
	  buf[2] = val*255/100;
	  len=3;
	  break;
	case 51:
	case 5001:
	  buf[2] = val;
	  len=3;
	  break;
	case 6:
	case 6001:
	  //FIXME: This is basically wrong but as we get a uchar we expect the sender to know..
	  buf[2] = val;
	  len=3;
	  break;
	}

	len = EIBSendAPDU (con, len, buf);
	if (len == -1)
		syslog (LOG_WARNING,"sendknxdram: Request failed");
	EIBClose (con);
	usleep(50*1000); //throttle a little (50ms/max 20tps)
	return 0;
} */

void sigtime(int signo)
{
  for(std::map<string, knxdmxd::Fixture>::const_iterator i = fixtureList.begin(); i != fixtureList.end(); ++i) {
    fixtureList[i->first].Refresh(dmxWriteBuffer);
  }
  signal(SIGALRM, sigtime);
}

void *worker(void *) {
  syslog(LOG_DEBUG, "Internal worker thread started");

  signal(SIGALRM, sigtime);
  
  itimerval itm;
  itm.it_interval.tv_sec=0;
  itm.it_value.tv_sec = 0;
  itm.it_interval.tv_usec = FADING_INTERVAL; // 20 ms is enough
  itm.it_value.tv_usec = FADING_INTERVAL;
  setitimer(ITIMER_REAL,&itm,0);

  while (1) {
    sleep(POLLING_INTERVAL);   
  } 
  
  pthread_exit(NULL);
}

/*void *sendKNXresponse(eibaddr_t dest, int zone, int func) {
	syslog(LOG_DEBUG, "KNX response %d zone %d func %d", dest,zone,func);
	pthread_mutex_lock (&zonelock);
	switch (func) {
		case 0:
		case 20:
			sendKNXdgram (0x40,1,dest,zones[zone].zonepower);
			break;
		case 1:
		case 21:
			sendKNXdgram (0x40,51,dest,zones[zone].srcid);
			break;
		case 2:
		case 22:
			sendKNXdgram (0x40,5,dest,zones[zone].volume*2);
			break;
		case 3:
		case 23:
			if (zones[zone].bass<10)
				sendKNXdgram (0x40,51,dest,zones[zone].bass-10+256);
			else
				sendKNXdgram (0x40,51,dest,zones[zone].bass-10);
			break;
		case 4:
		case 24:
			if (zones[zone].treble<10)
				sendKNXdgram (0x40,51,dest,zones[zone].treble-10+256);
			else
				sendKNXdgram (0x40,51,dest,zones[zone].treble-10);
			break;
		case 5:
		case 25:
			sendKNXdgram (0x40,1,dest,zones[zone].loudness);
			break;
		case 6:
		case 26:
			if (zones[zone].balance<10)
				sendKNXdgram (0x40,51,dest,zones[zone].balance-10+256);
			else
				sendKNXdgram (0x40,51,dest,zones[zone].balance-10);
			break;
		case 27:
			sendKNXdgram (0x40,1,dest,zones[zone].partymode);
			break;
		case 28:
			sendKNXdgram (0x40,1,dest,zones[zone].dnd);
			break;
		case 29:
			sendKNXdgram (0x40,5,dest,zones[zone].onvolume*2);
			break;
	}
	pthread_mutex_unlock (&zonelock);
	return 0;
}
*/
void *handleKNXdgram(eibaddr_t dest, unsigned char* buf, int len){
  unsigned char val;
  switch (buf[1] & 0xC0) {
	case 0x00:
//    sendKNXresponse (dest,zone+(controller*ZONES_PER_CONTROLLER),func);
	  break;
	case 0x40:
	  //FIXME: response dunno
	  break;
	case 0x80:
	  if (buf[1] & 0xC0) {
		if (len == 2)
		  val = buf[1] & 0x3F;
		else 
		  val = buf[2];
        svector uFixtures = patchMap.find(dest)->second;
        for (unsigned int i=0; i < uFixtures.size(); i++) {
          fixtureList[uFixtures[i]].Update(dest, val);
        }
        syslog(LOG_DEBUG, "Received %d @ %d, updated %d fixtures", val, dest, (int) uFixtures.size());
	  }
	  break;
  }
  return 0;
}


void *olahandler(void *) { // thread just reliably sends the data to DMX via OLA
  syslog(LOG_DEBUG, "OLA sender thread started");

  ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);

  while (1) { // retry forever

    ola::StreamingClient ola_client;

    if (!ola_client.Setup()) {  // setup client
      syslog(LOG_WARNING, "OLA:  Client setup failed");
      sleep(RETRY_TIME);
      continue;
    }
    
    syslog(LOG_INFO, "OLA:  Client connected");

    while (1) { // loop forever
      int error_flag=0;
      for(std::map<int, ola::DmxBuffer>::const_iterator i = dmxWriteBuffer.begin(); i != dmxWriteBuffer.end(); ++i) {
        int universe = i->first;
        if (!ola_client.SendDmx(universe, i->second)) { // send all universes
          syslog(LOG_WARNING, "OLA: failed to send universe %d", universe);
      	  error_flag = 1;
          break; // something went wrong
        }
      }
      if (error_flag==1) {
        sleep(RETRY_TIME);  
        break;
      }
      
      usleep(20000);
    }

    ola_client.Stop(); // close the client
  }

  pthread_exit(NULL);
}


void *knxhandler(void *) {
	syslog(LOG_DEBUG, "KNX reader thread started");
	int len;
	EIBConnection *con;
	eibaddr_t dest;
	eibaddr_t src;
	unsigned char buf[255];
	
	while (1) //retry infinite
	{
		con = EIBSocketURL ((char *)eibd_url.c_str());
		if (!con) {
			syslog(LOG_WARNING, "eibd: Open failed");
			sleep(RETRY_TIME);
			continue;
		}

		if (EIBOpen_GroupSocket (con, 0) == -1) {
			syslog(LOG_WARNING, "eibd: Connect failed");
			sleep(RETRY_TIME);
			continue;
		}

		while (1)
		{
			len = EIBGetGroup_Src (con, sizeof (buf), buf, &src, &dest);
			if (len == -1) {
				syslog(LOG_WARNING, "eibd: Read failed");
				sleep(RETRY_TIME);
				break;
			}
			if (len < 2) {
				syslog(LOG_WARNING, "eibd: Invalid Packet");
				break;
			}
			if (buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0) {
				syslog(LOG_WARNING, "eibd: Unknown APDU from %d to %d",src,dest);
				break;
			} else {
				if ( patchMap.count(dest)<=0 ) //not for us
					continue;
				handleKNXdgram(dest,buf,len); 
			}
		}
		syslog(LOG_WARNING,"eibd: closed connection"); //break in read-loop
		EIBClose (con);
	}
	pthread_exit(NULL);
}

void load_config() {

/*  bpt::ptree config;

  read_xml(conf_file, config);

  BOOST_FOREACH( bpt::ptree::value_type const& e, config) {
    if( e.first == "fixture" ) {
       string name = e.second.get<std::string>("<xmlattr>.name");
       knxdmxd::Fixture f(name);
       BOOST_FOREACH( bpt::ptree::value_type const& c, e.second) {
         if (c.first == "channel") {
           f.Patch(patchMap, c.second.data(), c.second.get<std::string>("<xmlattr>.dmx"), c.second.get<std::string>("<xmlattr>.knx"));
         }
         if (c.first == "fading") {
            float t;
            string content = c.second.data();
            sscanf((char*) content.c_str(), "%f", &t);
            f.SetFadeTime((int) (t*1.e6));
         }
       }
       fixtureList[name] = f;
    }
    
  } */
  struct json_object *config;
  
  config = json_object_from_file((char *)conf_file.c_str());
  // first all fixtures
  struct json_object *fixtures = json_object_object_get(config, "fixtures");
 
  int fixturenum = json_object_array_length(fixtures);
 
  for (int i=0; i<fixturenum; i++) { // read all
    // get fixture
    struct json_object *fixture = json_object_array_get_idx(fixtures, i);

    // get name & create
    string fname(json_object_get_string(json_object_object_get(fixture, "name")));
    knxdmxd::Fixture f(fname);
 
    // get channels & patch them
    struct json_object *channels = json_object_object_get(fixture, "channels");
    int channelnum = json_object_array_length(channels);
    for (int j=0; j<channelnum; j++) { // read all
      // get channel
      struct json_object *channel = json_object_array_get_idx(channels, j);
      string cname(json_object_get_string(json_object_object_get(channel, "name")));
      string cdmx(json_object_get_string(json_object_object_get(channel, "dmx")));
      string cknx(json_object_get_string(json_object_object_get(channel, "knx")));
      f.Patch(patchMap, cname, cdmx, cknx);
    }
    
    // get fading
    struct json_object *fading = json_object_object_get(fixture, "fading");
    float ftime = (fading) ? json_object_get_double(json_object_object_get(fading, "time")) : 0;
    f.SetFadeTime(ftime);

    fixtureList[fname] = f;
  }
  
  return;
}

int main(int argc, char **argv) {
	int daemonize = 0;
    int c;
	//char *p;
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
		openlog(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);
        syslog(LOG_DEBUG, "startup with debug; pidfile: %s, eibd: %s", 
               (char *)pidfilename.c_str(), (char *)eibd_url.c_str());
	} else {
	    setlogmask(LOG_UPTO(LOG_INFO));
		openlog(DAEMON_NAME, LOG_CONS, LOG_USER);
	}

    syslog(LOG_INFO, "using config-file %s", (char *)conf_file.c_str());

    load_config();

	pid_t pid, sid;
 
    if (daemonize) {
        syslog(LOG_INFO, "starting daemon");
 
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
		syslog(LOG_INFO, "Could not open pidfile %s, exiting", (char *)pidfilename.c_str());
	    fprintf(stderr, "Could not open pidfile %s, exiting", (char *)pidfilename.c_str());	
		exit(EXIT_FAILURE);
	}
	if (lockf(pidFilehandle,F_TLOCK,0) == -1)
	{
		syslog(LOG_INFO, "Could not lock pidfile %s, exiting", (char *)pidfilename.c_str());
	    fprintf(stderr, "Could not lock pidfile %s, exiting", (char *)pidfilename.c_str());	
		exit(EXIT_FAILURE);
	}
	sprintf(pidstr,"%d\n",getpid());
	c = write(pidFilehandle, pidstr, strlen(pidstr));

	int knxthread, olathread, workerthread;
	pthread_t threads[NUM_THREADS];
	// PTHREAD_CREATE_DETACHED?
	knxthread = pthread_create(&threads[1], NULL, knxhandler, NULL); //id, thread attributes, subroutine, arguments
	olathread = pthread_create(&threads[2], NULL, olahandler, NULL); //id, thread attributes, subroutine, arguments
    workerthread = pthread_create(&threads[3], NULL, worker, NULL); //id, thread attributes, subroutine, arguments
	syslog(LOG_DEBUG,"Threads created: %d %d %d",knxthread, olathread, workerthread);
	//TODO: Maybe another console/TCP-server/Logging thread?
	while (1) {
//		syslog(LOG_DEBUG, "%s daemon running", DAEMON_NAME);
		sleep(POLLING_INTERVAL);
	}
	syslog(LOG_INFO, "%s daemon exiting", DAEMON_NAME);
     // TODO: Free any allocated resources before exiting - we never get here though -> signal handler
    exit(EXIT_SUCCESS);
}
