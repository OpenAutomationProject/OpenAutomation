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

PRG1	= ebus_send
OBJ1	= ebus_send.o

PRG2	= check
OBJ2	= check.o

INCDIR	= -I../lib
LIBDIR	= -L../lib

CC	= gcc
CFLAGS	= -g -O2 -Wall $(INCDIR)
LDFLAGS	= -lebus

all: $(PRG1) $(PRG2)

$(PRG1): $(OBJ1)
	$(CC) $(CFLAGS) $(OBJ1) $(LIBDIR) $(LDFLAGS) -o $@  

$(PRG2): $(OBJ2)
	$(CC) $(CFLAGS) $(OBJ2) $(LIBDIR) $(LDFLAGS) -o $@
 
%.o: %.c %h
	$(CC) $(CFLAGS) -c $<
clean:
	-rm -f $(PRG1) $(OBJ1) $(PRG2) $(OBJ2) 
	
.PHONY: all clean
