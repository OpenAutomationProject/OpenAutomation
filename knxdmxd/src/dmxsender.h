/*
 * dmxsender.h
 *
 * (c) by JNK 2012
 *
 *
 */

#ifndef DMXSENDER_H
#define DMXSENDER_H

#include <string.h>
#include <knxdmxd.h>
#include <ola/DmxBuffer.h>
#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>
#include <trigger.h>
#include <map>

namespace knxdmxd
{

  class Universe
  {
  protected:
    unsigned char current_[512], new_[512], old_[512];
    unsigned long fadestart_[512], fadeend_[512];
    ola::DmxBuffer buffer_;
    ola::thread::Mutex data_mutex;
    char universe_;
  public:
    Universe(char universe )
    {
      universe_ = universe; // universe should now it's own number
      ola::thread::MutexLocker locker(&data_mutex);
      for (int i = 0; i < 512; i++)
        {
          current_[i] = new_[i] = old_[i] = 0;
        }
      buffer_.Blackout();
    }
    ;
    void
    Set(const unsigned channel, const unsigned char value, const float fade_in =
        1.e-4, const float fade_out = 1.e-4)
    {
      ola::thread::MutexLocker locker(&data_mutex);
      const unsigned ch = channel - 1;
      new_[ch] = value;
      old_[ch] = current_[ch];
      float fadeTime = (new_[ch] > current_[ch]) ? fade_in : fade_out;
      fadestart_[ch] = Timer::Get();
      fadeend_[ch] = fadestart_[ch]
          + (unsigned long) (fadeTime * 1000.0);
    }
    unsigned char
    Get(const unsigned channel)
    {
      return current_[channel-1];
    }
    ola::DmxBuffer &
    GetBuffer()
    {
      ola::thread::MutexLocker locker(&data_mutex);
      buffer_.Set(current_, 512);
      return buffer_;
    }
    void
    Crossfade();
  };

  typedef Universe* pUniverse;

  class DMX
  {
  protected:
    static std::map<char, pUniverse> output;
    static ola::OlaCallbackClientWrapper m_client;

  public:
    DMX()
    {
    }
    ;
    static void
    SetDMXChannel(const dmx_addr_t channel, const unsigned char value,
        const float fade_in = 1.e-4, const float fade_out = 1.e-4)
    {
      int dmxuniverse = (int) (channel / 512), dmxchannel = channel % 512;
      output[dmxuniverse]->Set(dmxchannel, value, fade_in, fade_out);
    }
    ;
    char
    GetDMXChannel(int channel);

    static dmx_addr_t
    Address(const std::string &addr);
    static ola::OlaCallbackClientWrapper&
    GetOLAClient()
    {
      return m_client;
    }
    ;

  };

  class DMXSender : private DMX
  {
    bool sender_running_;

  public:
    DMXSender()
    {
      sender_running_ = false;
    }

    ~DMXSender();

    bool
    Init();
    int
    Start();
    void
    SendDMX();

    void
    Terminate();
    bool
    Running()
    {
      return sender_running_;
    }
    ;

    void
    AddUniverse(char universe)
    {
      if (!output.count(universe))
        { // only create if not already existing;
          pUniverse new_universe = new Universe(universe);
          output.insert(std::pair<char, pUniverse>(universe, new_universe));
          std::clog << kLogInfo << "DMXSender created universe " << (int) universe
              << std::endl;
        }
    }
    ;

  };

  class Dimmer : public TriggerHandler
    {
      std::string name_;
      dmx_addr_t dmx_;
      float fade_time_;
      eibaddr_t ga_, ga_fading_;
    public:
      Dimmer()
      {
      }
      ;
      Dimmer(const std::string name, eibaddr_t ga, dmx_addr_t dmx)
      {
        name_ = name;
        dmx_ = dmx;
        ga_ = ga;
        fade_time_ = 1.e-4;
        std::clog << kLogDebug << "Created dimmer '" << name << "' for " << dmx << std::endl;
      }
      void
      SetFadeTime(float fade_time)
      {
        fade_time_ = fade_time;
      }
      void
      SetFadeKNX(eibaddr_t ga)
      {
        ga_fading_ = ga;
      }
      void
      Process(const Trigger& trigger)
      {
        eibaddr_t ga = trigger.GetKNX();
        std::clog << kLogDebug << "Checking trigger " << trigger << " for " << name_
            << " (GA: " << ga_ << ")" << std::endl;
        if (ga == ga_)
          {
            DMX::SetDMXChannel(dmx_, trigger.GetValue(), fade_time_, fade_time_);
            std::clog << kLogDebug << "Dimmer/Process: Updating value for " << name_ << " to "
                << trigger.GetValue() << std::endl;
          }
        if (ga == ga_fading_)
          {
            std::clog << kLogDebug << "Dimmer/Process: Updating (tba) fading for " << name_
                << " to " << trigger.GetValue() << std::endl;
          }
      }
    };

}

#endif
