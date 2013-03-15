/*
 * cue.h
 *
 * (c) by JNK 2012
 *
 *
 */

#ifndef CUE_H
#define CUE_H

#include <string.h>
#include <knxdmxd.h>
#include <list>
#include <trigger.h>
#include <dmxsender.h>

namespace knxdmxd
{

  typedef struct
  {
    dmx_addr_t dmx;
    int value;
  } cue_channel_t;

  class Cue : public TriggerHandler
  {
    std::list<cue_channel_t> _channel_data;
    float fadeIn_, fadeOut_;
    float waittime_, delay_;
    bool is_link_, delay_on_;
    // fixture_lock_t lock_;

  public:
    Cue()
    {
    }

    virtual ~Cue() {
      while (!_channel_data.empty()) {
        _channel_data.pop_back();
      }

    }

    Cue(const std::string name, const bool isLink = false);

    void
    AddChannel(const cue_channel_t& channel);
    void
    SetFading(const float fadeIn, const float fadeOut);
    void
    SetWaittime(const float waittime)
    {
      waittime_ = waittime;
    }

    void
    SetDelay(const float delay)
    {
      delay_ = delay;
    }

    void
    Go();
    void
    Release() {}



    const std::string
    GetName()
    {
      return _name;
    }

    const float
    GetWaitTime()
    {
      return waittime_;
    }
    ;
    const float
    GetDelayTime()
    {
      return delay_;
    }
    ;

    bool
    isLink()
    {
      return is_link_;
    }
    ;
  };

  class Cuelist : public TriggerHandler
  {
    int current_cue_, max_cue_;
    bool cue_halted_, was_direct_;
    bool release_on_halt_, proceed_on_go_;
    //   fixture_lock_t lock_;
    std::vector<knxdmxd::Cue> cue_data_;
    std::map<std::string, int> cue_names_;

    void
    NextCue(const int direct);

  public:
    Cuelist()
    {
    }

    Cuelist(const std::string name);

    virtual ~Cuelist() {
      while (!cue_data_.empty()) {
        cue_data_.pop_back();
      }
    }

    void
    SetReleaseOnHalt(const bool release_on_halt)
    {
      release_on_halt_ = release_on_halt;
      std::clog << kLogDebug << "Cue " << _name << ": set release_on_halt to "
          << release_on_halt << std::endl;
    }
    void
    SetProceedOnGo(const bool proceed_on_go)
    {
      proceed_on_go_ = proceed_on_go;
      std::clog << kLogDebug << "Cue " << _name << ": set proceed_on_go to "
          << proceed_on_go << std::endl;
    }

    void
    AddCue(knxdmxd::Cue& cue);

    void
    Go();

    void
    Halt()
    {
      std::clog << kLogDebug << "Cue " << _name << ": halt " << std::endl;
      cue_halted_ = true;
      if (release_on_halt_)
        {
          Release();
        }
    }
    ;
    void
    Direct(const int value)
    {
      NextCue(value);
    }
    ;
    void
    Release()
    {
      std::clog << kLogDebug << "Cue " << _name << ": release " << std::endl;
      current_cue_ = -1;
    }
    ;
  };

}

#endif
