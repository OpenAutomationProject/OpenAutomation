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
#include "ebus.h"
#include "ebusd.h"


/* global variables */
const char *progname;

static int pidfile_locked = NO;

static int pidfd = -1; /* pidfile file descriptor */
static int serialfd = -1; /* serial file descriptor */
static int socketfd = -1; /* socket file descriptor */

static int loglevel = DAEMON_LOGLEVEL;
static int foreground = DAEMON_FOREGROUND;
static int dump = DAEMON_DUMP;
static int nosyn = DAEMON_NOSYN;

static const char *confdir = DAEMON_CONFDIR;
static const char *pidfile = DAEMON_PIDFILE;
static const char *logfile = DAEMON_LOGFILE;
static const char *dumpfile = DAEMON_DUMPFILE;

static const char *serial = SERIAL_DEVICE;

static int listenport = SOCKET_PORT;

void
signal_handler(int sig) {
	switch(sig) {
	case SIGHUP:
		log_print_msg(INF, "received SIGHUP");
		break;
	case SIGINT:
	case SIGTERM:
		log_print_msg(INF, "daemon exiting");
		cleanup(EXIT_SUCCESS);
		break;
	default:
		log_print_msg(INF, "unknown signal %s", strsignal(sig));
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
	if (pidfile_open(pidfile, &pidfd) == -1)
		cleanup(EXIT_FAILURE);

	pidfile_locked = YES;
	log_print_msg(INF, "pid file %s successfully created.", pidfile);

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
		if (!socket_close(socketfd))
			log_print_msg(INF, "tcp port %d successfully closed.", listenport);

	/* close serial device */
	if (serialfd > 0)
		if (!serial_close())
			log_print_msg(INF, "serial device %s successfully closed.", serial);

	/* close dumpfile */
	if (dump)
		if (!dumpfile_close())
			log_print_msg(INF, "dumpfile %s successfully closed.", dumpfile);

	if (!foreground) {

		/* delete PID file */
		if (pidfile_locked)
			if (!pidfile_close(pidfile, pidfd))
				log_print_msg(INF, "pid file %s successfully deleted.", pidfile);

		/* Reset all signal handlers to default */
		signal(SIGCHLD, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		signal(SIGTTOU, SIG_DFL);
		signal(SIGTTIN, SIG_DFL);
		signal(SIGHUP,  SIG_DFL);
		signal(SIGINT,  SIG_DFL);
		signal(SIGTERM, SIG_DFL);

		/* print end message */
		log_print_msg(INF, DAEMON_NAME " " DAEMON_VERSION " stopped.");
		syslog(LOG_INFO, DAEMON_NAME " " DAEMON_VERSION " stopped.");
	}

	/* close logging system */
	log_close();

	exit(state);
}

void
cmdline(int *argc, char ***argv)
{
	static struct option opts[] = {
		{"confdir",    required_argument, NULL, 'c'},
		{"dumpfile",   required_argument, NULL, 'D'},
		{"dump",       no_argument,       NULL, 'd'},
		{"foreground", no_argument,       NULL, 'f'},
		{"logfile",    required_argument, NULL, 'L'},
		{"loglevel",   required_argument, NULL, 'l'},
		{"nosyn",      no_argument,       NULL, 'n'},
		{"pidfile",    required_argument, NULL, 'p'},
		{"listenport", required_argument, NULL, 'P'},
		{"serial",     required_argument, NULL, 's'},
		{"version",    no_argument,       NULL, 'v'},
		{"help",       no_argument,       NULL, 'h'},
		{NULL,         no_argument,       NULL,  0 },
	};

	for (;;) {
		int i;

		i = getopt_long(*argc, *argv, "c:D:dfL:l:np:P:s:vh", opts, NULL);

		if (i == -1)
			break;

		switch (i) {
		case 'c':
			confdir = optarg;
			break;
		case 'D':
			dumpfile = optarg;
			dump = YES;
			break;
		case 'd':
			dump = YES;
			break;
		case 'f':
			foreground = YES;
			break;
		case 'L':
			logfile = optarg;
			break;
		case 'l':
			if (isdigit(*optarg)) {
				int l = atoi(optarg);
				if (INF <= l && l <= DBG)
					loglevel = l;
				else
					loglevel = ERR;
			}
			break;
		case 'n':
			nosyn = YES;
			break;
		case 'p':
			pidfile = optarg;
			break;
		case 'P':
			if (isdigit(*optarg))
				listenport = atoi(optarg);
			break;
		case 's':
			serial = optarg;
			break;
		case 'v':
			fprintf(stdout, DAEMON_NAME " " DAEMON_VERSION "\n");
			exit(EXIT_SUCCESS);
		case 'h':
		default:
			fprintf(stdout, "\nUsage: %s [OPTIONS]\n"
							"  -c --confdir      set the configuration directory. (%s)\n"
							"  -D --dumpfile     use a specified dump file. (%s)\n"
							"  -d --dump         dump raw ebus messages to dump file.\n"
							"  -f --foreground   run in foreground.\n"
							"  -L --logfile      use a specified log file. (%s)\n"
							"  -l --loglevel     set log level. (INF | INF=0 WAR=1 ERR=2 DBG=3)\n"
							"  -n --nosyn        discard syn in logfile\n"
							"  -p --pidfile      use a specified pid file. (%s)\n"
							"  -P --listenport   use a specified listening port. (%d)\n"
							"  -s --serial       use a specified serial device. (%s)\n"
							"  -v --version      print version information.\n"
							"  -h --help         print this message.\n"
							"\n",
							progname,
							DAEMON_CONFDIR,
							DAEMON_DUMPFILE,
							DAEMON_LOGFILE,
							DAEMON_PIDFILE,
							SOCKET_PORT,
							SERIAL_DEVICE);
			exit(EXIT_FAILURE);
			break;
		}
	}
}

void
main_loop()
{
	fd_set listenfds;
	int maxfd;

	FD_ZERO(&listenfds);
	FD_SET(serialfd, &listenfds);
	FD_SET(socketfd, &listenfds);

	maxfd = socketfd;

	/* serialfd should be always lower then socketfd */
	if (serialfd > socketfd) {
		log_print_msg(ERR, "serialfd %d > %d socketfd", serialfd, socketfd);
		cleanup(EXIT_FAILURE);
	}

	for (;;) {
		fd_set readfds;
		int readfd;
		int ret;

		/* set readfds to inital listenfds */
		readfds = listenfds;

		ret = select(maxfd + 1, &readfds, NULL, NULL, NULL);

		//todo: handle signals instead of ignore ?

		/* ignore signals*/
		if ((ret < 0) && (errno == EINTR)) {
			log_print_msg(INF, "get signal at select: %s", strerror(errno));
			continue;
		} else if (ret < 0) {
			err_if(1);
			cleanup(EXIT_FAILURE);
		}

		/* new data from serial port? */
		if (FD_ISSET(serialfd, &readfds)) {
			unsigned char serbuf[SERIAL_BUFSIZE];
			int serbuflen;

			serbuflen = sizeof(serbuf);

			/* get message from client */
			ret = serial_ebus_get_msg(serialfd, serbuf, &serbuflen, dump, nosyn);
			if (ret == -1)
				log_print_msg(WAR,"serial device reading: *buflen < 0 || *buflen > maxlen");

		}

		/* new incoming connection at TCP port arrived? */
		if (FD_ISSET(socketfd, &readfds)) {
			ret = socket_client_accept(socketfd, &readfd);
			if (readfd >= 0) {
				/* add new TCP client FD to listenfds */
				FD_SET(readfd, &listenfds);
				(readfd > maxfd) ? (maxfd = readfd) : (1);
			}
		}

		/* run through connected sockets for new data */
		for (readfd = socketfd + 1; readfd <= maxfd; ++readfd) {

			if (FD_ISSET(readfd, &readfds)) {
				char tcpbuf[SOCKET_BUFSIZE];
				int tcpbuflen = sizeof(tcpbuf);

				/* get message from client */
				ret = socket_client_read(readfd, tcpbuf, &tcpbuflen);

				if (ret == -1) {
					/* remove dead TCP client FD from listenfds */
					FD_CLR(readfd, &listenfds);
				} else {
					/* just echo message to sender */
					socket_client_write(readfd, tcpbuf, tcpbuflen);
				}
			}
		}
	}
}

int
main(int argc, char *argv[])
{

	/* set progname */
	progname = (const char *)strrchr(argv[0], '/');
	progname = progname ? (progname + 1) : argv[0];

	/* read command line  */
	cmdline(&argc, &argv);

	/* open log */
	log_set_level(loglevel);
	log_open(logfile, foreground);

	/* to be daemon */
	if (!foreground) {
		log_print_msg(INF, DAEMON_NAME " " DAEMON_VERSION " started.");
		syslog(LOG_INFO, DAEMON_NAME " " DAEMON_VERSION " started.");
		daemonize();
	}

	/* open dump file */
	if (dump) {
		if (dumpfile_open(dumpfile) == -1)
			cleanup(EXIT_FAILURE);
		else
			log_print_msg(INF, "dumpfile %s successfully opened.", dumpfile);

	}

	/* open serial device */
	if (serial_open(serial, &serialfd) == -1)
		cleanup(EXIT_FAILURE);
	else
		log_print_msg(INF, "serial device %s successfully opened.", serial);


	/* open listing tcp socket */
	if (socket_open(&socketfd, listenport) == -1)
		cleanup(EXIT_FAILURE);
	else
		log_print_msg(INF, "tcp port %d successfully opened.", listenport);


	/* enter main loop */
	main_loop();

	cleanup(EXIT_SUCCESS);

	return 0;
}
