CC		= gcc
LIBS	= 
CFLAGS	= -Os -g -Wall -DVERSION
PRG		= ebusd
OBJS	= log.o ebus.o main.o

all: $(PRG)

$(PRG): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)
 
%.o: %.c
	$(CC) -c $(CFLAGS) $<

clean:
	-rm -f $(OBJS) $(PRG)
	
.PHONY: clean
