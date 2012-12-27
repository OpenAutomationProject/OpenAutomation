/*
 * Copyright (C) Roland Jax 2012 <roland.jax@liwest.at>
 *
 * This file is part of ebusd.
 *
 * ebusd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ebusd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ebusd. If not, see http://www.gnu.org/licenses/.
 */

#ifndef MAIN_H_
#define MAIN_H_

//#include "utils.h"
//#include "log.h"

#define YES 1
#define NO  0


#define EBUSD_VERSION		"0.0.1"

#define EBUSD_DAEMON		"ebusd"

#define EBUSD_LOGLEVEL		ERR
#define EBUSD_FOREGROUND	NO
#define EBUDS_WORKDIR		"/tmp/"
#define EBUSD_CONFDIR		"/etc/ebusd/"
#define EBUSD_PIDFILE		"/var/run/ebusd.pid"
#define EBUSD_LOGFILE		"/var/log/ebusd.log"

#define SERIAL_DEVICE		"/dev/ttyUSB0"
#define SERIAL_BAUDRATE		B2400
#define SERIAL_BUFSIZE		512

#define SOCKET_PORT			8888
#define SOCKET_BUFSIZE		512

void decode_ebus_msg(unsigned char *data, int size);

int serial_open(const char *device, int *serialfd, struct termios *oldtermios);
int serial_close(int *serialfd, struct termios *oldtermios);

void signal_handle(int sig);

int pidfile_open();

void daemonize(char *workdir, const char *pidfile);

void cmdline(int *argc, char ***argv);

void cleanup(int state);

void main_loop(int serialfd, int socketfd);

#endif /* MAIN_H_ */
