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
#include <trigger.h>
#include <ola/OlaClientWrapper.h>
#include <ola/thread/Mutex.h>
#include <dmxsender.h>


namespace knxdmxd {

  class Dimmer : public TriggerHandler {
      std::string name_;
      dmx_addr_t dmx_;
      float fade_time_;
      eibaddr_t ga_, ga_fading_;
    public:
      Dimmer() {};
      Dimmer(const std::string name, eibaddr_t ga, dmx_addr_t dmx) {
        name_ = name;
        dmx_ = dmx;
        ga_ = ga;
        fade_time_ = 1.e-4;
        std::clog << "Created dimmer '" << name << "' for "
          << dmx << std::endl;
      }
      void SetFadeTime(float fade_time) {
        fade_time_ = fade_time;
      }
      void SetFadeKNX(eibaddr_t ga) {
        ga_fading_ = ga;
      }
      void Process(const Trigger& trigger) {
        eibaddr_t ga = trigger.GetKNX();
        std::clog << "Checking trigger" << trigger << " / " << ga_ << std::endl;
        if (ga == ga_) {
          DMX::SetDMXChannel(dmx_, trigger.GetValue(), fade_time_, fade_time_ );
          std::clog << "Dimmer/Process: Updating value" << std::endl;
        }
        if (ga == ga_fading_) {
          std::clog << "Dimmer/Process: Updating fading" << std::endl;
        }
      }
  };

  

}

#endif
