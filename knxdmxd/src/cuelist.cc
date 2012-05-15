/*
 * cuelist.cc
 *
 * (c) by JNK 2012
 *
 *
*/


#include "cuelist.h"

namespace knxdmxd {

Cuelist::Cuelist(const std::string name) {
  _name=name;
  _current_cue=-1;
  _next_cue_start=-1;
  std::clog << "Creating Cuelist '" << name << "'" << std::endl;
}

void Cuelist::AddTrigger(knxdmxd::knx_patch_map_t& patchMap, int KNX, int val) {
  _go_trigger_knx = KNX;
  _go_trigger_val = val;
  
  std::clog << kLogDebug << "Cuelist '" << _name << "': added trigger by KNX:" << _go_trigger_knx << ", value " << _go_trigger_val << ((_go_trigger_val==-1) ? " (all)" : "") << std::endl;
  
  std::pair<knxdmxd::knx_patch_map_t::iterator, knxdmxd::knx_patch_map_t::iterator> alreadypatched; 
  alreadypatched = patchMap.equal_range(KNX);

  for (knxdmxd::knx_patch_map_t::iterator it = alreadypatched.first; it != alreadypatched.second; ++it) {
    if (it->second == _name) return; // already patched that one
  }
  
  patchMap.insert(knxdmxd::knx_patch_map_element_t(KNX,_name)); // no, add to patchMap
}

void Cuelist::AddTrigger(knxdmxd::knx_patch_map_t& patchMap, std::string KNX, int val) {
  AddTrigger(patchMap, readgaddr(KNX), val);
}

void Cuelist::AddTrigger(knxdmxd::knx_patch_map_t& patchMap, const knxdmxd::Trigger trigger) {

  if ((trigger.type) == knxdmxd::kTriggerGo) {
    _go_trigger_knx = trigger.knx;
    _go_trigger_val = trigger.value;
  }

  if ((trigger.type) == knxdmxd::kTriggerHalt) {
    _halt_trigger_knx = trigger.knx;
    _halt_trigger_val = trigger.value;
  }
  
  std::clog << kLogDebug << "Cuelist '" << _name << "': added " << trigger.type << " trigger by KNX:" << trigger.knx << ", value " << trigger.value << ((trigger.value==-1) ? " (all)" : "") << std::endl;
  
  std::pair<knxdmxd::knx_patch_map_t::iterator, knxdmxd::knx_patch_map_t::iterator> alreadypatched; 
  alreadypatched = patchMap.equal_range(trigger.knx);

  for (knxdmxd::knx_patch_map_t::iterator it = alreadypatched.first; it != alreadypatched.second; ++it) {
    if (it->second == _name) return; // already patched that one
  }
  
  patchMap.insert(knxdmxd::knx_patch_map_element_t(trigger.knx,_name)); // no, add to patchMap
}

void Cuelist::AddCue(knxdmxd::Cue& cue) {
  _cue_data.push_back(cue);
  int cue_num = _cue_data.size()-1;
  if (!cue.isLink()) {
    _cue_names.insert(std::pair<std::string, int>(cue.GetName(), cue_num));
    std::clog << "Cuelist '" << _name << "': added cue " << cue.GetName() << " as #" << _cue_data.size()-1 << std::endl;
  } else {
    std::clog << "Cuelist '" << _name << "': added link to cue '" << cue.GetName() << "' as #" << _cue_data.size()-1 << std::endl;
  }
}

void Cuelist::NextCue(std::map<std::string, knxdmxd::Fixture>& fixtureList, const unsigned long long loopCounter) {
  if (_cue_data.size()>(_current_cue+1)) {
    _current_cue++;
    knxdmxd::Cue cue = _cue_data.at(_current_cue);
    if (cue.isLink()) {
      _current_cue = _cue_names.find(cue.GetName())->second;
    }
        
    _cue_data.at(_current_cue).Update(fixtureList);
    
    float waittime;
    int nextCuenum = _current_cue+1;
    if (_cue_data.size()>nextCuenum) { // last cue stops automatically
      knxdmxd::Cue nextCue = _cue_data.at(nextCuenum);
      if (nextCue.isLink()) {
        nextCuenum = _cue_names.find(nextCue.GetName())->second;
        nextCue = _cue_data.at(nextCuenum);
      }
      waittime = nextCue.GetWaitTime();
    } else {
      waittime = -1;
    }
    if (waittime>=0) { // if waittime < 0 : manual trigger
      _next_cue_start = loopCounter + (int) (waittime*1.e6/FADING_INTERVAL);
    } else {
      _next_cue_start = -1;
    }
  } else {
    _current_cue = -1;
    _next_cue_start = -1;
  }
}

void Cuelist::Update(std::map<std::string, knxdmxd::Fixture>& fixtureList, const int KNX, const int val, const unsigned long long loopCounter) {
  if ((KNX==_go_trigger_knx) && ((_go_trigger_val==-1) || (_go_trigger_val==val))) {
    NextCue(fixtureList, loopCounter);
  }
  if ((KNX==_halt_trigger_knx) && ((_halt_trigger_val==-1) || (_halt_trigger_val==val))) {
    _next_cue_start = -1;
  }
 
}

void Cuelist::Refresh(std::map<std::string, knxdmxd::Fixture>& fixtureList, const unsigned long long loopCounter) {
  if ((loopCounter>_next_cue_start) && (_next_cue_start>0)) { // if next_cue_start < 0 : manual GO required
    NextCue(fixtureList, loopCounter);
  }
}

}
