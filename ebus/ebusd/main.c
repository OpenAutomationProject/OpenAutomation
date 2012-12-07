/*
 * Author:  Jax Roland
 * Date:    29.11.2012
 * License: GPLv3
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

#include "log.h"
#include "main.h"


/* global variables */
const char *progname;

static int pfp; /* pidfile pointer */

static int loglevel = EBUSD_LOGLEVEL;
static int foreground = EBUSD_FOREGROUND;

static const char *confdir = EBUSD_CONFDIR;
static const char *pidfile = EBUSD_PIDFILE;
static const char *logfile = EBUSD_LOGFILE;


void
signal_handler(int sig)
{
	switch(sig)
	{
		case SIGHUP:
			log_print_msg(INF,  "Received SIGHUP signal.");
			break;
		case SIGINT:
		case SIGTERM:
			log_print_msg(INF,  "Daemon exiting");
			close(pfp);
			cleanup(EXIT_SUCCESS);
			break;
		default:
			log_print_msg(INF,  "Unhandled signal %s", strsignal(sig));
			break;
	}
}

int
lock_pidfile()
{
	char pid_str[10];

	/* Ensure only one copy */
	pfp = open(pidfile, O_RDWR|O_CREAT, 0600);

	if (pfp == -1) {
		log_print_msg(ERR, "Could not open PID file %s, exiting", pidfile);
		return -1;
	}

	/* Try to lock file */
	if (lockf(pfp, F_TLOCK, 0) == -1) {
		log_print_msg(ERR, "Could not lock PID file %s, exiting", pidfile);
		close(pfp);
		return -1;
	}

	/* Get PID and write into file */
	sprintf(pid_str, "%d\n", getpid());

	if (write(pfp, pid_str, strlen(pid_str)) == -1) {
		log_print_msg(ERR, "Could not write PID into file %s, exiting", pidfile);
		close(pfp);
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
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* Change the current working directory.  This prevents the current
	   directory from being locked; hence not being able to remove it. */
	if (chdir(workdir) < 0) {
		/* Log any failure here */
		log_print_msg(ERR, "chdir(\"/\"): %s", strerror(errno));
		cleanup(EXIT_FAILURE);
	}

	/* write pidfile and try to lock it */
	if (lock_pidfile() < 0) {
		cleanup(EXIT_FAILURE);
	}
	else {
		log_print_msg(DBG, "PID file %s created.", pidfile);
	}

    /* Cancel certain signals */
    signal(SIGCHLD, SIG_DFL); /* A child process dies */
    signal(SIGTSTP, SIG_IGN); /* Various TTY signals */
    signal(SIGTTOU, SIG_IGN); /* Ignore TTY background writes */
    signal(SIGTTIN, SIG_IGN); /* Ignore TTY background reads */

    /* Trap signals that we expect to receive */
    signal(SIGHUP,  signal_handler);
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

}

static int
cmdline_read(int *argc, char ***argv)
{
	struct option opts[] = {
//		{"confdir",    required_argument, NULL, 'c'},
		{"logfile",    required_argument, NULL, 'L'},
		{"loglevel",   required_argument, NULL, 'l'},
		{"foreground", no_argument,       NULL, 'f'},
		{"pidfile",    required_argument, NULL, 'p'},
		{"version",    no_argument,       NULL, 'v'},
		{"help",       no_argument,       NULL, 'h'},
		{NULL,         no_argument,       NULL,  0 },
	};

	const char *opts_help[] = {
//		"Set the configuration directory.",       /* confdir */
		"Use a specified LOG file.",              /* logfile */
		"Set log level",                          /* loglevel */
		"Run in foreground.",                     /* foreground */
		"Use a specified PID file.",              /* pidfile */
		"Print version information.",             /* version */
		"Print this message.",                    /* help */
	};

	struct option *opt;
	const char **hlp;
	int max, size;

	for (;;) {
		int i;
		i = getopt_long(*argc, *argv, "c:L:l:fp:vh", opts, NULL);
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
					}
					else {
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
			case 'v':
				fprintf(stdout, EBUSD_DAEMON " " EBUSD_VERSION "\n");
				exit(EXIT_SUCCESS);
			case 'h':
			default:
				fprintf(stdout, "Usage: %s [OPTIONS]\n", progname);
				max = 0;

				for (opt = opts; opt->name; opt++) {
					size = strlen(opt->name);
					if (size > max) {
						max = size;
					}
				}

				for (opt = opts, hlp = opts_help; opt->name; opt++, hlp++) {
					fprintf(stdout, "  -%c, --%s", opt->val, opt->name);
					size = strlen(opt->name);

					for (; size < max; size++) {
						fprintf(stdout, " ");
					}

					fprintf(stdout, "  %s\n", *hlp);
				}

				exit(EXIT_FAILURE);
				break;
		}
	}

	*argc -= optind;
	*argv += optind;

	return 0;
}

void
cleanup(int state)
{
	if (!foreground) {
		/* delete PID file */
		unlink(pidfile);
		log_print_msg(DBG, "PID file %s deleted.", pidfile);

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

int
main(int argc, char * argv[])
{
	/* set progname */
	progname = (const char *)strrchr(argv[0], '/');
	progname = progname ? (progname + 1) : argv[0];

	/* read commandline  */
	cmdline_read(&argc, &argv);

	/* open log */
	log_set_level(loglevel);
	log_open(logfile, foreground);

	/* become daemon */
	if (!foreground) {
		log_print_msg(INF, EBUSD_DAEMON " " EBUSD_VERSION " started.");
		syslog(LOG_INFO, EBUSD_DAEMON " " EBUSD_VERSION " started.");
		daemonize(EBUDS_WORKDIR, pidfile);
	}

	while (1)
	{
		log_print_msg(INF, "daemon says hello");
		sleep(1);
	}

	cleanup(EXIT_SUCCESS);

}
