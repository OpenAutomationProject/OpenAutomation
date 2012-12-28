#
# Copyright (C) Roland Jax 2012 <roland.jax@liwest.at>
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

CC		= gcc
LIBS	= 
CFLAGS	= -Os -g -Wall
PRG		= ebusd
OBJS	= log.o utils.o ebus.o main.o

all: $(PRG)

$(PRG): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)
 
%.o: %.c
	$(CC) -c $(CFLAGS) $<

clean:
	-rm -f $(OBJS) $(PRG)
	
.PHONY: clean
