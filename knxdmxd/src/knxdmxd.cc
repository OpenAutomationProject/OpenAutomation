/*
 * knxdmxd.cc
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
#include <ola/StringUtils.h>

#include <eibclient.h>

#include <iostream>
#include <fstream>
#include <map>

#include <knxdmxd.h>
#include <log.h>

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
  std::string
  t_to_string(T i)
  {
    std::stringstream ss;
    ss << i;
    return ss.str();
  }

void
daemonShutdown();

std::string conf_file = "knxdmxd.conf";
int pidFilehandle;
std::string pidfilename = "/var/run/knxdmxd.pid";
knxdmxd::TriggerList triggerList;
knxdmxd::DMXSender sender;

namespace knxdmxd
{
  std::map<std::string, knxdmxd::dmx_addr_t> channel_names;
  std::map<knxdmxd::dmx_addr_t, eibaddr_t> statusmap;

  std::queue<Trigger> KNXHandler::fromKNX;
  ola::thread::Mutex KNXHandler::mutex_fromKNX;
  std::queue<eib_message_t> KNXSender::toKNX;
  ola::thread::Mutex KNXSender::mutex_toKNX;
  ola::thread::ConditionVariable KNXSender::condition_toKNX;

  std::set<eibaddr_t> KNXHandler::listenGA;
  std::map<eibaddr_t, long> KNXHandler::messagetracker;

  eibaddr_t
  KNXHandler::Address(const std::string addr)
  {
    unsigned int a, b, c;
    char *s = (char *) addr.c_str();

    if (sscanf(s, "%u/%u/%u", &a, &b, &c) == 3)
      return ((a & 0x01f) << 11) | ((b & 0x07) << 8) | ((c & 0xff));
    if (sscanf(s, "%u/%u", &a, &b) == 2)
      return ((a & 0x01f) << 11) | ((b & 0x7FF));
    if (sscanf(s, "%x", &a) == 1)
      return a & 0xffff;
    std::clog << kLogWarning << "invalid group address format " << addr
        << std::endl;
    return 0;
  }

  eibaddr_t
  KNXSender::Address(const std::string addr)
  {
    unsigned int a, b, c;
    char *s = (char *) addr.c_str();

    if (sscanf(s, "%u/%u/%u", &a, &b, &c) == 3)
      return ((a & 0x01f) << 11) | ((b & 0x07) << 8) | ((c & 0xff));
    if (sscanf(s, "%u/%u", &a, &b) == 2)
      return ((a & 0x01f) << 11) | ((b & 0x7FF));
    if (sscanf(s, "%x", &a) == 1)
      return a & 0xffff;
    std::clog << kLogWarning << "invalid group address format " << addr
        << std::endl;
    return 0;
  }

  void
  KNXSender::knxsender()
  {
    std::clog << "KNX sender thread started" << std::endl;
    int len;
    EIBConnection *con;
    unsigned char buf[255];
    knxdmxd::eib_message_t message;
    while (1)
      {
        ola::thread::MutexLocker locker(&KNXSender::mutex_toKNX);

        if (!KNXSender::toKNX.empty())
          { // there is something to send

            message = KNXSender::toKNX.front();
            KNXSender::toKNX.pop();

            con = EIBSocketURL((char *) eibd_url_.c_str());
            if (!con)
              {
                std::clog << kLogWarning
                    << "KNXHandler: connection to eibd failed, message not sent"
                    << std::endl;
                sleep(RETRY_TIME);
                continue;
              }

            if (EIBOpenT_Group(con, message.ga, 1) == -1)
              {
                std::clog << kLogWarning
                    << "KNXSender: socket failed, message not sent"
                    << std::endl;
                sleep(RETRY_TIME);
                continue;
              }

            len = 0;

            buf[0] = 0;
            buf[1] = 0x80;

            switch (message.dpt)
              {
            case 1:
              buf[1] |= message.value & 0x3f;
              len = 2;
              break;
            case 5:
              buf[2] = message.value;
              len = 3;
              break;
              }

            len = EIBSendAPDU(con, len, buf);
            if (len == -1)
              {
                std::clog << kLogDebug << "KNXSender: failed to sent "
                    << (int) message.value << " to " << message.ga << " (DPT: "
                    << (int) message.dpt << ")" << std::endl;

              }
            else
              {
                std::clog << kLogDebug << "KNXSender: sent "
                    << (int) message.value << " to " << message.ga << " (DPT: "
                    << (int) message.dpt << ")" << std::endl;
              }
            usleep(25000); // wait 25 ms, max. 40 tps
            EIBClose(con);

          }
        else
          {
            std::clog << kLogDebug << "KNX sender waiting for message"
                << std::endl;
            KNXSender::condition_toKNX.Wait(&KNXSender::mutex_toKNX);
            std::clog << kLogDebug << "KNX sender resumed" << std::endl;

          }
      }
  }

  void
  KNXHandler::knxhandler()
  {
    std::clog << "KNX handler thread started" << std::endl;
    int len;
    EIBConnection *con;
    eibaddr_t dest;
    eibaddr_t src;
    unsigned char buf[255], val;

    while (1)
      { //retry infinite
        con = EIBSocketURL((char *) eibd_url_.c_str());
        if (!con)
          {
            std::clog << kLogWarning << "KNXHandler: connection to eibd failed"
                << std::endl;
            sleep(RETRY_TIME);
            continue;
          }

        if (EIBOpen_GroupSocket(con, 0) == -1)
          {
            std::clog << kLogWarning << "KNXHandler: socket failed"
                << std::endl;
            sleep(RETRY_TIME);
            continue;
          }

        while (1)
          {
            triggerList.Process();
            len = EIBGetGroup_Src(con, sizeof(buf), buf, &src, &dest);
            if (len == -1)
              {
                std::clog << kLogWarning << "KNXHandler: Read failed"
                    << std::endl;
                sleep(RETRY_TIME);
                break;
              }

            if (len < 2)
              {
                std::clog << kLogWarning << "KNXHandler: Invalid Packet"
                    << std::endl;
                break;
              }

            if ((buf[0] & 0x3) || (buf[1] & 0xC0) == 0xC0)
              {
                std::clog << kLogWarning << "KNXHandler: Unknown APDU from "
                    << src << " to " << dest << std::endl;
                break;
              }
            else
              {
                switch (buf[1] & 0xC0)
                  {
                case 0x00:
                  break;
                case 0x40:
                  //FIXME: response dunno
                  break;
                case 0x80:
                  if (buf[1] & 0xC0)
                    {
                      val = (len == 2) ? buf[1] & 0x3F : buf[2];
                      if (KNXHandler::listenGA.count(dest))
                        { // keep queue clean from unwanted messages
                          std::clog << kLogDebug << "KNXHandler: " << dest
                              << " " << (int) val << std::endl;
                          knxdmxd::Trigger trigger(knxdmxd::kTriggerAll, dest,
                              val);
                          ola::thread::MutexLocker locker(
                              &KNXHandler::mutex_fromKNX);
                          KNXHandler::fromKNX.push(trigger);
                        }

                    }
                  break;
                  }
              }
          }

        std::clog << kLogWarning << "eibd: Closed connection" << std::endl; //break in read-loop
        EIBClose(con);
      }

  }

  void
  OLAThread::olahandler()
  { // thread for OLA connection
    std::clog << kLogDebug << "OLA thread started" << std::endl;

    ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);

    while (1)
      { // retry forever
        if (!sender.Init())
          {
            std::clog << kLogWarning << "OLA: Client setup failed" << std::endl;
            sleep(RETRY_TIME);
            continue;
          }

        sender.Start(); // Start the sender

        while (1)
          { // loop forever, should be used for monitoring the client
            if (!sender.Running())
              break;
            usleep(20000);
          }
      }
  }
}

void
daemonShutdown()
{
  //FIXME: clean exit pthread_exit(NULL); pthread_cancel(..);
  std::clog << kLogInfo << DAEMON_NAME << " daemon exiting" << std::endl;
  fprintf(stderr, "%s daemon exiting", DAEMON_NAME);
  sender.Terminate();
  close(pidFilehandle);
  unlink((char *) pidfilename.c_str());
  exit(EXIT_SUCCESS);
}

void
signal_handler(int sig)
{
  switch (sig)
    {
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
    std::clog << kLogWarning << "Unhandled signal (" << sig << ") "
        << strsignal(sig) << std::endl;
    break;
    }
}

knxdmxd::Trigger *
json_get_trigger(struct json_object *trigger, const int type)
{

  if (!trigger)
    {
      return NULL;
    }

  struct json_object *trigger_knx = json_object_object_get(trigger, "knx");
  struct json_object *trigger_value = json_object_object_get(trigger, "value");

  if (!trigger_knx)
    {
      return NULL;
    }

  eibaddr_t knx = knxdmxd::KNXHandler::Address(
      json_object_get_string(trigger_knx));
  int val = (trigger_value) ? json_object_get_int(trigger_value) : 256;

  knxdmxd::KNXHandler::listenGA.insert(knx);
  return new knxdmxd::Trigger(type, knx, val);
}

knxdmxd::Cue *
json_get_cue(struct json_object *cue, const int cuenum, const int type)
{

  // check link first
  struct json_object *link = json_object_object_get(cue, "link");
  if (link)
    { // is link
      knxdmxd::Cue *new_cue = new knxdmxd::Cue(json_object_get_string(link),
          true);
      return new_cue;
    }

  struct json_object *name = json_object_object_get(cue, "name");
  std::string c_name =
      (name) ? json_object_get_string(name) : "_c_" + t_to_string(cuenum);
  knxdmxd::Cue *new_cue = new knxdmxd::Cue(c_name);

  // get channels
  struct json_object *channels = json_object_object_get(cue, "data");
  if (!channels)
    {
      std::clog << kLogInfo << "Skipping cue '" << c_name
          << "' (no channels defined)" << std::endl;
      delete new_cue;
      return new_cue;
    }

  for (int j = 0; j < json_object_array_length(channels); j++)
    { // read all
// get channel
      struct json_object *channel = json_object_array_get_idx(channels, j);

      struct json_object *chan = json_object_object_get(channel, "channel");
      struct json_object *value = json_object_object_get(channel, "value");

      if ((!chan) || (!value))
        {
          std::clog << kLogInfo << "Skipping errorneous channel def " << j
              << " in cue '" << c_name << "'" << std::endl;
          continue;
        }
      std::string chan_str = json_object_get_string(chan);
      if (!knxdmxd::channel_names.count(chan_str))
        {
          std::clog << kLogInfo << "Skipping errorneous channel def " << j
              << " in cue '" << c_name << "'" << std::endl;
          continue;
        }
      knxdmxd::dmx_addr_t dmx = knxdmxd::channel_names.find(chan_str)->second;

      knxdmxd::cue_channel_t channeldata;

      channeldata.dmx = dmx;
      channeldata.value = json_object_get_int(value);

      // add
      new_cue->AddChannel(channeldata);
    }

  // fading

  struct json_object *fading = json_object_object_get(cue, "fading");
  if (fading)
    {
      struct json_object *fading_time = json_object_object_get(fading, "time");
      if (!fading_time)
        {
          struct json_object *fading_time_in = json_object_object_get(fading,
              "in");
          struct json_object *fading_time_out = json_object_object_get(fading,
              "out");

          if ((!fading_time_in) || (!fading_time_out))
            {
              std::clog << kLogInfo << "Skipping errorneous fading def in cue '"
                  << c_name << "'" << std::endl;
            }
          else
            {
              new_cue->SetFading(json_object_get_double(fading_time_in),
                  json_object_get_double(fading_time_out));
            }
        }
      else
        {
          new_cue->SetFading(json_object_get_double(fading_time),
              json_object_get_double(fading_time));
        }
    }

  // waittime

  struct json_object *waittime = json_object_object_get(cue, "waittime");
  if (waittime)
    {
      new_cue->SetWaittime(json_object_get_double(waittime));
    }

  struct json_object *delay = json_object_object_get(cue, "delay");
  if (delay)
    {
      new_cue->SetDelay(json_object_get_double(delay));
    }

  if (type == knxdmxd::kScene)
    {
      struct json_object *triggers = json_object_object_get(cue, "trigger");
      if (!triggers)
        {
          std::clog << kLogInfo << "Skipping cue '" << c_name
              << "' (trigger required in scene definition)" << std::endl;
          delete new_cue;
          return new_cue;
        }

      struct json_object *go = json_object_object_get(triggers, "go");
      knxdmxd::Trigger *go_trigger = json_get_trigger(go, knxdmxd::kTriggerGo);
      if (go_trigger)
        {
          triggerList.Add(*go_trigger, new_cue);
        }
    }

  return new_cue;
}

void
load_config()
{

  struct json_object *config;
  char *cfg_name = (char*) conf_file.c_str();

  std::ifstream cf(cfg_name);
  if (!cf.good())
    {
      std::clog << kLogErr << "Error opening " << conf_file
          << ", check position and/or permissions " << std::endl;
      exit(EXIT_FAILURE);
    }

  try
    {
      config = json_object_from_file(cfg_name);
    }
  catch (char *str)
    {
      std::clog << kLogErr << "Exception " << str
          << " while trying to parse config file " << std::endl;
      exit(EXIT_FAILURE);
    }
  /*
   * patch table
   */

  struct json_object *in_data = json_object_object_get(config, "patch");
  int in_length = (in_data) ? json_object_array_length(in_data) : 0;
  if (in_length>0) {
    std::clog << "Trying to import " << in_length << " patche(s)" << std::endl;
  } else {
    std::clog << kLogInfo << "No patches defined, manual output patch needed" << std::endl;
  }
  for (int i = 0; i < in_length; i++)
      { // read all
        struct json_object *element = json_object_array_get_idx(in_data, i);
        struct json_object *d = json_object_object_get(element, "device");
        struct json_object *p = json_object_object_get(element, "port");
        struct json_object *u = json_object_object_get(element, "universe");

        if ((!d) || (!p) || (!u)) { std::clog << kLogInfo << "Skipping errorneous patch " << i+1 << std::endl;
          continue;
        }

        knxdmxd::DMX::Patch(json_object_get_int(d), json_object_get_int(p), json_object_get_int(u));
      }

  /*
   * channel definitions
   */

  in_data = json_object_object_get(config, "channels");
  in_length = json_object_array_length(in_data);
  std::clog << "Trying to import " << in_length << " channel(s)" << std::endl;

  for (int i = 0; i < in_length; i++)
    { // read all
      struct json_object *element = json_object_array_get_idx(in_data, i);
      struct json_object *name = json_object_object_get(element, "name");

      std::string name_str =
          (name) ? json_object_get_string(name) : "_f_" + t_to_string(i);
      struct json_object *dmx = json_object_object_get(element, "dmx");

      if (!dmx)
        {
          std::clog << kLogInfo << "Skipping channel '" << name_str
              << " (missing dmx)" << std::endl;
          continue;
        }
      std::string s = json_object_get_string(dmx);

      knxdmxd::dmx_addr_t dmx_addr(knxdmxd::DMX::Address(s));
      sender.AddUniverse((char) (dmx_addr / 512));
      knxdmxd::channel_names.insert(
          std::pair<std::string, knxdmxd::dmx_addr_t>(name_str, dmx_addr));
      std::clog << kLogDebug << "Named DMX " << dmx_addr << " as " << name_str;
      struct json_object *ga = json_object_object_get(element, "statusga");
      if (!ga)
        {
          std::clog << std::endl;
          continue;
        }

      std::string ga_str = json_object_get_string(ga);
      knxdmxd::statusmap.insert(
          std::pair<knxdmxd::dmx_addr_t, eibaddr_t>(dmx_addr,
              knxdmxd::KNXHandler::Address(ga_str)));
      std::clog << " (GA: " << ga_str << ")" << std::endl;

    }

  /*
   * dimmers
   */

  in_data = json_object_object_get(config, "dimmers");
  in_length = json_object_array_length(in_data);
  std::clog << kLogDebug << "Trying to import " << in_length << " dimmer(s)"
      << std::endl;

  for (int i = 0; i < in_length; i++)
    { // read all
// get fixture
      struct json_object *element = json_object_array_get_idx(in_data, i);

      // get name & create
      struct json_object *name = json_object_object_get(element, "name");
      std::string name_str =
          (name) ? json_object_get_string(name) : "_d_" + t_to_string(i);

      // get channel
      struct json_object *channel = json_object_object_get(element, "channel");
      if (!channel)
        {
          std::clog << kLogInfo << "Skipping dimmer '" << name_str
              << "' (no channels defined)" << std::endl;
          continue;
        }
      std::string channel_str = json_object_get_string(channel);
      if (!knxdmxd::channel_names.count(channel_str))
        {
          std::clog << kLogInfo << "Skipping dimmer '" << name_str
              << "' (invalid channel name)" << std::endl;
          continue;
        }
      knxdmxd::dmx_addr_t dmx = knxdmxd::channel_names.find(channel_str)->second;

      struct json_object *ga = json_object_object_get(element, "ga");
      if (!ga)
        {
          std::clog << kLogInfo << "Skipping dimmer '" << name_str
              << "' (no group address defined)" << std::endl;
          continue;
        }

      eibaddr_t ga_ = knxdmxd::KNXHandler::Address(json_object_get_string(ga));

      knxdmxd::Dimmer* d = new knxdmxd::Dimmer(name_str, ga_, dmx);

      knxdmxd::KNXHandler::listenGA.insert(ga_);
      knxdmxd::Trigger* trigger = new knxdmxd::Trigger(knxdmxd::kTriggerProcess,
          ga_, 256);
      triggerList.Add(*trigger, d);

      // get fading
      struct json_object *fading = json_object_object_get(element, "fading");
      if (fading)
        {
          d->SetFadeTime(json_object_get_double(fading));
        }

    }

  /*
   * scenes
   */

  in_data = json_object_object_get(config, "scenes");
  in_length = json_object_array_length(in_data);
  std::clog << "Trying to import " << in_length << " scene(s)" << std::endl;

  for (int i = 0; i < in_length; i++)
    { // read all
      struct json_object *scene = json_object_array_get_idx(in_data, i);

      // get name & create
      struct json_object *name = json_object_object_get(scene, "name");
      std::string sname =
          (name) ? json_object_get_string(name) : "_s_" + t_to_string(i);

      knxdmxd::Cue *s = json_get_cue(scene, i, knxdmxd::kScene);
    }

  /*
   * cuelists
   */

  struct json_object *cuelists = json_object_object_get(config, "cuelists");
  int cuelistnum = json_object_array_length(cuelists);
  std::clog << kLogDebug << "Trying to import " << cuelistnum << " cuelist(s)"
      << std::endl;

  for (int i = 0; i < cuelistnum; i++)
    { // read all
      struct json_object *cuelist = json_object_array_get_idx(cuelists, i);

      // get name & create
      struct json_object *name = json_object_object_get(cuelist, "name");
      std::string cname =
          (name) ? json_object_get_string(name) : "_c_" + t_to_string(i);
      knxdmxd::Cuelist *c = new knxdmxd::Cuelist(cname);

      struct json_object *roh = json_object_object_get(cuelist, "release_on_halt");
      if (roh) {
          bool rohval = json_object_get_boolean(roh);
          c->SetReleaseOnHalt(rohval);
      }

      struct json_object *pog = json_object_object_get(cuelist, "proceed_on_go");
            if (pog) {
                bool pogval = json_object_get_boolean(roh);
                c->SetProceedOnGo(pogval);
            }

      // get cues
      struct json_object *cues = json_object_object_get(cuelist, "cues");
      for (int i = 0; i < json_object_array_length(cues); i++)
        { // read all
          knxdmxd::Cue* cue = json_get_cue(json_object_array_get_idx(cues, i),
              i, knxdmxd::kCue);
          c->AddCue(*cue);
        }

      // trigger is required

      struct json_object *triggers = json_object_object_get(cuelist, "trigger");
      if (!triggers)
        {
          std::clog << kLogInfo << "Skipping cuelist '" << name
              << "' (trigger required in cuelist definition)" << std::endl;
          continue;
        }

      knxdmxd::Trigger *trigger;

      trigger = json_get_trigger(json_object_object_get(triggers, "go"),
          knxdmxd::kTriggerGo);
      if (trigger)
        {
          triggerList.Add(*trigger, c);
        }

      trigger = json_get_trigger(json_object_object_get(triggers, "halt"),
          knxdmxd::kTriggerHalt);
      if (trigger)
        {
          triggerList.Add(*trigger, c);
        }

      trigger = json_get_trigger(json_object_object_get(triggers, "direct"),
          knxdmxd::kTriggerDirect);
      if (trigger)
        {
          triggerList.Add(*trigger, c);
        }

      trigger = json_get_trigger(json_object_object_get(triggers, "release"),
          knxdmxd::kTriggerRelease);
      if (trigger)
        {
          triggerList.Add(*trigger, c);
        }
    }

  return;
}

