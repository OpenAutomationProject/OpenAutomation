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

namespace knxdmxd
{

  const int kTriggerAll = 0;

  const int kTriggerGo = 1;
  const int kTriggerHalt = 2;
  const int kTriggerDirect = 4;
  const int kTriggerRelease = 8;

  const int kTriggerProcess = 256;

  class Trigger
  {
    eibaddr_t _knx;
    unsigned _val;
    int _type;

  public:
    Trigger(const int type, const eibaddr_t knx, const unsigned val);

    bool
    operator ==(const Trigger &other) const;
    Trigger&
    operator =(const Trigger &other);
    const eibaddr_t
    GetKNX() const
    {
      return _knx;
    }

    const unsigned
    GetValue() const
    {
      return _val;
    }

    const int
    GetType() const
    {
      return _type;
    }


    friend std::ostream&
    operator<<(std::ostream &stream, const Trigger& t);
  };

  class TriggerHandler
  {
  protected:
    std::string _name;
  public:
    virtual void
    Go()
    {
    }

    virtual void
    Halt()
    {
    }

    virtual void
    Refresh()
    {
    }

    virtual void
    Direct(const int val)
    {
    }

    virtual void
    Release()
    {
    }

    virtual void
    Process(const Trigger& trigger)
    {
    }

    virtual
    ~TriggerHandler()
    {
    }

    const std::string GetName() {
      return _name;
    }

    friend std::ostream&
    operator<<(std::ostream &stream, const TriggerHandler& handler);
  };

  typedef TriggerHandler* pTriggerHandler;

  class TriggerList
  {
    std::vector<Trigger> _triggers;
    std::vector<pTriggerHandler> _handlers;
  public:
    TriggerList()
    {
    }

    ~TriggerList()
    {

    }
    void
    Add(Trigger& trigger, pTriggerHandler handler);
    void
    Process();
    void
    Clean()
    {
      std::set<pTriggerHandler> s( _handlers.begin(), _handlers.end() );

      std::set<pTriggerHandler>::iterator itr;
      for (itr = s.begin(); itr != s.end(); ++itr)
        {
          delete (*itr);
        }
    }

  };

}

#endif
