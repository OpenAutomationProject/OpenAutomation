/*
 * cue.cc
 *
 * (c) by JNK 2012
 *
 *
*/


#include "cue.h"

namespace knxdmxd {

  Cue::Cue(const std::string name, const bool isLink) {
    _name=name;
    fadeIn_ = 0.0;
    fadeOut_ = 0.0;
    waittime_ = -1; // never step automatically
    delay_ = 0.0; // call immediately
    is_link_ = isLink;
    if (!is_link_) {
      std::clog << "Creating Cue " << name << std::endl;
    } else {
      std::clog << "Creating link to cue " << name << std::endl;
    }
  }

  void Cue::AddChannel(const cue_channel_t& channel) {
    _channel_data.push_back(channel);
    std::clog << "Cue " << _name << ": added channel definition " << channel.fixture->GetName() << "/" << channel.name << "@" << channel.value << std::endl;
  }

  void Cue::SetFading(const float fadeIn, const float fadeOut) {
    fadeIn_ = fadeIn;
    fadeOut_ = (fadeOut < 0) ? fadeIn : fadeOut;
    
    if (fadeIn_<1.e-3) {
      fadeIn_ = (FADING_INTERVAL/1.e6);
    }  

    if (fadeOut_<1.e-3) {
      fadeOut_ = (FADING_INTERVAL/1.e6);
    }
  
    std::clog << kLogDebug << "Cue " << _name << ": set fading " << fadeIn_ << "/" << fadeOut_ << std::endl;
  }

  void Cue::Go() {
    for(std::list<cue_channel_t>::iterator it = _channel_data.begin(); it != _channel_data.end(); ++it) {
      int currentValue = it->fixture->GetCurrentValue(it->name);
      int deltaVal = currentValue - it->value;
      float ft = (deltaVal>0) ? (deltaVal/(fadeOut_*1.e6/FADING_INTERVAL)) : (-deltaVal/(fadeIn_*1.e6/FADING_INTERVAL));
      it->fixture->Update(it->name, it->value, (float)ft);
    }
    std::clog << "Called cue " << _name << std::endl;
  }

  Cuelist::Cuelist(const std::string name) {
    _name=name;
    current_cue_=-1;
    was_direct_ = false;
    cue_halted_=true;
    std::clog << "Creating Cuelist '" << name << "'" << std::endl;
  }  

  void Cuelist::AddCue(knxdmxd::Cue& cue) {
    _cue_data.push_back(cue);
    int cue_num = _cue_data.size()-1;
    if (!cue.isLink()) {
      _cue_names.insert(std::pair<std::string, int>(cue.GetName(), cue_num));
      std::clog << "Cuelist " << _name << ": added cue " << cue.GetName() << " as #" << _cue_data.size()-1 << std::endl;
    } else {
      std::clog << "Cuelist " << _name << ": added link to cue '" << cue.GetName() << "' as #" << _cue_data.size()-1 << std::endl;
    }
  }

  void Cuelist::NextCue(const int direct) {
    if (was_direct_) {
      was_direct_ = false;
      return;
    }

    if (direct != -1) {
      if (direct>(_cue_data.size()-1)) { // called cue too large
        std::clog << kLogInfo << "Tried direct call to cue " << direct << " in cuelist " << _name << ": too large" << std::endl;
        return;
      }
      current_cue_ = direct-1;
      if (!cue_halted_) // only if cuelist was running
        was_direct_ = true;
    }

    if (_cue_data.size()>(current_cue_+1)) {
      current_cue_++;
      knxdmxd::Cue cue = _cue_data.at(current_cue_);
      if (cue.isLink()) {
        current_cue_ = _cue_names.find(cue.GetName())->second;
      }
        
      _cue_data.at(current_cue_).Go();
    
      float waittime;
      int nextCuenum = current_cue_+1;
      if (_cue_data.size()>nextCuenum) { // last cue stops automatically
        knxdmxd::Cue nextCue = _cue_data.at(nextCuenum);
        if (nextCue.isLink()) {
          nextCuenum = _cue_names.find(nextCue.GetName())->second;
          nextCue = _cue_data.at(nextCuenum);
        }
        waittime = nextCue.GetWaitTime();
        if ((waittime>=0 && !cue_halted_)) {
          DMXSender::GetOLAClient().GetSelectServer()->RegisterSingleTimeout(
            (int) waittime*1000, ola::NewSingleCallback(this, &Cuelist::NextCue, -1));
        }
      }
    }
  }

  void Cuelist::Go() {
    cue_halted_=false;
    NextCue(-1);
  }

  void Cuelist::Halt() {
    cue_halted_=true;
  }

  void Cuelist::Direct(const int value) {
    NextCue(value); 
  }

}