int
main(int argc, char **argv)
{

  int daemonize = 0;
  int c;
  char pidstr[255];

  knxdmxd::KNXHandler knxhandler;
  knxdmxd::KNXSender knxsender;

  knxdmxd::OLAThread ola;

  while ((c = getopt(argc, argv, "dp:u:c:")) != -1)
    switch (c)
      {
    case 'd':
      daemonize = 1;
      break;
    case 'p':
      pidfilename.assign(optarg);
      break;
    case 'u':
      knxsender.SetEIBDURL(optarg);
      knxhandler.SetEIBDURL(optarg);
      break;
    case 'c':
      conf_file.assign(optarg);
      break;
    case '?':
      //FIXME: check arguments better, print_usage
      fprintf(stderr, "Unknown option `-%c'.\nUsage: %s %s", optopt, argv[0],
          USAGESTRING);
      return 1;
    default:
      abort();
      break;
      }

  //FIXME: clean shutdown in sub-thread with signals?
  signal(SIGHUP, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGQUIT, signal_handler);

  if (!daemonize)
    {
      setlogmask(LOG_UPTO(LOG_DEBUG));
      std::clog.rdbuf(
          new Log(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID,
              LOG_USER));
      std::clog << kLogDebug << "startup with debug; pidfile: " << pidfilename
          << ", eibd: " << knxhandler.GetEIBDURL() << std::endl;
    }
  else
    {
      setlogmask(LOG_UPTO(LOG_INFO));
      std::clog.rdbuf(new Log(DAEMON_NAME, LOG_CONS, LOG_USER));
    }
  std::clog << kLogInfo << DAEMON_NAME << " compiled on " << __DATE__ << " "
      << __TIME__ << " with GCC " << __VERSION__ << std::endl;
  std::clog << kLogInfo << "using config-file " << conf_file << std::endl;

  pid_t pid, sid;

  if (daemonize)
    {
      std::clog << kLogInfo << "starting daemon" << std::endl;

      pid = fork();
      if (pid < 0)
        {
          exit(EXIT_FAILURE);
        }
      if (pid > 0)
        {
          exit(EXIT_SUCCESS);
        }
      umask(0);
      sid = setsid();
      if (sid < 0)
        {
          exit(EXIT_FAILURE);
        }
      if ((chdir("/")) < 0)
        {
          exit(EXIT_FAILURE);
        }
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
    }

  //FIXME: output errors to stderr, change order
  pidFilehandle = open((char *) pidfilename.c_str(), O_RDWR | O_CREAT, 0600);
  if (pidFilehandle == -1)
    {

      std::clog << kLogInfo << "Could not open pidfile " << pidfilename
          << ", exiting" << std::endl;
      fprintf(stderr, "Could not open pidfile %s, exiting",
          (char *) pidfilename.c_str());
      exit(EXIT_FAILURE);
    }
  if (lockf(pidFilehandle, F_TLOCK, 0) == -1)
    {
      std::clog << kLogInfo << "Could not lock pidfile " << pidfilename
          << ", exiting" << std::endl;
      fprintf(stderr, "Could not lock pidfile %s, exiting",
          (char *) pidfilename.c_str());
      exit(EXIT_FAILURE);
    }
  sprintf(pidstr, "%d\n", getpid());
  c = write(pidFilehandle, pidstr, strlen(pidstr));

  load_config();

  knxhandler.Start();
  knxsender.Start();
  ola.Start();

  while (1)
    {
//		std::clog << DAEMON_NAME << " daemon running" << std::endl;
      sleep(POLLING_INTERVAL * 1000);
    }
  std::clog << kLogInfo << DAEMON_NAME << " daemon exiting" << std::endl;
  // TODO: Free any allocated resources before exiting - we never get here though -> signal handler
  exit(EXIT_SUCCESS);
}
