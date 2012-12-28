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

#ifndef UTILS_H_
#define UTILS_H_

#define SERIAL_DEVICE		"/dev/ttyUSB0"
#define SERIAL_BAUDRATE		B2400
#define SERIAL_BUFSIZE		1024

#define SOCKET_PORT			8888
#define SOCKET_BUFSIZE		1024


void decode_ebus_msg(unsigned char *data, int size);

int serial_open(const char *dev, int *fd, struct termios *oldtermios);
int serial_close(int *fd, struct termios *oldtermios);

int pidfile_open(const char *file, int *fd);
int pidfile_close(const char *file, int fd);

int socket_init(int port, int *socketfd);
int socket_accept(int listenfd, int *datafd);
int socket_read(int datafd, char msgbuf[], int *msgbuflen);
int socket_write(int datafd, char msgbuf[], int msgbuflen);

#endif /* UTILS_H_ */
