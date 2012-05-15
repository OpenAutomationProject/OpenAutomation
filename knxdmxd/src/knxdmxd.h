/*
 * knxdmxd.h
 *
 * (c) by JNK 2012
 *
 *
*/

#ifndef KNXDMXD_H
#define KNXDMXD_H

#include <iostream>
#include <map>
#include <vector>
#include <log.h>

#include <eibclient.h>
#include <ola/DmxBuffer.h>

#define FADING_INTERVAL 25000 // in us
#define DMX_INTERVAL 25 // in ms

namespace knxdmxd {
//  typedef std::vector<std::string> svector_t;
  typedef std::pair<int, std::string> knx_patch_map_element_t;
  typedef std::multimap<int, std::string>  knx_patch_map_t;
  typedef struct { int knx; int value; int type; } Trigger;
  
  const int kFixture = 1;
  const int kScene = 2;
  const int kCue = 4;
  const int kCuelist = 8;
  
  const int kTriggerGo = 1;
  const int kTriggerHalt = 2;
  
}

eibaddr_t readgaddr (const std::string addr);
int readdaddr (const std::string addr);

#endif


