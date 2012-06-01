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
//#include <ola/OlaCallbackClient.h>
#include <ola/OlaClientWrapper.h>
//#include <dmxsender.h>


namespace knxdmxd {

  class DMX {
    protected:
      static std::map<int, ola::DmxBuffer> output;
      static ola::OlaCallbackClientWrapper m_client;
      
    public:
      DMX() {};
      void SetDMXChannel(int channel, int value);
      int GetDMXChannel(int channel);
      
      static int Address(const std::string s);
      
  };
  
  typedef struct {
    int KNX, DMX, value;
    float fadeStep, floatValue;
  } fixture_channel_t;
  
  class Fixture : private DMX {
      std::string name_;
      std::vector<fixture_channel_t> channel_data_;
      std::map<std::string, unsigned> channel_names_;
      
      float _fadeTime; // is set by knx or config
      float _fadeStep; // calculated from _fadeTime
      int fadeTimeKNX_;

    public:
      Fixture() {};
      Fixture(const std::string name);

      void AddChannel(const std::string& name, const std::string& DMX, const std::string& KNX);
      void SetFadeTime(const float t);
      void PatchFadeTime(const int KNX) { fadeTimeKNX_ = KNX; };
      void Update(const std::string& channel, const int val, const float fadeStep);
      void Process(const Trigger& trigger);
      void Refresh();
      bool RegisterRefresh();
      
      int GetCurrentValue(const std::string& channel);
      std::string& GetName() { return name_; };

      void Lock(const std::string& cuelist, int lockpriority);
      int isLocked();
      void Release();
    
  };

  typedef Fixture* pFixture;
  
  class FixtureList {
      std::map<std::string, knxdmxd::pFixture> fixture_list_;
      
    public:
      FixtureList() {};
      
      void Add(pFixture fixture) {  fixture_list_.insert(std::pair<std::string, knxdmxd::pFixture> (fixture->GetName(), fixture)); };
      void Process(const Trigger& trigger);
      
      pFixture Get(const std::string& name) { return fixture_list_[name]; };
  };

}

#endif
