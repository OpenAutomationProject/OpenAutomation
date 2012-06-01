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
#include <log.h>

#include <eibclient.h>
#include <ola/DmxBuffer.h>

#define FADING_INTERVAL 25000 // in us
#define DMX_INTERVAL 25 // in ms

namespace knxdmxd {
  const int kFixture = 1;
  const int kScene = 2;
  const int kCue = 4;
  const int kCuelist = 8;
  
  class DMXSender;
}

eibaddr_t readgaddr (const std::string addr);

#endif


