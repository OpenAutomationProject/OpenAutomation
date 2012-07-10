/*
 * trigger.h
 *
 * (c) by JNK 2012
 *
 *
*/

#ifndef TRIGGER_H
#define TRIGGER_H

#include <knxdmxd.h>
#include <vector>

namespace knxdmxd {

  const int kTriggerAll = 0;
  
  const int kTriggerGo = 1;
  const int kTriggerHalt = 2;
  const int kTriggerDirect = 4;
  const int kTriggerRelease = 8;
  
  const int kTriggerProcess = 256;

  class Trigger {
      int _knx;
      int _val;
      int _type;

    public:
      Trigger(const int type, const int knx, const int val);
  
      bool operator == (const Trigger &other) const;
      Trigger& operator = (const Trigger &other);
      const int GetKNX() const { return _knx; };
      const int GetValue() const { return _val; };
      const int GetType() const { return _type; };
    
      friend std::ostream& operator<<(std::ostream &stream, const Trigger& t);
  };

  class TriggerHandler {
    protected:
       std::string _name;
    public:
      virtual void Go() {};
      virtual void Halt() {};
      virtual void Refresh() {};
      virtual void Direct(const int val) {};
      virtual void Release() {};
      virtual void Process(const Trigger& trigger) {};
      
      friend std::ostream& operator<<(std::ostream &stream, const TriggerHandler& handler);
  };
  
  typedef TriggerHandler* pTriggerHandler;

  class TriggerList {
      std::vector<Trigger> _triggers;
      std::vector<pTriggerHandler> _handlers;
    public:
      TriggerList() { };
      
      void Add(Trigger& trigger, pTriggerHandler handler);
      void Process(const Trigger& trigger);
  };

}

#endif
