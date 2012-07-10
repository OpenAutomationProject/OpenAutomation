/*
 * dmxsender.cc
 *
 * (c) by JNK 2012
 *
 *
*/


#include "dmxsender.h"

namespace knxdmxd {

  bool DMXSender::Init() {
    if (!m_client.Setup()) {
      std::clog << kLogWarning << "OLA: client Setup failed " << std::endl;
      return false;
    }
    return true;
  }

  int DMXSender::Start() {
    SendDMX();
    fixture_list_.StartRefresh();
    m_client.GetSelectServer()->Run();
    sender_running_ = true;
    return 0;
  }

  void DMXSender::SendDMX() {
    for(std::map<int, ola::DmxBuffer>::const_iterator i = output.begin(); i !=  output.end(); ++i) {
      int universe = i->first;
      //std::clog << "C1/2 " << (int) i->second.Get(0) << " " << (int) i->second.Get(1) << " " << (int) i->second.Get(2) << std::endl;
      if (!m_client.GetClient()->SendDmx(universe, i->second)) { // send all universes
        m_client.GetSelectServer()->Terminate();
        std::clog << kLogWarning << "OLA: failed to send universe "<< universe << std::endl;
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

