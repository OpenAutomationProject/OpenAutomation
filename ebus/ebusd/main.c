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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <termios.h>

#include "ebus.h"
#include "log.h"
#include "sock.h"
#include "main.h"


/* global variables */
const char *progname;

static int pidfile_locked = NO;

static int pidfd = -1; /* pidfile file descriptor */
static int serialfd = -1; /* serial file descriptor */
static int socketfd = -1; /* socket file descriptor */

static int loglevel = EBUSD_LOGLEVEL;
static int foreground = EBUSD_FOREGROUND;

static const char *confdir = EBUSD_CONFDIR;
static const char *pidfile = EBUSD_PIDFILE;
static const char *logfile = EBUSD_LOGFILE;

static struct termios oldtermios;
static const char *serial = SERIAL_DEVICE;

static int listenport = SOCKET_PORT;


void
decode_ebus_msg(unsigned char *data, int size)
{
	int k = 0;
	char buf[SERIAL_BUFSIZE];
	char tmp[4];

	memset(tmp, '\0', sizeof(tmp));
	memset(buf, '\0', sizeof(buf));

	for (k = 0; k < size; k++) {
		sprintf(tmp, " %02x", data[k]);
		strncat(buf, tmp, 3);
	}

	log_print_msg(DBG, "decode_ebus_msg: %s", buf);
}

int
serial_open(const char *device, int *serialfd, struct termios *oldtermios) {

	struct termios newtermios;

	/* open the serial port */
	if (!serialfd) {
		log_print_msg(ERR, "Could not open serial device %s - serialfd is NULL", serial);
		return -1;
	}

	*serialfd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);

	if (*serialfd == -1) {
		log_print_msg(ERR, "Could not open serial device %s - serialfd is -1", serial);
		return -1;
	}

	fcntl(*serialfd, F_SETFL, 0);

	/* save current settings of serial port */
	if (oldtermios) {
		tcgetattr(*serialfd, oldtermios);
	}

	memset(&newtermios, '\0', sizeof(newtermios));

	/* set new settings of serial port */
	newtermios.c_cflag = SERIAL_BAUDRATE | CS8 | CLOCAL | CREAD;
	newtermios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtermios.c_iflag = IGNPAR;
	newtermios.c_oflag &= ~OPOST;
	newtermios.c_cc[VMIN]  = 1;
	newtermios.c_cc[VTIME] = 0;

	tcflush(*serialfd, TCIFLUSH);

	/* activate new settings of serial port */
	tcsetattr(*serialfd, TCSANOW, &newtermios);

	return 0;
}

int
serial_close(int *serialfd, struct termios *oldtermios) {

	/* Reset serial device to default settings */
	tcsetattr(*serialfd, TCSANOW, oldtermios);

	/* Close file descriptor from serial device */
	close(*serialfd);

	return 0;
}

void
signal_handle(int sig)
{
	switch(sig)
	{
		case SIGHUP:
			log_print_msg(INF,  "Received SIGHUP signal.");
			break;
		case SIGINT:
		case SIGTERM:
			log_print_msg(INF,  "Daemon exiting");
			close(pidfd);
			cleanup(EXIT_SUCCESS);
			break;
		default:
			log_print_msg(INF,  "Unhandled signal %s", strsignal(sig));
			break;
	}
}

int
pidfile_open()
{
	char pid_str[10];

	/* open pidfile */
	pidfd = open(pidfile, O_RDWR|O_CREAT, 0600);

	if (pidfd == -1) {
		log_print_msg(ERR, "Could not open PID file %s, exiting", pidfile);
		return -1;
	}

	/* lock file */
	if (lockf(pidfd, F_TLOCK, 0) == -1) {
		log_print_msg(ERR, "Could not lock PID file %s, exiting", pidfile);
		close(pidfd);
		return -1;
	}

	/* write PID into pidfile */
	sprintf(pid_str, "%d\n", getpid());

	if (write(pidfd, pid_str, strlen(pid_str)) == -1) {
		log_print_msg(ERR, "Could not write PID into file %s, exiting", pidfile);
		close(pidfd);
		return -1;
	}

	return 0;
}

