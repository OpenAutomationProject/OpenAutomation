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

#ifndef UTILS_H_
#define UTILS_H_

#define NUMSTR2(s) #s
#define NUMSTR(s) NUMSTR2(s)

/* for log */
#define L_INF 0x01
#define L_NOT 0x02
#define L_WAR 0x04
#define L_ERR 0x08
#define L_DBG 0x10
#define L_EBH 0x20
#define L_EBS 0x40
#define L_NET 0x80

#define err_if(exp) \
	if(exp) { log_print(L_ERR, "%s: %d: %s: Error %s", \
		__FILE__, __LINE__, __PRETTY_FUNCTION__, strerror(errno));\
	}

#define err_ret_if(exp, ret) \
	if(exp) { log_print(L_ERR, "%s: %d: %s: Error %s", \
		__FILE__, __LINE__, __PRETTY_FUNCTION__, strerror(errno));\
		return(ret); \
	}

enum enum_number {UNSET = -1, NO, YES};


void print_ebus_msg(const unsigned char *buf, int buflen);



#define CFG_LINELEN   256

enum enum_config {STR, BOL, NUM};

struct config {
	char *key;
	int type;
	void *tgt;
	char *info;
};

void cfg_print(struct config *cfg, int len);
int cfgfile_set_param(char *param, struct config *cfg, int len);
int cfgfile_read(const char *file, struct config *cfg, int len);

int pidfile_open(const char *file, int *fd);
int pidfile_close(const char *file, int fd);

int rawfile_open(const char *file);
int rawfile_close();
int rawfile_write(const unsigned char *buf,  int buflen);

#define SOCKET_PORT      8888
#define SOCKET_BUFSIZE   1024

int socket_open(int *fd, int port);
int socket_close(int fd);
int socket_client_accept(int listenfd, int *datafd);
int socket_client_read(int fd, char *buf, int *buflen);
int socket_client_write(int fd, const char *buf, int buflen);

#endif /* UTILS_H_ */
