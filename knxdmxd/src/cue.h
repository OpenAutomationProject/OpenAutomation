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
#include <fixture.h>
#include <trigger.h>
#include <dmxsender.h>

namespace knxdmxd {

  typedef struct { 
    pFixture fixture;
    std::string name;
    int value;
  } cue_channel_t;

  class Cue : public TriggerHandler {
      std::list<cue_channel_t> _channel_data;
      float fadeIn_, fadeOut_;
      float waittime_, delay_;
      bool is_link_;

    public:
      Cue() {};
      Cue(const std::string name, const bool isLink=false);
 
      void AddChannel(const cue_channel_t& channel);
      void SetFading(const float fadeIn, const float fadeOut=-1);
      void SetWaittime(const float waittime) { waittime_ = waittime; };
      void SetDelay(const float delay) { delay_ = delay; };
      
      const std::string GetName() { return _name; };
      const float GetWaitTime() { return waittime_; };
      const float GetDelayTime() { return delay_; };

      virtual void Go();
      bool isLink() { return is_link_; };
  };

  class Cuelist : public TriggerHandler {
      int current_cue_;
      bool cue_halted_, was_direct_;
      std::vector<knxdmxd::Cue> _cue_data;
      std::map<std::string, int> _cue_names;

      void NextCue(const int direct);

    public:
      Cuelist() {};
      Cuelist(const std::string name);

      void AddCue(knxdmxd::Cue& cue);
      void Go();
      void Halt();
      void Direct(const int value);
  };

}

#endif
