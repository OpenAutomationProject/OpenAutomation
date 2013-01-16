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

#define DAEMON_VERSION        "0.0.1"
#define DAEMON_NAME           "ebusd"

#define DAEMON_LOGLEVEL       ERR
#define DAEMON_FOREGROUND     NO
#define DAEMON_DUMP           NO
#define DAEMON_NOSYN          NO

#define DAEMON_WORKDIR        "/tmp/"
#define DAEMON_CONFDIR        "/etc/ebusd/"
#define DAEMON_PIDFILE        "/var/run/ebusd.pid"
#define DAEMON_LOGFILE        "/var/log/ebusd.log"
#define DAEMON_DUMPFILE       "/tmp/ebusd.bin"

void signal_handler(int sig);

void daemonize();

void cleanup(int state);

void cmdline(int *argc, char ***argv);

void main_loop();

#endif /* MAIN_H_ */
