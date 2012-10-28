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
    ;
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
    ;
    void
    SetDelay(const float delay)
    {
      delay_ = delay;
    }
    ;
    //     void SetLock(const fixture_lock_t lock) { lock_ = lock; };

    void
    Go();
    void
    Release()
    {
      /*for(std::list<cue_channel_t>::iterator it = _channel_data.begin(); it != _channel_data.end(); ++it) {
       it->fixture->Release(lock_);
       }*/
    }
    ;

    const std::string
    GetName()
    {
      return _name;
    }
    ;
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
    bool release_on_halt_;
    //   fixture_lock_t lock_;
    std::vector<knxdmxd::Cue> cue_data_;
    std::map<std::string, int> cue_names_;

    void
    NextCue(const int direct);

  public:
    Cuelist()
    {
    }
    ;
    Cuelist(const std::string name);

    void
    AddCue(knxdmxd::Cue& cue);
    void
    Go()
    {
      if (cue_halted_)
        {
          cue_halted_ = false;
          NextCue(-1);
        }
      else
        {
          NextCue(-2);
        }

    }
    ;
    void
    Halt()
    {
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
      cue_halted_ = true;
      current_cue_ = -1;
    }
    ;
  };

}

#endif
