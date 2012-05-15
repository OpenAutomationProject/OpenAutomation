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

namespace knxdmxd {

class DMXSender {
  public:
    DMXSender() {};
    ~DMXSender();

    bool Init(std::map<int, ola::DmxBuffer> *dmxWriteBuffer);
    int Start();
    void SendDMX();
    bool RegisterTimeout();
    void Terminate();
    
  private:
//    unsigned int m_tick;
    std::map<int, ola::DmxBuffer> *_dmxWriteBuffer;

  //  ola::DmxBuffer m_buffer;
    ola::OlaCallbackClientWrapper m_client;
};


}

#endif
