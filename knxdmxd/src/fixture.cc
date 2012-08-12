/*
 * fixture.cc
 *
 * (c) by JNK 2012
 *
 *
*/

#include "fixture.h"

namespace knxdmxd {

 /* Fixture::Fixture(const std::string name) {
    name_ = name;
    fadeTimeKNX_ = 0;
    std::clog << "Creating Fixture " << name_ << std::endl;
  }

  void Fixture::AddChannel(const std::string& name, const std::string& DMX, const std::string& KNX) {
    fixture_channel_t channel;
    channel.KNX = (KNX!="") ? KNXHandler::Address(KNX) : -1;
    channel.DMX = Address(DMX);
    channel.value = channel.floatValue = 0; // we start blacked out
    channel.fadeStep = 255; // direct by default
    channel_data_.push_back(channel);
    channel_names_.insert(std::pair<std::string, int> (name, channel_data_.size()-1));
  }

  void Fixture::SetFadeTime(const float t) {
    _fadeTime = t;
    _fadeStep = (t<=0) ? 256 :  256/(t*1e3/DMX_INTERVAL);
  }

  void Fixture::Update(const std::string& channel, const int val, float fadeStep) {
    unsigned ch = channel_names_[channel];
    channel_data_[ch].value=val;
    channel_data_[ch].fadeStep=fadeStep;
    refresh_ = true;
  }

  void Fixture::Refresh() {
    if (refresh_) {
      bool refresh = false;
      for(std::vector<knxdmxd::fixture_channel_t>::iterator it=channel_data_.begin(); it!=channel_data_.end(); ++it) {
        int oldValue = GetDMXChannel(it->DMX);
        int newValue = it->value;
        if (oldValue<newValue) {
          it->floatValue += it->fadeStep;
          if (it->floatValue>newValue) {
            it->floatValue = newValue;
          } else {
            refresh = true;
          }
          SetDMXChannel(it->DMX, (int) it->floatValue);
        }
        if (oldValue>newValue) {
          it->floatValue -= it->fadeStep;
          if (it->floatValue<newValue) {
            it->floatValue = newValue;
          } else {
            refresh = true;
          }
          SetDMXChannel(it->DMX, (int) it->floatValue);
        }
      }
      refresh_ = refresh;
    }
    RegisterRefresh();
  }

  bool Fixture::RegisterRefresh() {
    m_client.GetSelectServer()->RegisterSingleTimeout(
      DMX_INTERVAL, ola::NewSingleCallback(this, &Fixture::Refresh));
    return true;
  }

  int Fixture::GetCurrentValue(const std::string& channel) {
    return (int) channel_data_[channel_names_[channel]].floatValue;
  }

  void Fixture::Process(const Trigger& trigger) {
    std::clog << "fprocess" << std::endl;
    for (std::vector<knxdmxd::fixture_channel_t>::iterator it=channel_data_.begin(); it!=channel_data_.end(); ++it) {
      if (it->KNX == trigger.GetKNX()) {
        it->value = trigger.GetValue();
        it->fadeStep = _fadeStep;
        refresh_ = true;
        std::clog << "Updated " << it->DMX << " to " << it->value << "rate: " << it->fadeStep << std::endl;
      }
    }
    if (fadeTimeKNX_ == trigger.GetKNX()) {
      SetFadeTime(trigger.GetValue());
    }
  } */

    
}
