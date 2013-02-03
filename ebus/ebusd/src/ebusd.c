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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <syslog.h>
#include <termios.h>
#include <errno.h>

#include "log.h"
#include "utils.h"
//~ #include "ebus-common.h"
#include "ebus-bus.h"
#include "ebusd.h"


/* global variables */
const char *progname;

static int pidfile_locked = NO;

static int pidfd = UNSET; /* pidfile file descriptor */
static int serialfd = UNSET; /* serial file descriptor */
static int socketfd = UNSET; /* socket file descriptor */


static char address[2];
static char cfgdir[CFG_LINELEN];
static char cfgfile[CFG_LINELEN];
static char device[CFG_LINELEN];
static char extension[5];
static int foreground = UNSET;
static char loglevel[CFG_LINELEN];
static char logfile[CFG_LINELEN];
static char pidfile[CFG_LINELEN];
static int port = UNSET;
static int rawdump = UNSET;
static char rawfile[CFG_LINELEN];
static int showraw = UNSET;
static int settings = UNSET;
static int max_retry = UNSET;
static int skip_ack = UNSET;
static int max_wait = UNSET;


static char options[] = "a:c:C:d:e:fl:L:P:p:rR:sSvh";

static struct option opts[] = {
	{"address",    required_argument, NULL, 'a'},
	{"cfgfdir",    required_argument, NULL, 'c'},
	{"cfgfile",    required_argument, NULL, 'C'},
	{"device",     required_argument, NULL, 'd'},
	{"extension",  required_argument, NULL, 'e'},
	{"foreground", no_argument,       NULL, 'f'},
	{"loglevel",   required_argument, NULL, 'l'},
	{"logfile",    required_argument, NULL, 'L'},
	{"pidfile",    required_argument, NULL, 'P'},
	{"port",       required_argument, NULL, 'p'},
	{"rawdump",    no_argument,       NULL, 'r'},
	{"rawfile",    required_argument, NULL, 'R'},
	{"showraw",    no_argument,       NULL, 's'},	
	{"settings",   no_argument,       NULL, 'S'},
	{"version",    no_argument,       NULL, 'v'},
	{"help",       no_argument,       NULL, 'h'},
	{NULL,         no_argument,       NULL,  0 },
};

static struct config cfg[] = {

{"address",    STR, &address, "\tbus address (" NUMSTR(EBUS_QQ) ")"},
{"cfgdir",     STR, &cfgdir, "\tconfiguration directory of command files " \
							"(" DAEMON_CFGDIR ")"},
{"cfgfile",    STR, &cfgfile, "\tdaemon configuration file (" DAEMON_CFGFILE ")"},
{"device",     STR, &device, "\tserial device (" SERIAL_DEVICE ")"},
{"extension",  STR, &extension, "extension of command files (" DAEMON_EXTENSION ")"},
{"foreground", BOL, &foreground, "run in foreground"},
{"loglevel",   STR, &loglevel, "\tlog level (INF | " LOGTXT ")"},
{"logfile",    STR, &logfile, "\tlog file (" DAEMON_LOGFILE ")"},
{"pidfile",    STR, &pidfile, "\tpid file (" DAEMON_PIDFILE ")"},
{"port",       NUM, &port, "\tport (" NUMSTR(SOCKET_PORT) ")"},
{"rawdump",    BOL, &rawdump, "\tdump raw ebus data to file"},
{"rawfile",    STR, &rawfile, "\traw file (" DAEMON_RAWFILE ")"},
{"showraw",    BOL, &showraw, "\tprint raw data"},
{"settings",   BOL, &settings, "\tprint daemon settings"},
{"max_retry",  NUM, &max_retry, NULL},
{"skip_ack",   NUM, &skip_ack, NULL},
{"max_wait",   NUM, &max_wait, NULL},
{"version",    STR, NULL, "\tprint version information"},
{"help",       STR, NULL, "\tprint this message"}
};

const int cfglen = sizeof(cfg) / sizeof(cfg[0]);

