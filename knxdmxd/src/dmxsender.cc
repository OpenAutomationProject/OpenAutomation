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
  RefreshFixtures();
  m_client.GetSelectServer()->Run();
  sender_running_ = true;
  return 0;
}

void DMXSender::SendDMX() {
  for(std::map<int, ola::DmxBuffer>::const_iterator i = output.begin(); i !=  output.end(); ++i) {
    int universe = i->first;
    if (!m_client.GetClient()->SendDmx(universe, i->second)) { // send all universes
      m_client.GetSelectServer()->Terminate();
      std::clog << kLogWarning << "OLA: failed to send universe "<< universe << std::endl;
      sender_running_ = false;
      return;
    }
  }

  RegisterTimeout();
}

void DMXSender::RefreshFixtures() {
  fixture_list_.Refresh();
  RegisterFixtureTimeout();
}

bool DMXSender::RegisterTimeout() {
  m_client.GetSelectServer()->RegisterSingleTimeout(
      DMX_INTERVAL,
      ola::NewSingleCallback(this, &DMXSender::SendDMX));
  return true;
}

bool DMXSender::RegisterFixtureTimeout() {
  m_client.GetSelectServer()->RegisterSingleTimeout(
      DMX_INTERVAL,
      ola::NewSingleCallback(this, &DMXSender::RefreshFixtures));
  return true;
}

DMXSender::~DMXSender() {
//  m_client.GetSelectServer()->Terminate();
}

void DMXSender::Terminate() {
  sender_running_ = false;
  m_client.GetSelectServer()->Terminate();
}

ola::OlaCallbackClientWrapper DMXSender::m_client;

}

