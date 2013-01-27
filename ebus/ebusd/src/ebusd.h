/*
 * Copyright (C) Roland Jax 2012-2013 <roland.jax@liwest.at>
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

#define DAEMON_NAME           "ebusd"
#define DAEMON_VERSION        "0.1"

#define DAEMON_WORKDIR        "/tmp/"

#define DAEMON_CFGDIR         "/etc/ebusd"
#define DAEMON_CFGFILE        DAEMON_CFGDIR"/ebusd.conf"
#define DAEMON_FOREGROUND     NO
#define DAEMON_LOGLEVEL       "INF"
#define DAEMON_LOGFILE        "/var/log/ebusd.log"
#define DAEMON_NOSYN          NO
#define DAEMON_PIDFILE        "/var/run/ebusd.pid"
#define DAEMON_RAWFILE        "/tmp/ebusd.bin"
#define DAEMON_RAWDUMP        NO


void usage();

void cmdline(int *argc, char ***argv);

void set_unset();

void signal_handler(int sig);

void daemonize();

void cleanup(int state);

void print_ebus_msg(const unsigned char *buf, int buflen);

int parse_cycle_data(unsigned char *buf, int *buflen);

void main_loop();

#endif /* MAIN_H_ */