void
usage()
{
	fprintf(stdout, "\nUsage: %s [OPTIONS]\n", progname);

	int i, skip;

	skip = 0;

	for (i = 0; i < cfglen; i++) {
		if (cfg[i].info != NULL) {
			fprintf(stdout, "  -%c --%s\t%s\n",
				opts[i - skip].val,
				opts[i - skip].name,
				cfg[i].info);
		} else {
			skip++;
		}
	}

	fprintf(stdout, "\n");
}

void
cmdline(int *argc, char ***argv)
{
	for (;;) {
		int i;

		i = getopt_long(*argc, *argv, options, opts, NULL);

		if (i == -1)
			break;

		switch (i) {
		case 'a':
			if (strlen(optarg) > 2)
				strncpy(address, &optarg[strlen(optarg) - 2 ], 2);				
			else
				strncpy(address, optarg, strlen(optarg));

			break;			
		case 'c':
			strncpy(cfgdir, optarg, strlen(optarg));
			break;
		case 'C':
			strncpy(cfgfile, optarg, strlen(optarg));
			break;
		case 'd':
			strncpy(device, optarg, strlen(optarg));
			break;
		case 'e':
			if (strlen(optarg) > 3) {
				usage();
				exit(EXIT_FAILURE);
			}
			extension[0] = '.';
			strncpy(&extension[1], optarg, strlen(optarg));
			break;						
		case 'f':
			foreground = YES;
			break;
		case 'l':
			strncpy(loglevel, optarg, strlen(optarg));
			break;
		case 'L':
			strncpy(logfile, optarg, strlen(optarg));
			break;
		case 'P':
			strncpy(pidfile, optarg, strlen(optarg));
			break;
		case 'p':
			if (isdigit(*optarg))
				port = atoi(optarg);
			break;
		case 'r':
			rawdump = YES;
			break;
		case 'R':
			strncpy(rawfile, optarg, strlen(optarg));
			rawdump = YES;
			break;
		case 's':
			showraw = YES;
			break;			
		case 'S':
			settings = YES;
			break;						
		case 'v':
			fprintf(stdout, DAEMON_NAME " " DAEMON_VERSION "\n");
			exit(EXIT_SUCCESS);
		case 'h':
		default:
			usage();
			exit(EXIT_FAILURE);
			break;
		}
	}
}

void
set_unset()
{

	if (*address == '\0')
		strncpy(address , &NUMSTR(EBUS_QQ)[2], 2);

	if (*cfgdir == '\0')
		strncpy(cfgdir , DAEMON_CFGDIR, strlen(DAEMON_CFGDIR));

	if (*device == '\0')
		strncpy(device , SERIAL_DEVICE, strlen(SERIAL_DEVICE));

	if (*extension == '\0') {
		extension[0] = '.';
		strncpy(&extension[1] , DAEMON_EXTENSION, strlen(DAEMON_EXTENSION));
	}

	if (foreground == UNSET)
		foreground = NO;		

	if (*loglevel == '\0')
		strncpy(loglevel , DAEMON_LOGLEVEL, strlen(DAEMON_LOGLEVEL));

	if (*logfile == '\0')
		strncpy(logfile , DAEMON_LOGFILE, strlen(DAEMON_LOGFILE));

	if (*pidfile == '\0')
		strncpy(pidfile , DAEMON_PIDFILE, strlen(DAEMON_PIDFILE));

	if (port == UNSET)
		port = SOCKET_PORT;

	if (rawdump == UNSET)
		rawdump = NO;

	if (*rawfile == '\0')
		strncpy(rawfile , DAEMON_RAWFILE, strlen(DAEMON_RAWFILE));
		
	if (showraw == UNSET)
		showraw = NO;		

	if (settings == UNSET)
		settings = NO;

	if (max_retry == UNSET)
		max_retry = EBUS_MAX_RETRY;

	if (skip_ack == UNSET)
		skip_ack = EBUS_SKIP_ACK;

	if (max_wait == UNSET)
		max_wait = EBUS_MAX_WAIT;

		
}


void
signal_handler(int sig) {
	switch(sig) {
	case SIGHUP:
		log_print(L_NOT, "received SIGHUP");
		break;
	case SIGINT:
	case SIGTERM:
		log_print(L_INF, "daemon exiting");
		cleanup(EXIT_SUCCESS);
		break;
	default:
		log_print(L_NOT, "unknown signal %s", strsignal(sig));
		break;
	}
}

