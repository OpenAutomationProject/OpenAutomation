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
#include <map>

namespace knxdmxd {

  class Universe {
    protected:
      unsigned char current_[512], new_[512], old_[512];
      unsigned long fadestart_[512], fadeend_[512];
      ola::DmxBuffer buffer_;
      ola::thread::Mutex data_mutex;
    public:
      Universe() {
        ola::thread::MutexLocker locker(&data_mutex);
        for (int i=0; i<512; i++) {
          current_[i] = new_[i] = old_[i] = 0;
        }
        buffer_.Blackout();
      };
      void Set(const unsigned channel, const unsigned char value, const float fade_in=1.e-4, const float fade_out=1.e-4) {
        ola::thread::MutexLocker locker(&data_mutex);
        new_[channel] = value;
        old_[channel] = current_[channel];
        float fadeTime = (new_[channel]>current_[channel]) ? fade_in : fade_out;
        fadestart_[channel] = Timer::Get();
        fadeend_[channel] = fadestart_[channel] + (unsigned long) (fadeTime*1000.0);
      }
      unsigned char Get(const unsigned channel) {
        return current_[channel];
      }
      ola::DmxBuffer &GetBuffer() {
        ola::thread::MutexLocker locker(&data_mutex);
        buffer_.Set(current_, 512);
        return buffer_;
      }
      void Crossfade();
  };

  typedef Universe* pUniverse;

  class DMX {
    protected:
      static std::map<char, pUniverse> output;
      static ola::OlaCallbackClientWrapper m_client;

    public:
      DMX() {};
      static void SetDMXChannel(const dmx_addr_t channel, const unsigned char value, const float fade_in=1.e-4, const float fade_out=1.e-4) {
        int dmxuniverse = (int) (channel / 512), dmxchannel = channel % 512;
        output[dmxuniverse]->Set(dmxchannel, value, fade_in, fade_out);
      };
      char GetDMXChannel(int channel);

      static dmx_addr_t Address(const std::string addr);
      static ola::OlaCallbackClientWrapper& GetOLAClient() { return m_client; };

  };

  class DMXSender: private DMX {
      bool sender_running_;

    public:
      DMXSender() {
        sender_running_ = false;
      }

      ~DMXSender();

      bool Init();
      int Start();
      void SendDMX();

      void Terminate();
      bool Running() {
        return sender_running_;
      };

      void AddUniverse(char universe) {
        if (!output.count(universe)) { // only create if not already existing;
          pUniverse new_universe = new Universe();
          output.insert(std::pair<char, pUniverse> (universe, new_universe));
          std::clog << "DMXSender created universe " << (int) universe << std::endl;
        }
      };

  };

}

#endif