void
daemonize(char *workdir, const char *pidfile)
{
	pid_t pid;

	/* fork off the parent process */
	pid = fork();
	if (pid < 0) {
		log_print_msg(ERR, "fork: %s", strerror(errno));
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
		log_print_msg(ERR, "setsid: %s", strerror(errno));
		cleanup(EXIT_FAILURE);
	}

	/* Route I/O connections */
//	close(STDIN_FILENO);
//	close(STDOUT_FILENO);
//	close(STDERR_FILENO);

	/* Change the current working directory.  This prevents the current
	   directory from being locked; hence not being able to remove it. */
	if (chdir(workdir) < 0) {
		/* Log any failure here */
		log_print_msg(ERR, "chdir(\"/\"): %s", strerror(errno));
		cleanup(EXIT_FAILURE);
	}

	/* write pidfile and try to lock it */
	if (pidfile_open() < 0) {
		cleanup(EXIT_FAILURE);
	} else {
		pidfile_locked = YES;
		log_print_msg(DBG, "PID file %s created.", pidfile);
	}

    /* Cancel certain signals */
    signal(SIGCHLD, SIG_DFL); /* A child process dies */
    signal(SIGTSTP, SIG_IGN); /* Various TTY signals */
    signal(SIGTTOU, SIG_IGN); /* Ignore TTY background writes */
    signal(SIGTTIN, SIG_IGN); /* Ignore TTY background reads */

    /* Trap signals that we expect to receive */
    signal(SIGHUP,  signal_handle);
    signal(SIGINT,  signal_handle);
    signal(SIGTERM, signal_handle);

}

void
cmdline(int *argc, char ***argv)
{
	static struct option opts[] = {
		{"confdir",    required_argument, NULL, 'c'},
		{"foreground", no_argument,       NULL, 'f'},
		{"logfile",    required_argument, NULL, 'L'},
		{"loglevel",   required_argument, NULL, 'l'},
		{"pidfile",    required_argument, NULL, 'p'},
		{"listenport", required_argument, NULL, 'P'},
		{"serial",     required_argument, NULL, 's'},
		{"version",    no_argument,       NULL, 'v'},
		{"help",       no_argument,       NULL, 'h'},
		{NULL,         no_argument,       NULL,  0 },
	};

	for (;;) {
		int i;
		i = getopt_long(*argc, *argv, "c:L:l:fp:P:s:vh", opts, NULL);
		if (i == -1) {
			break;
		}
		switch (i) {
			case 'c':
				confdir = optarg;
				break;
			case 'L':
				logfile = optarg;
				break;
			case 'l':
				if (isdigit(*optarg)) {
					int l = atoi(optarg);
					if (INF <= l && l <= DBG) {
						loglevel = l;
					} else {
						loglevel = ERR;
					}
				}
				break;
			case 'f':
				foreground = 1;
				break;
			case 'p':
				pidfile = optarg;
				break;
			case 'P':
				if (isdigit(*optarg)) {
					listenport = atoi(optarg);
				}
				break;
			case 's':
				serial = optarg;
				break;
			case 'v':
				fprintf(stdout, EBUSD_DAEMON " " EBUSD_VERSION "\n");
				exit(EXIT_SUCCESS);
			case 'h':
			default:
				fprintf(stdout, "Usage: %s [OPTIONS]\n"
								"  -c --confdir      Set the configuration directory. (def: %s)\n"
								"  -f --foreground   Run in foreground.\n"
								"  -L --logfile      Use a specified LOG file. (def: %s)\n"
								"  -l --loglevel     Set log level. (def: INF | INF=0 WAR=1 ERR=2 DBG=3)\n"
								"  -p --pidfile      Use a specified PID file. (def: %s)\n"
								"  -P --listenport   Use a specified listening PORT. (def: %d)\n"
								"  -s --serial       Use a specified SERIAL device. (def: %s)\n"
								"  -v --version      Print version information.\n"
								"  -h --help         Print this message.\n"
								"\n",
								progname,
								EBUSD_CONFDIR,
								EBUSD_LOGFILE,
								EBUSD_PIDFILE,
								SOCKET_PORT,
								SERIAL_DEVICE);
				exit(EXIT_FAILURE);
				break;
		}
	}
}

