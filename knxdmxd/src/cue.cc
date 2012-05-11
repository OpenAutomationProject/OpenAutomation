/*
 * cue.cc
 *
 * (c) by JNK 2012
 *
 *
*/


#include "cue.h"

namespace knxdmxd {

Cue::Cue(const std::string name) {
  _name=name;
  std::clog << "Creating Cue '" << name << "'" << std::endl;
}

void Cue::AddTrigger(knxdmxd::knx_patch_map_t& patchMap, int KNX, int val) {
  _triggerKNX = KNX;
  _trigger_val = val;
  
  std::clog << kLogDebug << "Cue '" << _name << "': Trigger by KNX:" << _triggerKNX << ", value " << _trigger_val << ((_trigger_val==-1) ? " (all)" : "") << std::endl;
  
  std::pair<knxdmxd::knx_patch_map_t::iterator, knxdmxd::knx_patch_map_t::iterator> alreadypatched; 
  alreadypatched = patchMap.equal_range(KNX);

  for (knxdmxd::knx_patch_map_t::iterator it = alreadypatched.first; it != alreadypatched.second; ++it) {
    if (it->second == _name) return; // already patched that one
  }
  
  patchMap.insert(knxdmxd::knx_patch_map_element_t(KNX,_name)); // no, add to patchMap
}

void Cue::AddTrigger(knxdmxd::knx_patch_map_t& patchMap, std::string KNX, int val) {
  AddTrigger(patchMap, readgaddr(KNX), val);
}

void Cue::AddChannel(cue_channel_t channel) {
  _channel_data.push_back(channel);
  std::clog << "Added Channel definition " << channel.fixture << "/" << channel.name << "@" << channel.value << std::endl;
}

void Cue::Update(std::map<std::string, knxdmxd::Fixture>& fixtureList, const int KNX, const int val) {
  if ((_trigger_val==-1) || (_trigger_val==val)) {
    for(std::list<cue_channel_t>::iterator it = _channel_data.begin(); it != _channel_data.end(); ++it) {
      fixtureList[it->fixture].Update(it->name, it->value, (float)255.);
    }
    std::clog << "Called cue " << _name << std::endl;
  }
}

}
