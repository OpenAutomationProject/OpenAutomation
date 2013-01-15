#
# Copyright (C) Roland Jax 2012-2013 <roland.jax@liwest.at>
#
# This file is part of ebusd.
#
# ebusd is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ebusd is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ebusd. If not, see http://www.gnu.org/licenses/.
#

ifeq ($(DEBUG), 1)
	CFLAGS	= -g -O2 -Wall -DDEBUG
else
	CFLAGS	= -g -O2 -Wall
endif

CC	= gcc
LIBS	= 
PRG1	= ebus_send
OBJS1	= ebus.o ebus_send.o
PRG2	= check
OBJS2	= ebus.o check.o

all: $(PRG1) $(PRG2)

$(PRG1): $(OBJS1)
	$(CC) -o $@ $(OBJS1) $(LIBS)

$(PRG2): $(OBJS2)
	$(CC) -o $@ $(OBJS2) $(LIBS)
 
%.o: %.c
	$(CC) -c $(CFLAGS) $<

clean:
	-rm -f $(OBJS1) $(PRG1) $(OBJS2) $(PRG2)
	
.PHONY: clean
