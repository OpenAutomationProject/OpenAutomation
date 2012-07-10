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

#define DMX_INTERVAL 50 // in ms
namespace knxdmxd {
  const int kFixture = 1;
  const int kScene = 2;
  const int kCue = 4;
  const int kCuelist = 8;

  class DMXSender;

  class KNXHandler: public ola::thread::Thread {
    public:
      KNXHandler() :
          Thread(), m_mutex() {
        eibd_url_= "local:/tmp/eib";
      }
      ~KNXHandler() {
      }

      static eibaddr_t Address(const std::string addr);
      void knxhandler();

      void SetEIBDURL(std::string EIBD_URL) {
        eibd_url_ = EIBD_URL;
      }
      const std::string& GetEIBDURL() {
              return eibd_url_;
            }
      void *Run() {
        ola::thread::MutexLocker locker(&m_mutex);
        knxhandler();
        return NULL;
      }

    private:
      ola::thread::Mutex m_mutex;
      std::string eibd_url_;
  };

  class OLAThread: public ola::thread::Thread {
    public:
      OLAThread() :
          Thread(), m_mutex() {
      }
      ~OLAThread() {
      }

      void olahandler();

      void *Run() {
        ola::thread::MutexLocker locker(&m_mutex);
        olahandler();
        return NULL;
      }

    private:
      ola::thread::Mutex m_mutex;
  };

}

#endif

