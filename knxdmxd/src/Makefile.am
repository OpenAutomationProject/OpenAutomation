# what flags you want to pass to the C compiler & linker
#CPPFLAGS = --pedantic -Wall -std=c99 -O2
#LDFLAGS =

knxdmxd_CXXFLAGS = $(OLA_CFLAGS) $(AM_CXXFLAGS)
knxdmxd_LDADD    = $(OLA_LIBS)

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
bin_PROGRAMS = knxdmxd
knxdmxd_SOURCES = knxdmxd.cc log.cc fixture.cc cue.cc cuelist.cc dmxsender.cc
