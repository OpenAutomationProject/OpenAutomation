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
      float _fadeIn, _fadeOut;
      float _waittime;
      bool _is_link;

    public:
      Cue() {};
      Cue(const std::string name, const bool isLink=false);

      void AddChannel(const cue_channel_t& channel);
      void SetFading(const float fadeIn, const float fadeOut=-1);
      void SetWaittime(const float waittime);

      const std::string GetName() { return _name; };
      const float GetWaitTime() { return _waittime; };

      virtual void Go();
      bool isLink();
  };

  class Cuelist : public TriggerHandler {
      int _current_cue;
      bool _cue_halted;
      std::vector<knxdmxd::Cue> _cue_data;
      std::map<std::string, int> _cue_names;

      void NextCue();

    public:
      Cuelist() {};
      Cuelist(const std::string name);

      void AddCue(knxdmxd::Cue& cue);
      void Go();
      void Halt();        
  };

}

#endif
