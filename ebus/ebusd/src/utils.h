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

#define YES 1
#define NO  0

#define SOCKET_PORT			8888
#define SOCKET_BUFSIZE		1024

void debug_ebus_msg(unsigned char buf[], int buflen, int nosyn);

int serial_ebus_get_msg(int fd, unsigned char buf[], int *buflen, int rawdump, int skipsyn);

int dumpfile_open(const char *file);
int dumpfile_close();
int dumpfile_write(unsigned char buf[],  int buflen);

int pidfile_open(const char *file, int *fd);
int pidfile_close(const char *file, int fd);

int socket_open(int *fd, int port);
int socket_close(int fd);
int socket_client_accept(int listenfd, int *datafd);
int socket_client_read(int fd, char buf[], int *buflen);
int socket_client_write(int fd, char buf[], int buflen);

#endif /* UTILS_H_ */