void
daemonize()
{
	pid_t pid;

	/* fork off the parent process */
	pid = fork();
	if (pid < 0) {
		err_if(1);
		cleanup(EXIT_FAILURE);
	}
	/* If we got a good PID, then we can exit the parent process */
	if (pid > 0) {
		/* printf("Child process created: %d\n", pid); */
		exit(EXIT_SUCCESS);
	}

	/* At this point we are executing as the child process */

	/* Set file permissions 750 */
	umask(027);

	/* Create a new SID for the child process and */
	/* detach the process from the parent (normally a shell) */
	if (setsid() < 0) {
		err_if(1);
		cleanup(EXIT_FAILURE);
	}

	/* Change the current working directory.  This prevents the current
	   directory from being locked; hence not being able to remove it. */
	if (chdir(DAEMON_WORKDIR) < 0) {
		/* Log any failure here */
		err_if(1);
		cleanup(EXIT_FAILURE);
	}

	/* Route I/O connections */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* write pidfile and try to lock it */
	if (pid_file_open(pidfile, &pidfd) == -1) {
		log_print(L_ERR, "can't open pidfile: %s\n", pidfile);
		cleanup(EXIT_FAILURE);
	} else {
		pidfile_locked = YES;
		log_print(L_INF, "%s created.", pidfile);
	}

    /* Cancel certain signals */
	signal(SIGCHLD, SIG_DFL); /* A child process dies */
    signal(SIGTSTP, SIG_IGN); /* Various TTY signals */
    signal(SIGTTOU, SIG_IGN); /* Ignore TTY background writes */
    signal(SIGTTIN, SIG_IGN); /* Ignore TTY background reads */

    /* Trap signals that we expect to receive */
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

void
cleanup(int state)
{

	/* close listing tcp socket */
	if (socketfd > 0)
		if (sock_close(socketfd) == -1)
			log_print(L_INF, "port %d closed.", port);

	/* close serial device */
	if (serialfd > 0)
		if (eb_serial_close() == -1)
			log_print(L_INF, "%s closed.", device);

	/* close rawfile */
	if (rawdump == YES)
		if (eb_raw_file_close() == -1)
			log_print(L_INF, "%s closed.", rawfile);

	if (foreground == NO) {

		/* delete PID file */
		if (pidfile_locked)
			if (pid_file_close(pidfile, pidfd) == -1)
				log_print(L_INF, "%s deleted.", pidfile);

		/* Reset all signal handlers to default */
		signal(SIGCHLD, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		signal(SIGTTOU, SIG_DFL);
		signal(SIGTTIN, SIG_DFL);
		signal(SIGHUP,  SIG_DFL);
		signal(SIGINT,  SIG_DFL);
		signal(SIGTERM, SIG_DFL);

		/* print end message */
		log_print(L_ALL, DAEMON_NAME " " DAEMON_VERSION " stopped.");
		syslog(LOG_INFO, DAEMON_NAME " " DAEMON_VERSION " stopped.");
	}

	/* close logging system */
	log_close();

	exit(state);
}


void
main_loop()
{
	int maxfd;
	fd_set listenfds;

	FD_ZERO(&listenfds);
	FD_SET(serialfd, &listenfds);
	FD_SET(socketfd, &listenfds);

	maxfd = socketfd;

	/* serialfd should be always lower then socketfd */
	if (serialfd > socketfd) {
		log_print(L_ERR, "serialfd %d > %d socketfd", serialfd, socketfd);
		cleanup(EXIT_FAILURE);
	}

	for (;;) {
		fd_set readfds;
		int readfd;
		int ret, i;

		/* set readfds to inital listenfds */
		readfds = listenfds;

		ret = select(maxfd + 1, &readfds, NULL, NULL, NULL);

		//todo: handle signals instead of ignore ?

		/* ignore signals*/
		if ((ret < 0) && (errno == EINTR)) {
			log_print(L_NOT,
				"get signal at select: %s", strerror(errno));
			continue;
		} else if (ret < 0) {
			err_if(1);
			cleanup(EXIT_FAILURE);
		}

		/* new data from serial port? */
		if (FD_ISSET(serialfd, &readfds)) {
			unsigned char serbuf[SERIAL_BUFSIZE];
			int serbuflen;

			memset(serbuf, '\0', sizeof(serbuf));
			serbuflen = sizeof(serbuf);

			/* get message from client */
			eb_cyc_data_recv(serbuf, &serbuflen);
				
		}

		/* new incoming connection at TCP port arrived? */
		if (FD_ISSET(socketfd, &readfds)) {

			/* get new TCP client fd*/
			ret = sock_client_accept(socketfd, &readfd);
			if (readfd >= 0) {
				/* add new TCP client fd to listenfds */
				FD_SET(readfd, &listenfds);
				(readfd > maxfd) ? (maxfd = readfd) : (1);
			}
		}

		/* run through connected sockets for new data */
		for (readfd = socketfd + 1; readfd <= maxfd; ++readfd) {

			/* check all connected clients */
			if (FD_ISSET(readfd, &readfds)) {
				char tcpbuf[SOCKET_BUFSIZE];
				int tcpbuflen = sizeof(tcpbuf);

				/* get message from client */
				ret = sock_client_read(readfd, tcpbuf, &tcpbuflen);

				if (ret == -1) {
					/* remove dead TCP client */
					FD_CLR(readfd, &listenfds);
				}

				ret = cmd_decode(tcpbuf, tcpbuflen);

				//~ else {
					//~ /* just echo message to sender */
					//~ sock_client_write(readfd, tcpbuf, tcpbuflen);
				//~ }
			}
		}
	}
}

int
main(int argc, char *argv[])
{
	int tmp;
	
	/* set progname */
	progname = (const char *)strrchr(argv[0], '/');
	progname = progname ? (progname + 1) : argv[0];

	/* read command line  */
	cmdline(&argc, &argv);
	
	/* set default cfgfile */
	if (*cfgfile == '\0')
		strncpy(cfgfile , DAEMON_CFGFILE, strlen(DAEMON_CFGFILE));

	/* read config file */
	if (cfg_file_read(cfgfile, cfg, cfglen) == -1)
		fprintf(stderr, "can't open cfgfile: %s ==> " \
				"build in settings will be used.\n", cfgfile);	

	/* set unset configuration */
	set_unset();

	/* print configuration */
	if (settings == YES)
		cfg_print(cfg, cfglen);

	
	/* set ebus configuration */
	eb_set_rawdump(rawdump);
	eb_set_showraw(showraw);

	tmp = (eb_htoi(&address[0])) * 16 + (eb_htoi(&address[1]));
	eb_set_qq((unsigned char) tmp);

	eb_set_max_retry(max_retry);
	eb_set_skip_ack(skip_ack);
	eb_set_max_wait(max_wait);	

	/* open log */
	log_level(loglevel);
	log_open(logfile, foreground);	

	/* to be daemon */
	if (foreground == NO) {
		log_print(L_ALL, DAEMON_NAME " " DAEMON_VERSION " started.");
		syslog(LOG_INFO, DAEMON_NAME " " DAEMON_VERSION " started.");
		daemonize();
	}

	/* read ebus command configuration files */
	eb_cmd_dir_read(cfgdir, extension);

	/* open raw file */
	if (rawdump == YES) {
		if (eb_raw_file_open(rawfile) == -1) {
			log_print(L_ERR, "can't open rawfile: %s\n", rawfile);
			cleanup(EXIT_FAILURE);
		} else {
			log_print(L_INF, "%s opened.", rawfile);
		}

	}

	/* open serial device */
	if (eb_serial_open(device, &serialfd) == -1) {
		log_print(L_ERR, "can't open device: %s", device);
		cleanup(EXIT_FAILURE);
	} else {
		log_print(L_INF, "%s opened.", device);
	}


	/* open listing tcp socket */
	if (sock_open(&socketfd, port) == -1) {
		log_print(L_ERR, "can't open port: %d", port);
		cleanup(EXIT_FAILURE);
	} else {
		log_print(L_INF, "port %d opened.", port);
	}

	/* enter main loop */
	main_loop();

	cleanup(EXIT_SUCCESS);

	return 0;
}