/*
 * trigger.cc
 *
 * (c) by JNK 2012
 *
 *
*/

#include "trigger.h"

namespace knxdmxd {

  Trigger::Trigger(const int type, int knx, const int val) {
   _type= type;
   _knx = knx;
   _val = val;
  }
  
  bool Trigger::operator==(const Trigger &other) const {
    return ((_knx == other._knx)  // same knx
      && ((_type == other._type) || (other._type == kTriggerAll) || (_type==kTriggerAll))  // same trigger type
      && ((_val == other._val) || (other._val ==-1) || (_val==-1))); // same value
  }

  Trigger& Trigger::operator= (Trigger const& other) { 
    if (this != &other) { 
      _knx = other._knx;
      _type = other._type;
      _val = other._val;
    } 
    return *this; 
  }

  void TriggerList::Add(Trigger& trigger, pTriggerHandler handler) {
    _triggers.push_back(trigger);
    _handlers.push_back(handler);
    std::clog << "Added Trigger " << trigger << " for handler " << (*handler) << std::endl;
  }

  void TriggerList::Process(const Trigger& trigger) {
    for (unsigned i=0; i<_triggers.size(); i++) {
      knxdmxd::Trigger tr = _triggers[i];
      if (tr==trigger) {
        switch (tr.GetType()) {
          case kTriggerGo:
            _handlers[i]->Go();
            break;
          case kTriggerHalt:
            _handlers[i]->Halt();
            break;
          case kTriggerDirect:
            _handlers[i]->Direct(trigger.GetValue());
            break;
          default: ;
        }
      }
    }
  
}

std::ostream& operator<<(std::ostream& stream, const knxdmxd::Trigger& trigger) 
{  
  stream << trigger._type << "(" << trigger._val << "@" << trigger._knx <<")";
  return stream;
}
 
std::ostream& operator<<(std::ostream& stream, const knxdmxd::TriggerHandler& handler) 
{  
  stream << handler._name;
  return stream;
}
  
}