void
cleanup(int state)
{

	/* close listing tcp socket */
	if (socketfd > 0) {
		close(socketfd);
		log_print_msg(DBG, "socketfd %d closed.", socketfd);
	}

	/* close serial device */
	if (serialfd > 0) {
		serial_close(&serialfd, &oldtermios);
		log_print_msg(DBG, "serialfd %d closed.", serialfd);
	}

	if (!foreground) {

		/* delete PID file */
		if (pidfile_locked) {
			close(pidfd);
			unlink(pidfile);
			log_print_msg(DBG, "PID file %s deleted.", pidfile);
		}

		/* Reset all signal handlers to default */
		signal(SIGCHLD, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		signal(SIGTTOU, SIG_DFL);
		signal(SIGTTIN, SIG_DFL);
		signal(SIGHUP,  SIG_DFL);
		signal(SIGINT,  SIG_DFL);
		signal(SIGTERM, SIG_DFL);

		/* print end message */
		log_print_msg(INF, EBUSD_DAEMON " " EBUSD_VERSION " stopped.");
		syslog(LOG_INFO, EBUSD_DAEMON " " EBUSD_VERSION " stopped.");
	}

	/* close logging system */
	log_close();

	exit(state);
}

void
main_loop(int serialfd, int socketfd)
{
	fd_set listenfds;
	int maxfd;

	unsigned char tmpbuf[SERIAL_BUFSIZE];
	memset(tmpbuf, '\0', sizeof(tmpbuf));

	int j = 0;

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
			log_print_msg(ERR, "select: %s", strerror(errno));
			cleanup(EXIT_FAILURE);
		}

		/* new data from serial port? */
		if (FD_ISSET(serialfd, &readfds)) {

			unsigned char serbuf[SERIAL_BUFSIZE];
			unsigned char *pserbuf = serbuf;
			int i = 0;
			int size = 0;

			size = read(serialfd, serbuf, SERIAL_BUFSIZE);

			while (i < size) {

				if (pserbuf[i] == EBUS_SYN) {
					/* skip 0xAA entries */
					if (j > 0) {

						/* decode ebus messages */
						decode_ebus_msg(tmpbuf, j);

						memset(tmpbuf, '\0', sizeof(tmpbuf));
						j = 0;
					}
				} else {

					/* copy input data into buffer */
					tmpbuf[j] = pserbuf[i];
					j++;
				}
				i++;
			}

		}

		/* new incoming connection at TCP port arrived? */
		if (FD_ISSET(socketfd, &readfds)) {
			ret = socket_accept(socketfd, &readfd);
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
				int tcpbuflen;

				/* get message from client */
				tcpbuflen = sizeof(tcpbuf);
				ret = socket_read(readfd, tcpbuf, &tcpbuflen);

				if (-1 == ret) {
					/* remove dead TCP client FD from listenfds */
					FD_CLR(readfd, &listenfds);
				} else {
					/* just echo message to sender */
					socket_write(readfd, tcpbuf, tcpbuflen);
				}
			}
		}
	}
}

int
main(int argc, char * argv[])
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
		log_print_msg(INF, EBUSD_DAEMON " " EBUSD_VERSION " started.");
		syslog(LOG_INFO, EBUSD_DAEMON " " EBUSD_VERSION " started.");
		daemonize(EBUDS_WORKDIR, pidfile);
	}

	/* open serial device */
	if (serial_open(serial, &serialfd, &oldtermios) < 0) {
		cleanup(EXIT_FAILURE);
	}

	/* open listing tcp socket */
	if (socket_init(listenport, &socketfd) < 0) {
		cleanup(EXIT_FAILURE);
	}

	main_loop(serialfd, socketfd);

	cleanup(EXIT_SUCCESS);

	return 0;
}
