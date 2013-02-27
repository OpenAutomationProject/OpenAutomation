/*
 * Copyright (C) Roland Jax 2012-2013 <roland.jax@liwest.at>
 * crc calculations from http://www.mikrocontroller.net/topic/75698
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
#include <getopt.h>
#include <dirent.h>
#include <ctype.h>

#include "log.h"
#include "ebus-bus.h"

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

	

	/* test */
	//~ const unsigned char tmp[] = {'\x10','\xfe','\xb5','\x05','\x03','\x4a','\x01','\x00','\xf4'};
	//~ const unsigned char tmp[] = {'\x10','\xfe','\xb5','\x05','\x03','\x4a','\x01','\xa9','\x01','\xb8'};
	
	//~ const unsigned char tmp[] = {'\x10','\x23','\xb5','\x04','\x01','\x31','\xf6','\x00'};
	//~ const unsigned char tmp[] = {'\x10','\x23','\xb5','\x04','\x01','\x31','\xf6','\xff'};

	//~ const unsigned char tmp[] = {'\x10','\x08','\xb5','\x09','\x03','\x29','\x01','\x00','\x23',
	                             //~ '\x00','\x05','\x01','\x00','\x1d','\x02','\x00','\x60','\x00'};

	//~ const unsigned char tmp[] = {'\x10','\x08','\xb5','\x09','\x03','\x29','\x01','\x00','\x23',
	                             //~ '\x00','\x05','\x01','\xa9','\x01','\x1d','\x02','\x00','\xec','\x00'};

	//~ const unsigned char tmp[] = {'\x10','\x08','\xb5','\x10','\x09','\x00','\x02','\x40','\x00',
				     //~ '\x00','\x00','\x00','\x00','\x02','\x15','\x00','\x00','\x00','\x00'};

	//~ const unsigned char tmp[] = {'\x10','\x08','\xb5','\x11','\x02','\x03','\x00','\x00','\x27','\x02','\x07',
				     //~ '\x04','\x2c','\x00','\x10','\xb5','\x01','\x02','\x00','\x00','\x00','\x00',
				     //~ '\xc8','\xca','\x00'};


	//~ const unsigned char tmp[] = {'\x01','\x02','\x03','\x04','\x04','\x05','\x06','\x07','\x08','\x09',
				     //~ '\x11','\x12','\x13','\x14','\x14','\x15','\x16','\x17','\x18','\x19',
				     //~ '\x21','\x22','\x23','\x24','\x24','\x25','\x26','\x27','\x28','\x29',
				     //~ '\x31','\x32','\x33','\x34','\x34','\x35','\x36','\x37','\x38','\x39',
				     //~ '\x41','\x42','\x43','\x44','\x44','\x45','\x46','\x47','\x48','\x49',
				     //~ '\x51','\x52','\x53','\x54','\x54','\x55','\x56','\x57','\x58','\x59',
				     //~ '\x61','\x62','\x63','\x64','\x64','\x65','\x66','\x67','\x68','\x69'};

	//~ const unsigned char tmp[] = {'\x10','\x03','\x05','\x07','\x09','\xbb','\x06','\x1b','\x02','\x00',
				     //~ '\x80','\xff','\x6e','\xff','\xf2','\x00'};				     
	//~ 
	//~ const int tmplen = sizeof(tmp) / sizeof(unsigned char);
//~ 
	//~ eb_raw_print_hex(tmp, tmplen);
	//~ eb_cyc_data_process(tmp, tmplen);

	eb_cmd_dir_free();

	return 0;
}
