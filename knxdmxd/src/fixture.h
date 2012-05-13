/*
 * fixture.h
 *
 * (c) by JNK 2012
 *
 *
*/

#ifndef FIXTURE_H
#define FIXTURE_H

#include <string.h>
#include <knxdmxd.h>

namespace knxdmxd {

class Fixture {
  public:
    Fixture() {};
    Fixture(const std::string name);
    
    void Patch(knxdmxd::knx_patch_map_t& patchMap, const std::string channel, const int DMX, const int KNX);
    void Patch(knxdmxd::knx_patch_map_t& patchMap, const std::string channel, const std::string DMX, const std::string KNX);
    void SetFadeTime(const float t);
    void PatchFadeTime(const int KNX);
    void Update(const std::string& channel, const int val, const bool direct=false);
    void Update(const std::string& channel, const int val, const float fadeStep);
    void Update(const int KNX, const int val, const bool direct=false);
    void Refresh(std::map<int, ola::DmxBuffer>& output);
    int GetCurrentValue(const std::string& channel);
  private:
    std::string _name;
    std::map <std::string, int> _channelKNX;
    std::map <std::string, int> _channelDMX;
    std::map <std::string, int> _channelValue; // set value
    std::map <std::string, float> _channelFadeStep; // individual by channel
    std::map <std::string, float> _channelFloatValue; // internal calculation

    float _fadeTime; // is set by knx or config
    float _fadeStep; // calculated from _fadeTime
    int _fadeTimeKNX;
};

}

#endif
