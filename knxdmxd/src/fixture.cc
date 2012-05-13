/*
 * fixture.cc
 *
 * (c) by JNK 2012
 *
 *
*/


#include "fixture.h"

namespace knxdmxd {

Fixture::Fixture(const std::string name) {
  _name=name;
  std::clog << "Creating Fixture '" << _name << "'" << std::endl;
}

void Fixture::Patch(knx_patch_map_t& patchMap, const std::string channel, const int DMX, const int KNX=-1) {
  _channelKNX[channel] = KNX;
  _channelDMX[channel] = DMX;
  
  std::clog << "Fixture '" << _name << "': Patched channel '" << channel << "' (KNX " << KNX << " to " << DMX << ") " << std::endl;
  
  std::pair<knxdmxd::knx_patch_map_t::iterator, knxdmxd::knx_patch_map_t::iterator> alreadypatched;  // fixtures that handle this
  alreadypatched = patchMap.equal_range(KNX);

  for (knxdmxd::knx_patch_map_t::iterator it = alreadypatched.first; it != alreadypatched.second; ++it) {
    if (it->second == _name) return; // already patched that one
  }
  
  patchMap.insert(knxdmxd::knx_patch_map_element_t(KNX,_name)); // no, add to patchMap

}

void Fixture::Patch(knx_patch_map_t& patchMap, const std::string channel, const std::string DMX, const std::string KNX) {
   Patch(patchMap, channel, readdaddr(DMX), readgaddr(KNX));
}

void Fixture::SetFadeTime(const float t) {
  _fadeTime = t;
  _fadeStep = (t<=0) ? 256 :  256/(t*1e6/FADING_INTERVAL);
  std::clog << "Fixture '" << _name << "': Set global fadetime to " << _fadeTime << "s (" << _fadeStep << " steps/interval)" << std::endl;
}

void Fixture::PatchFadeTime(const int KNX) {
  _fadeTimeKNX = KNX;
}

void Fixture::Update(const std::string& channel, const int val, bool direct) {
  _channelValue[channel] = val;
  _channelFadeStep[channel] = direct ? 256 : _fadeStep; 
  std::clog << "Fixture '" << _name << "': Channel '" << channel << "' @ " << val << ", direct: " << ((direct) ? "true" : "false") << std::endl;
}

void Fixture::Update(const int KNX, const int val, bool direct) {
  for(std::map<std::string, int>::const_iterator i = _channelKNX.begin(); i != _channelKNX.end(); ++i) {
    if (i->second == KNX) Update(i->first, val, direct);
  }
}

void Fixture::Update(const std::string& channel, const int val, float fadeStep) {
  _channelValue[channel] = val;
  _channelFadeStep[channel] = fadeStep; 
  std::clog << "Fixture '" << _name << "': Channel '" << channel << "' @ " << _channelValue[channel] << ", fading: " << fadeStep << " old: "<< _channelFloatValue[channel] << std::endl;
}


void Fixture::Refresh(std::map<int, ola::DmxBuffer>& output) {
  for(std::map<std::string, int>::const_iterator i = _channelDMX.begin(); i != _channelDMX.end(); ++i) {  
    int dmxuniverse = (int) (i->second / 512), dmxchannel = i->second % 512;
    int oldValue = output[dmxuniverse].Get(dmxchannel);
    int newValue = _channelValue[i->first];
    if (oldValue<newValue) {
      _channelFloatValue[i->first] += _channelFadeStep[i->first];
      if (_channelFloatValue[i->first]>newValue) {
        _channelFloatValue[i->first] = newValue;
      }
      output[dmxuniverse].SetChannel(dmxchannel, (int) _channelFloatValue[i->first]);
      //std::clog << "Fade: " << dmxuniverse << "." << dmxchannel << " @ " << _channelFloatValue[i->first] << std::endl;
    }
    if (oldValue>newValue) {
      _channelFloatValue[i->first] -= _channelFadeStep[i->first];
      if (_channelFloatValue[i->first]<newValue) {
        _channelFloatValue[i->first] = newValue;
      }
      output[dmxuniverse].SetChannel(dmxchannel, (int) _channelFloatValue[i->first]);
      //std::clog << "Fade: " << dmxuniverse << "." << dmxchannel << " @ " << _channelFloatValue[i->first] << std::endl;
    }
  }
}

int Fixture::GetCurrentValue(const std::string& channel) {
  return (int) _channelFloatValue.find(channel)->second;
}
}
