/*
 * cue.cc
 *
 * (c) by JNK 2012
 *
 *
 */

#include "cue.h"

namespace knxdmxd
{

  Cue::Cue(const std::string name, const bool isLink)
  {
    _name = name;
    fadeIn_ = 0.0;
    fadeOut_ = 0.0;
    waittime_ = -1; // never step automatically
    delay_ = 0.0; // call immediately
    delay_on_ = false;
    is_link_ = isLink;
    if (!is_link_)
      {
        std::clog << kLogDebug << "Creating Cue " << name << std::endl;
      }
    else
      {
        std::clog << kLogDebug << "Creating link to cue " << name << std::endl;
      }
  }

  void
  Cue::AddChannel(const cue_channel_t& channel)
  {
    _channel_data.push_back(channel);
    std::clog << kLogDebug << "Cue " << _name << ": added channel definition "
        << channel.dmx << "@" << channel.value << std::endl;
  }

  void
  Cue::SetFading(const float fadeIn, const float fadeOut)
  {
    fadeIn_ = (fadeIn < 1.e-3) ? (DMX_INTERVAL / 1.e3) : fadeIn;
    fadeOut_ = (fadeOut < 1.e-3) ? (DMX_INTERVAL / 1.e3) : fadeOut;

    std::clog << kLogDebug << "Cue " << _name << ": set fading " << fadeIn_
        << "/" << fadeOut_ << std::endl;
  }

  void
  Cue::Go()
  {
    if ((delay_ > 0) && (!delay_on_))
      {
        DMX::GetOLAClient().GetSelectServer()->RegisterSingleTimeout(
            (int) delay_ * 1000, ola::NewSingleCallback(this, &Cue::Go));
        delay_on_ = true;
        std::clog << kLogDebug << "Called cue " << _name << " (delaying)"
            << std::endl;
      }
    else
      {
        for (std::list<cue_channel_t>::iterator it = _channel_data.begin();
            it != _channel_data.end(); ++it)
          {
            DMX::SetDMXChannel(it->dmx, it->value, fadeIn_, fadeOut_);
          }
        delay_on_ = false;
        std::clog << kLogDebug << "Called cue " << _name << std::endl;
      }

  }

  Cuelist::Cuelist(const std::string name)
  {
    _name = name;
    current_cue_ = -1;

    was_direct_ = false;
    cue_halted_ = true;
    release_on_halt_ = true;

    max_cue_ = 0;

    std::clog << kLogDebug << "Creating Cuelist '" << name << "'" << std::endl;
  }

  void
  Cuelist::AddCue(knxdmxd::Cue& cue)
  {
    cue_data_.push_back(cue);
    int cue_num = max_cue_;
    if (!cue.isLink())
      {
        cue_names_.insert(std::pair<std::string, int>(cue.GetName(), cue_num));
        std::clog << kLogDebug << "Cuelist " << _name << ": added cue "
            << cue.GetName() << " as #" << cue_num << std::endl;
      }
    else
      {
        std::clog << kLogDebug << "Cuelist " << _name << ": added link to cue '"
            << cue.GetName() << "' as #" << cue_num << std::endl;
      }
    max_cue_++;
  }

  void
  Cuelist::NextCue(const int direct)
  {
    if (was_direct_)
      {
        was_direct_ = false;
        return;
      }

    switch (direct)
      {
    default: // direct call
      if (direct >= max_cue_)
        { // called cue too large
          std::clog << kLogInfo << "Tried direct call to cue " << direct
              << " in cuelist " << _name << ": too large" << std::endl;
          return;
        }
      current_cue_ = direct - 1;
      if (!cue_halted_) // only if cuelist was running
        was_direct_ = true;
      break;
    case -1: // normal call
      break;
    case -2: // next on running cuelist
      was_direct_ = true;
      break;
      }

    if (max_cue_ > (current_cue_ + 1))
      {
        try
          {
            if (current_cue_ >= 0)
              cue_data_.at(current_cue_).Release();

            current_cue_++;
            knxdmxd::Cue cue = cue_data_.at(current_cue_);
            if (cue.isLink())
              {
                current_cue_ = cue_names_.find(cue.GetName())->second;
              }

            cue_data_.at(current_cue_).Go();

            float waittime;
            int nextCuenum = current_cue_ + 1;
            if (max_cue_ > nextCuenum)
              {
                knxdmxd::Cue nextCue = cue_data_.at(nextCuenum);
                if (nextCue.isLink())
                  {
                    nextCuenum = cue_names_.find(nextCue.GetName())->second;
                    nextCue = cue_data_.at(nextCuenum);
                  }
                waittime = nextCue.GetWaitTime();
                if ((waittime >= 0) && (!cue_halted_))
                  {
                    DMX::GetOLAClient().GetSelectServer()->RegisterSingleTimeout(
                        (int) (waittime * 1000.),
                        ola::NewSingleCallback(this, &Cuelist::NextCue, -1));
                  }
              } else {
                  // last cue halts automatically
                  Halt();
              }
          }
        catch (char *str)
          {
            std::clog << kLogErr << "Exception " << str
                << " in calling cue, current_cue_ = " << current_cue_
                << std::endl;
          }
      }
  }

}
