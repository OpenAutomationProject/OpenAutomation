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

#ifndef LOG_H_
#define LOG_H_

/*
 * INF = 0 - normal messages
 * WAR = 1 - warnings
 * ERR = 2 - errors
 * DBG = 3 - for debugging purpose
 */

enum enum_log {INF, WAR, ERR, DBG};

//~ #define INF 0
//~ #define WAR 1
//~ #define ERR 2
//~ #define DBG 3

#define err_if(exp) \
	if(exp) { log_print_msg(ERR, "%s: %d: %s: Error %s", \
		__FILE__, __LINE__, __PRETTY_FUNCTION__, strerror(errno));\
	}

#define err_ret_if(exp, ret) \
	if(exp) { log_print_msg(ERR, "%s: %d: %s: Error %s", \
		__FILE__, __LINE__, __PRETTY_FUNCTION__, strerror(errno));\
		return(ret); \
	}

void log_set_file(FILE *fp);
void log_set_level(int loglevel);

int log_open(const char *logfile, int foreground);
void log_close();

void log_print_msg(int loglevel, const char *logtxt, ...);

#endif /* LOG_H_ */
