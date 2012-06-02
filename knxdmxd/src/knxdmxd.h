/*
 * knxdmxd.h
 *
 * (c) by JNK 2012
 *
 *
*/

#ifndef KNXDMXD_H
#define KNXDMXD_H

#include <iostream>
#include <map>
#include <string>
#include <log.h>

#include <eibclient.h>
#include <ola/DmxBuffer.h>
#include <ola/thread/Thread.h>

#define DMX_INTERVAL 25 // in ms

namespace knxdmxd {
  const int kFixture = 1;
  const int kScene = 2;
  const int kCue = 4;
  const int kCuelist = 8;
  
  class DMXSender;
 
  class KNXHandler : public ola::thread::Thread {
    public:
      KNXHandler() : Thread(), m_mutex() { }
      ~KNXHandler() {}

      static eibaddr_t Address(const std::string addr);
      void knxhandler();
      void *Run();

    private:
      ola::thread::Mutex m_mutex;
  };
  
}

#endif


