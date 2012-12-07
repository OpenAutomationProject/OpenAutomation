/*
 * Author:  Jax Roland
 * Date:    29.11.2012
 * License: GPLv3
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


void signal_handler(int sig);
int lock_pidfile();
void daemonize(char *workdir, const char *pidfile);
static int cmdline_read(int *argc, char ***argv);
void cleanup(int state);

#endif /* MAIN_H_ */
