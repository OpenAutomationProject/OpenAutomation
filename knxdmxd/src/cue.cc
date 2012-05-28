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
  _fadeIn = 0.0;
  _fadeOut = 0.0;
  _waittime = -1;
  _is_link = isLink;
  if (!_is_link) {
    std::clog << "Creating Cue '" << name << "'" << std::endl;
  } else {
    std::clog << "Creating link to cue '" << name << "'" << std::endl;
  }
}

void Cue::AddChannel(const cue_channel_t& channel) {
  _channel_data.push_back(channel);
  std::clog << "Cue '" << _name << "': added channel definition " << channel.fixture->GetName() << "/" << channel.name << "@" << channel.value << std::endl;
}

void Cue::SetFading(const float fadeIn, const float fadeOut) {
  _fadeIn = fadeIn;
  _fadeOut = (fadeOut < 0) ? fadeIn : fadeOut;
  
  if (_fadeIn<1.e-3) {
    _fadeIn = (FADING_INTERVAL/1.e6);
  }

  if (_fadeOut<1.e-3) {
    _fadeOut = (FADING_INTERVAL/1.e6);
  }
  
  std::clog << kLogDebug << "Cue '" << _name << "': set fading " << _fadeIn << "/" << _fadeOut << std::endl;
}

void Cue::SetWaittime(const float waittime) {
  _waittime = waittime;
  std::clog << kLogDebug << "Cue '" << _name << "': set waittime " << _waittime << std::endl;
}

bool Cue::isLink() {
  return _is_link;
}

void Cue::Go() {
  std::clog << kLogDebug << "Cue '" << _name << "': go " << std::endl;
  for(std::list<cue_channel_t>::iterator it = _channel_data.begin(); it != _channel_data.end(); ++it) {
    int currentValue = it->fixture->GetCurrentValue(it->name);
    int deltaVal = currentValue - it->value;
    float ft = (deltaVal>0) ? (deltaVal/(_fadeOut*1.e6/FADING_INTERVAL)) : (-deltaVal/(_fadeIn*1.e6/FADING_INTERVAL));
    it->fixture->Update(it->name, it->value, (float)ft);
  }
  std::clog << "Called cue " << _name << std::endl;
}

Cuelist::Cuelist(const std::string name) {
  _name=name;
  _current_cue=-1;
  _cue_halted=true;
  std::clog << "Creating Cuelist '" << name << "'" << std::endl;
}

void Cuelist::AddCue(knxdmxd::Cue& cue) {
  _cue_data.push_back(cue);
  int cue_num = _cue_data.size()-1;
  if (!cue.isLink()) {
    _cue_names.insert(std::pair<std::string, int>(cue.GetName(), cue_num));
    std::clog << "Cuelist '" << _name << "': added cue " << cue.GetName() << " as #" << _cue_data.size()-1 << std::endl;
  } else {
    std::clog << "Cuelist '" << _name << "': added link to cue '" << cue.GetName() << "' as #" << _cue_data.size()-1 << std::endl;
  }
}

void Cuelist::NextCue() {
  if (_cue_data.size()>(_current_cue+1)) {
    _current_cue++;
    knxdmxd::Cue cue = _cue_data.at(_current_cue);
    if (cue.isLink()) {
      _current_cue = _cue_names.find(cue.GetName())->second;
    }
        
    _cue_data.at(_current_cue).Go();
    
    float waittime;
    int nextCuenum = _current_cue+1;
    if (_cue_data.size()>nextCuenum) { // last cue stops automatically
      knxdmxd::Cue nextCue = _cue_data.at(nextCuenum);
      if (nextCue.isLink()) {
        nextCuenum = _cue_names.find(nextCue.GetName())->second;
        nextCue = _cue_data.at(nextCuenum);
      }
      waittime = nextCue.GetWaitTime();
      if ((waittime>=0 && !_cue_halted)) {
        DMXSender::GetOLAClient().GetSelectServer()->RegisterSingleTimeout(
          (int) waittime*1000, ola::NewSingleCallback(this, &Cuelist::NextCue));
      }
    }
  }
}

void Cuelist::Go() {
  _cue_halted=false;
  NextCue();
}

void Cuelist::Halt() {
  _cue_halted=true;
}
}
