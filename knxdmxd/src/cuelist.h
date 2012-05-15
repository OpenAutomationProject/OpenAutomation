/*
 * cuelist.h
 *
 * (c) by JNK 2012
 *
 *
*/

#ifndef CUELIST_H
#define CUELIST_H

#include <string.h>
#include <knxdmxd.h>
#include <deque>
#include <fixture.h>
#include <cue.h>

namespace knxdmxd {

class Cuelist {
  public:
    Cuelist() {};
    Cuelist(const std::string name);

    void AddTrigger(knxdmxd::knx_patch_map_t& patchMap, const int KNX, const int val);    
    void AddTrigger(knxdmxd::knx_patch_map_t& patchMap, const std::string KNX, const int val);
    void AddTrigger(knxdmxd::knx_patch_map_t& patchMap, const knxdmxd::Trigger trigger);
    void AddCue(knxdmxd::Cue& cue);
            
    void Update(std::map<std::string, knxdmxd::Fixture>& fixtureList, const int KNX, const int val, const unsigned long long loopCounter);
    void Refresh(std::map<std::string, knxdmxd::Fixture>& fixtureList, const unsigned long long loopCounter);
    
  private:
    void NextCue(std::map<std::string, knxdmxd::Fixture>& fixtureList, const unsigned long long loopCounter);
    
    std::string _name;
    int _go_trigger_knx;
    int _go_trigger_val;
    int _halt_trigger_knx;
    int _halt_trigger_val;

    int _current_cue;
    unsigned long long _next_cue_start;
    std::deque<knxdmxd::Cue> _cue_data;
    std::map<std::string, int> _cue_names;

};


}

#endif
