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
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>

#include "log.h"

const char *log_level_txt[] = {"INF","WAR","ERR","DBG"};

static int log_level = INF;
static FILE *log_file_fp = NULL;

char * log_get_time(char *time);


void
log_set_file(FILE *fp)
{
	if(log_file_fp)
		fclose(log_file_fp);

	log_file_fp = fp;
}

void
log_set_level(int loglevel)
{
	switch (loglevel) {
	case INF:
		log_level = INF;
		setlogmask(LOG_UPTO(LOG_ERR));
		break;
	case WAR:
		log_level = WAR;
		setlogmask(LOG_UPTO(LOG_WARNING));
		break;
	case ERR:
		log_level = ERR;
		setlogmask(LOG_UPTO(LOG_INFO));
		break;
	case DBG:
		log_level = DBG;
		setlogmask(LOG_UPTO(LOG_DEBUG));
		break;
	default:
		log_level = INF;
		setlogmask(LOG_UPTO(LOG_ERR));
		break;
	}
}

int
log_open(const char *logfile, int foreground)
{
	FILE *fp = NULL;

	if (foreground) {
		log_set_file(stdout);
	} else {
		if (logfile) {
			if (!(fp = fopen(logfile, "a+"))) {
				fprintf(stderr, "error opening logfile %s.\n", logfile);
				return -1;
			}

			log_set_file(fp);
		}
	}

	openlog(NULL, LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);

	return 0;
}

void
log_close()
{
	if (log_file_fp) {
		fflush(log_file_fp);
		fclose(log_file_fp);
	}

	closelog();
}

char *
log_get_time(char *time)
{
	struct timeval tv;
	struct tm *tm;

	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);

    sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
			tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec, (int)tv.tv_usec/1000);

    return time;
}

void
log_print_msg(int loglevel, const char *logtxt, ...)
{
	char time[24];
	char buf[512];
	int priority = ERR;
	va_list ap;

	va_start(ap, logtxt);
	vsprintf(buf, logtxt, ap);

	if (log_file_fp) {
		if (loglevel <= log_level) {
			fprintf(log_file_fp, "%s [%s] %s\n", log_get_time(time), log_level_txt[loglevel], buf);
			fflush(log_file_fp);
		}
	} else {
		switch (loglevel) {
		case INF:
			priority = LOG_ERR;
			break;
		case WAR:
			priority = LOG_WARNING;
			break;
		case ERR:
			priority = LOG_INFO;
			break;
		case DBG:
			priority = LOG_DEBUG;
			break;
		default:
			priority = LOG_ERR;
			break;
		}
		syslog(priority, "[%s] %s\n", log_level_txt[loglevel], logtxt);
	}

	va_end(ap);
}

