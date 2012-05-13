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
  _fadeIn = 0.0;
  _fadeOut = 0.0;
  std::clog << "Creating Cue '" << name << "'" << std::endl;
}

void Cue::AddTrigger(knxdmxd::knx_patch_map_t& patchMap, int KNX, int val) {
  _triggerKNX = KNX;
  _trigger_val = val;
  
  std::clog << kLogDebug << "Cue '" << _name << "': added trigger by KNX:" << _triggerKNX << ", value " << _trigger_val << ((_trigger_val==-1) ? " (all)" : "") << std::endl;
  
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

void Cue::AddChannel(const cue_channel_t& channel) {
  _channel_data.push_back(channel);
  std::clog << "Cue '" << _name << "': added channel definition " << channel.fixture << "/" << channel.name << "@" << channel.value << std::endl;
}

void Cue::SetFading(const float fadeIn, const float fadeOut) {
  _fadeIn = fadeIn;
  _fadeOut = (fadeOut < 0) ? fadeIn : fadeOut;
  std::clog << kLogDebug << "Cue '" << _name << "': set fading " << _fadeIn << "/" << _fadeOut << std::endl;
}

void Cue::Update(std::map<std::string, knxdmxd::Fixture>& fixtureList) {
  for(std::list<cue_channel_t>::iterator it = _channel_data.begin(); it != _channel_data.end(); ++it) {
    knxdmxd::Fixture& f = fixtureList[it->fixture];
    int currentValue = f.GetCurrentValue(it->name);
    int deltaVal = currentValue - it->value;
    float ft = (deltaVal>0) ? (deltaVal/(_fadeOut*1.e6/FADING_INTERVAL)) : (-deltaVal/(_fadeIn*1.e6/FADING_INTERVAL));
    f.Update(it->name, it->value, (float)ft);
  }
  std::clog << "Called cue " << _name << std::endl;
}

void Cue::Update(std::map<std::string, knxdmxd::Fixture>& fixtureList, const int KNX, const int val) {
  if ((_trigger_val==-1) || (_trigger_val==val)) {
    Update(fixtureList);
  }
}

std::string Cue::GetName() {
  return _name;
}

float Cue::GetWaitTime() {
  return _waittime;
}

void Cue::SetWaittime(const float waittime) {
  _waittime = waittime;
  std::clog << kLogDebug << "Cue '" << _name << "': set waittime " << _waittime << std::endl;
}

}
