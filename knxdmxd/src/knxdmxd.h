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

namespace knxdmxd {
  typedef std::vector<std::string> svector_t;
  typedef std::pair<int, std::string> knx_patch_map_element_t;
  typedef std::multimap<int, std::string>  knx_patch_map_t;
}

eibaddr_t readgaddr (const std::string addr);
int readdaddr (const std::string addr);

#endif


