/*
 * dmxsender.cc
 *
 * (c) by JNK 2012
 *
 *
*/


#include "dmxsender.h"

namespace knxdmxd {

  dmx_addr_t DMX::Address(const std::string addr) {
    char universe;
    int channel;
    sscanf( (char*)addr.c_str(), "%d.%d", &universe, &channel);
    return (channel==-1) ? (universe+512) : ((universe << 9) + channel);
  }

  // initalize static variables of DMX class
  ola::OlaCallbackClientWrapper DMX::m_client;
  std::map<char, knxdmxd::pUniverse> DMX::output;

  bool DMXSender::Init() {
    if (!m_client.Setup()) {
      std::clog << kLogWarning << "DMXSender: OLA client setup failed " << std::endl;
      return false;
    }
    return true;
  }

  int DMXSender::Start() {
    SendDMX();
    m_client.GetSelectServer()->Run();
    sender_running_ = true;
    return 0;
  }

  void Universe::Crossfade() {
   ola::thread::MutexLocker locker(&data_mutex);
   int i;
   int max=512;
   unsigned long currenttime = Timer::Get();

   for(i=511; i>=0; max=i, i--) // find last value
     if(new_[i]||old_[i])
       break;

   for(i=0; i<max; i++) {
     if ((new_[i] || old_[i]) && (new_[i] != old_[i])) { /* avoid calculating with only 0 or finished */
       if (currenttime>fadeend_[i]) {
         current_[i] = old_[i] = new_[i];
       } else {
         float p = (float)(currenttime-fadestart_[i])/(fadeend_[i]-fadestart_[i]);
         float q = 1.0f - p;
         current_[i]=(unsigned char)(old_[i]*q + new_[i]*p);
       }
     }
   }
  }

  void DMXSender::SendDMX() {
    for(std::map<char, pUniverse>::const_iterator i = output.begin(); i !=  output.end(); ++i) {
      int universe = i->first;
      i->second->Crossfade();
      if (!m_client.GetClient()->SendDmx(universe, i->second->GetBuffer())) { // send all universes
        m_client.GetSelectServer()->Terminate();
        std::clog << kLogWarning << "DMXSender: OLA failed to send universe "<< universe << std::endl;
        sender_running_ = false;
        return;
      }
    }

    m_client.GetSelectServer()->RegisterSingleTimeout(
        DMX_INTERVAL,
        ola::NewSingleCallback(this, &DMXSender::SendDMX));
  }
  
  DMXSender::~DMXSender() {
    if (sender_running_)
      m_client.GetSelectServer()->Terminate();
  }

  void DMXSender::Terminate() {
    sender_running_ = false;
    m_client.GetSelectServer()->Terminate();
  }  

}

