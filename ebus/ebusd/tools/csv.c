#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <dirent.h>
#include <ctype.h>

#include "log.h"
#include "ebus-bus.h"

//~ #define YES  1
//~ #define NO   0

/* global variables */
const char *progname;

static const char *cfgdir = NULL;
static int all = NO;
static int cyc = NO;
static int detail = NO;
static int get = NO;
static int set = NO;
static char loglevel[] = {"ALL"};


void
usage()
{
	fprintf(stdout, "\nUsage: %s [OPTION] cfgdir\n\n"
	"  -a --all      print ALL\n"
	"  -c --cyc      print CYC\n"
	"  -d --detail   print DETAIL\n"	
	"  -g --get      print GET\n"
	"  -s --set      print SET\n"
	"  -h --help     print this message.\n"
	"\n",
	progname);
}

void
cmdline(int *argc, char ***argv)
{
	static struct option opts[] = {
		{"all",        no_argument,       NULL, 'a'},
		{"cyc",        no_argument,       NULL, 'c'},
		{"detail",     no_argument,       NULL, 'd'},		
		{"get",        no_argument,       NULL, 'g'},
		{"set",        no_argument,       NULL, 's'},
		{"help",       no_argument,       NULL, 'h'},
		{NULL,         no_argument,       NULL,  0 },
	};

	for (;;) {
		int i;

		i = getopt_long(*argc, *argv, "acdgsh", opts, NULL);

		if (i == -1) {
			*argc = optind;			
			break;
		}

		switch (i) {
		case 'a':
			cyc = YES;
			get = YES;
			set = YES;
			break;			
		case 'c':
			cyc = YES;
			break;
		case 'd':
			detail = YES;
			break;			
		case 'g':
			get = YES;
			break;	
		case 's':
			set = YES;
			break;			
		case 'h':
		default:
			usage();
			exit(EXIT_FAILURE);
			break;
		}
	}
}



int
main(int argc, char *argv[])
{
	
	/* set progname */
	progname = (const char *)strrchr(argv[0], '/');
	progname = progname ? (progname + 1) : argv[0];

	cmdline(&argc, &argv);

	cfgdir = argv[argc];

	/* open log */
	log_level(loglevel);
	log_open("", 1);

	if (cfgdir)
		eb_cmd_dir_read(cfgdir, ".csv");
	else
		usage();

	if (cyc)
		eb_cmd_print("cyc", all, detail);

	if (get)
		eb_cmd_print("get", all, detail);
		
	if (set)
		eb_cmd_print("set", all, detail);		

	eb_cmd_dir_free();

	return 0;
}
