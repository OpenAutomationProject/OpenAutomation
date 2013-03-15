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
#include <queue>
#include <set>
#include <eibclient.h>
#include <ola/DmxBuffer.h>
#include <ola/thread/Thread.h>
#include <sys/time.h>

#define DMX_INTERVAL 50 // in ms
namespace knxdmxd
{
  const int kFixture = 1;
  const int kScene = 2;
  const int kCue = 4;
  const int kCuelist = 8;

  typedef long dmx_addr_t;

  typedef struct
  {
    eibaddr_t ga;
    long value;
    char dpt; // 1 or 5 !
  } eib_message_t;

  class DMXSender;
  class Trigger;

  class Timer
  {
  private:
    unsigned long start_time_;
  public:
    Timer()
    {
      start_time_ = Get();
    }
    static unsigned long
    Get()
    {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      return (unsigned long) tv.tv_sec * 1000UL
          + (unsigned long) tv.tv_usec / 1000;
    }
  };

  class KNXHandler : public ola::thread::Thread
  {
  public:
    KNXHandler() :
        Thread(), m_mutex()
    {
      eibd_url_ = "local:/tmp/eib";
      exit_ = false;
      handler_exit_ = false;
    }
    virtual
    ~KNXHandler()
    {
    }
    static std::queue<Trigger> fromKNX;
    static ola::thread::Mutex mutex_fromKNX;

    static std::set<eibaddr_t> listenGA;
    static std::map<eibaddr_t, long> messagetracker;

    static eibaddr_t
    Address(const std::string addr);
    void
    knxhandler();
    void
    knxsender();
    void
    SetEIBDURL(std::string EIBD_URL)
    {
      eibd_url_ = EIBD_URL;
    }
    const std::string&
    GetEIBDURL()
    {
      return eibd_url_;
    }
    void *
    Run()
    {
      ola::thread::MutexLocker locker(&m_mutex);
      knxhandler();
      return NULL;
    }
    void
    Stop()
    {
      exit_ = true;
    }
    bool
    stopped()
    {
      return handler_exit_;
    }
  private:
    ola::thread::Mutex m_mutex;
    std::string eibd_url_;
    std::map<eibaddr_t, unsigned long> lastmsg;
    bool exit_;
    bool handler_exit_;
  };

  class KNXSender : public ola::thread::Thread
  {
  public:
    KNXSender() :
        Thread(), m_mutex()
    {
      eibd_url_ = "local:/tmp/eib";
      exit_ = false;
      handler_exit_ = false;
    }

    virtual
    ~KNXSender()
    {
    }

    static std::queue<eib_message_t> toKNX;
    static ola::thread::Mutex mutex_toKNX;
    static ola::thread::ConditionVariable condition_toKNX;

    static eibaddr_t
    Address(const std::string addr);
    void
    knxsender();
    void
    SetEIBDURL(std::string EIBD_URL)
    {
      eibd_url_ = EIBD_URL;
    }
    const std::string&
    GetEIBDURL()
    {
      return eibd_url_;
    }
    void *
    Run()
    {
      ola::thread::MutexLocker locker(&m_mutex);
      knxsender();
      return NULL;
    }
    void
    Stop()
    {
      exit_ = true;
    }
    bool
    stopped()
    {
      return handler_exit_;
    }

  private:
    ola::thread::Mutex m_mutex;
    std::string eibd_url_;
    std::map<eibaddr_t, unsigned long> lastmsg;
    bool exit_;
    bool handler_exit_;
  };

  class OLAThread : public ola::thread::Thread
  {
  public:
    OLAThread() :
        Thread(), m_mutex()
    {
      exit_ = false;
      handler_exit_ = false;
    }
    ~OLAThread()
    {
    }

    void
    olahandler();

    void *
    Run()
    {
      ola::thread::MutexLocker locker(&m_mutex);
      olahandler();
      return NULL;
    }

    void
    Stop();

    bool
    stopped()
    {
      return handler_exit_;
    }

  private:
    ola::thread::Mutex m_mutex;
    bool exit_;
    bool handler_exit_;
  };

  extern std::map<std::string, knxdmxd::dmx_addr_t> channel_names;
  extern std::map<knxdmxd::dmx_addr_t, eibaddr_t> statusmap;

}

#endif

