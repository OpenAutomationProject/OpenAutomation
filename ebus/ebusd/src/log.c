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
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>

#include "log.h"

static unsigned char _loglvl = L_NUL;
static const char ***_logtxt = NULL;
static int _logtxtlen = 0;

static FILE *_logfp = NULL;


char * log_time(char *time);
char * log_txt(unsigned char lvl);

void
log_file(FILE *fp)
{
	if(_logfp)
		fclose(_logfp);

	_logfp = fp;
}

void
log_level(char *lvl, const char *txt[], int len)
{
	unsigned char tmp;
	int i;
	char *par;
	
	_loglvl = L_NUL;
	
	par = strtok(lvl, ", ");
	while (par) {
		if (strncasecmp(par, "ALL", 3) == 0) {
			_loglvl = L_ALL;
			break;
		}	
		
		tmp = 0x01;
		for (i = 0; i < len; i++) {
			if (strncasecmp(par, txt[i], 3) == 0) {
				_loglvl |= (tmp << i);
				break;
			}
		}
		par = strtok(NULL, ", ");
	}
}

int
log_open(const char *file, int foreground, const char ***txt, int len)
{
	FILE *fp = NULL;
	
	_logtxt = txt;
	_logtxtlen = len;

	if (foreground) {
		log_file(stdout);
	} else {
		if (file) {
			if (!(fp = fopen(file, "a+"))) {
				fprintf(stderr, "can't open logfile: %s\n", file);

				return -1;
			}

			log_file(fp);
		}
	}

	openlog(NULL, LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);

	return 0;
}

void
log_close()
{
	if (_logfp) {
		fflush(_logfp);
		fclose(_logfp);
	}

	closelog();
}

char *
log_time(char *time)
{
	struct timeval tv;
	struct tm *tm;

	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);

	sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec/1000);

	return time;
}

char *
log_txt(unsigned char lvl)
{
	char *type = NULL;
	if (_logtxtlen > 0) {
		unsigned char tmp;
		int i;
		
		tmp = 0x01;
		for (i = 0; i < _logtxtlen; i++) {
			if (lvl == L_ALL) {
				type = "ALL";
				break;
			}	
					
			if ((lvl & (tmp << i)) != 0x00 &&
				(_loglvl & (tmp << i)) != 0x00) {
				type = (char *)_logtxt[i];
				break;
			}
		}
	}
	return type;	
}

void
log_print(unsigned char lvl, const char *txt, ...)
{
	char time[24];
	char buf[512];
	va_list ap;

	va_start(ap, txt);
	vsprintf(buf, txt, ap);

	if ((_loglvl & lvl) != 0x00) {
		if (_logfp) {
			fprintf(_logfp, "%s [0x%02x %s] %s\n",
					log_time(time), lvl , log_txt(lvl), buf);								
			fflush(_logfp);

		} else {
			syslog(LOG_INFO, "[0x%02x %s] %s\n",
							lvl, log_txt(lvl), buf);
		}
	}

	va_end(ap);
}

