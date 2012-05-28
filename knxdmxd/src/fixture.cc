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
    name_ = name;
    std::clog << "Creating Fixture '" << name_ << "'" << std::endl;
  }

  void Fixture::AddChannel(const std::string& name, const std::string& DMX, const std::string& KNX) {
    fixture_channel_t channel;
    channel.KNX = (KNX!="") ? readgaddr(KNX) : -1;
    channel.DMX = readdaddr(DMX);
    channel.value = channel.floatValue = 0; // we start blacked out
    channel.fadeStep = 255; // direct by default
    channel_data_.push_back(channel);
    channel_names_.insert(std::pair<std::string, int> (name, channel_data_.size()-1));
  }

  void Fixture::SetFadeTime(const float t) {
    _fadeTime = t;
    _fadeStep = (t<=0) ? 256 :  256/(t*1e6/FADING_INTERVAL);
    std::clog << "Fixture '" << name_ << "': Set global fadetime to " << _fadeTime << "s (" << _fadeStep << " steps/interval)" << std::endl;
  }

  void Fixture::Update(const std::string& channel, const int val, float fadeStep) {
    unsigned ch = channel_names_[channel];
    channel_data_[ch].value=val;
    channel_data_[ch].fadeStep=fadeStep;
    std::clog << "Fixture '" << name_ << "': Channel '" << channel << "' @ " << val << ", fading: " << fadeStep << " old: "<< channel_data_[ch].floatValue << std::endl;
  }

  void Fixture::Refresh() {
    for(std::vector<knxdmxd::fixture_channel_t>::iterator it=channel_data_.begin(); it!=channel_data_.end(); ++it) {
      int oldValue = GetDMXChannel(it->DMX);
      int newValue = it->value;
      if (oldValue<newValue) {
        it->floatValue += it->fadeStep;
        if (it->floatValue>newValue) {
          it->floatValue = newValue;
        }
        SetDMXChannel(it->DMX, (int) it->floatValue);
      }
      if (oldValue>newValue) {
        it->floatValue -= it->fadeStep;
        if (it->floatValue<newValue) {
          it->floatValue = newValue;
        }
        SetDMXChannel(it->DMX, (int) it->floatValue);
      }
    }    
  }

  int Fixture::GetCurrentValue(const std::string& channel) {
    return (int) channel_data_[channel_names_[channel]].floatValue;
  }
  
  void Fixture::Process(const Trigger& trigger) {
    for (std::vector<knxdmxd::fixture_channel_t>::iterator it=channel_data_.begin(); it!=channel_data_.end(); ++it) {
      if (it->KNX == trigger.GetKNX()) {
        it->value = trigger.GetValue();
        it->fadeStep = _fadeStep; 
        std::cout << "Set " << it->value << std::endl;
      }
    }
    if (fadeTimeKNX_ == trigger.GetKNX()) {
      SetFadeTime(trigger.GetValue());
    }
  }
   
  void DMX::SetDMXChannel(int channel, int value) {
    int dmxuniverse = (int) (channel / 512), dmxchannel = channel % 512;
    output[dmxuniverse].SetChannel(dmxchannel, value);
  } 
  
  int DMX::GetDMXChannel(int channel) {
    int dmxuniverse = (int) (channel / 512), dmxchannel = channel % 512;
    return output[dmxuniverse].Get(dmxchannel);
  }
  
  void FixtureList::Process(const Trigger& trigger) {
    for(std::map<std::string, knxdmxd::pFixture>::iterator it=fixture_list_.begin(); it!=fixture_list_.end(); ++it) {
      it->second->Process(trigger);
    }
  }
  
  void FixtureList::Refresh() {
    for(std::map<std::string, knxdmxd::pFixture>::iterator it=fixture_list_.begin(); it!=fixture_list_.end(); ++it) {
      it->second->Refresh();
    }
  }
}
