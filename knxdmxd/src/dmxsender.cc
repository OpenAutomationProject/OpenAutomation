/*
 * dmxsender.cc
 *
 * (c) by JNK 2012
 *
 *
*/


#include "dmxsender.h"

namespace knxdmxd {

bool DMXSender::Init(std::map<int, ola::DmxBuffer> *dmxWriteBuffer) {
  _dmxWriteBuffer = dmxWriteBuffer;
  if (!m_client.Setup()) {
    std::clog << kLogWarning << "OLA: client Setup failed " << std::endl;
    return false;
  }
  
  return true;
}

int DMXSender::Start() {
  SendDMX();
  m_client.GetSelectServer()->Run();
  return 0;
}

void DMXSender::SendDMX() {
  for(std::map<int, ola::DmxBuffer>::const_iterator i = (*_dmxWriteBuffer).begin(); i !=  (*_dmxWriteBuffer).end(); ++i) {
    int universe = i->first;
    if (!m_client.GetClient()->SendDmx(universe, i->second)) { // send all universes
      m_client.GetSelectServer()->Terminate();
      std::clog << kLogWarning << "OLA: failed to send universe "<< universe << std::endl;
    }
  }

  RegisterTimeout();
}

bool DMXSender::RegisterTimeout() {
  m_client.GetSelectServer()->RegisterSingleTimeout(
      DMX_INTERVAL,
      ola::NewSingleCallback(this, &DMXSender::SendDMX));
  return true;
}

DMXSender::~DMXSender() {
  m_client.GetSelectServer()->Terminate();
}

void DMXSender::Terminate() {
  m_client.GetSelectServer()->Terminate();
}


}

