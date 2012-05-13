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

namespace knxdmxd {

typedef struct { 
  std::string fixture;
  std::string name;
  int value;
} cue_channel_t;

class Cue {
  public:
    Cue() {};
    Cue(const std::string name);

    void AddTrigger(knxdmxd::knx_patch_map_t& patchMap, const int KNX, const int val);    
    void AddTrigger(knxdmxd::knx_patch_map_t& patchMap, const std::string KNX, const int val);
    void AddChannel(const cue_channel_t& channel);
    void SetFading(const float fadeIn, const float fadeOut=-1);
    void SetWaittime(const float waittime);
    void Update(std::map<std::string, knxdmxd::Fixture>& fixtureList);        
    void Update(std::map<std::string, knxdmxd::Fixture>& fixtureList, const int KNX, const int val);

    std::string GetName();
    float GetWaitTime();
        
  private:
    std::string _name;
    int _triggerKNX;
    int _trigger_val;
    std::list<cue_channel_t> _channel_data;
    float _fadeIn, _fadeOut;
    float _waittime;
};


}

#endif
