/*
 * dmxsender.h
 *
 * (c) by JNK 2012
 *
 *
*/

#ifndef DMXSENDER_H
#define DMXSENDER_H

#include <string.h>
#include <knxdmxd.h>
#include <ola/DmxBuffer.h>
#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>
#include <map>
#include <fixture.h>

namespace knxdmxd {

class DMXSender : private DMX {
    FixtureList fixture_list_;
    bool sender_running_;
    
  public:
    DMXSender() { sender_running_ = false; };
    ~DMXSender();

    bool Init();
    int Start();
    void SendDMX();
    bool RegisterTimeout();
    
    void RefreshFixtures();
    bool RegisterFixtureTimeout();
    
    void Terminate();
    bool Running() { return sender_running_; };
   
    void AddFixture(pFixture fixture) { fixture_list_.Add(fixture); };
    pFixture GetFixture(const std::string& name) { return fixture_list_.Get(name); };
    static ola::OlaCallbackClientWrapper& GetOLAClient() { return m_client; }; 

    void Process(const Trigger& trigger) { fixture_list_.Process(trigger); };

  private:
    static ola::OlaCallbackClientWrapper m_client;
};


}

#endif
