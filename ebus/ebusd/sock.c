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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>

#include "log.h"
#include "sock.h"

int
socket_init(int port, int *socketfd)
{
	int ret;
	struct sockaddr_in sock;
	int opt = 1;

	*socketfd = socket(PF_INET, SOCK_STREAM, 0);

	if (socketfd < 0) {
		log_print_msg(ERR, "Could not open socket at %d", port);
		return -1;
	}

	/* prevent "Error Address already in use" error message */
	ret = setsockopt(*socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

	if (ret < 0) {
		log_print_msg(ERR, "Could not set socket options");
		return -1;
	}

	memset((char *) &sock, 0, sizeof(sock));
	sock.sin_family = AF_INET;
	sock.sin_addr.s_addr = htonl(INADDR_ANY);
	sock.sin_port = htons(port);

	ret = bind(*socketfd, (struct sockaddr *) &sock, sizeof(sock));

	if (ret != 0) {
		log_print_msg(ERR, "Could not bind address to socket");
		return -1;
	}

	ret = listen(*socketfd, 5);

	if (ret < 0) {
		log_print_msg(ERR, "Could not set socket to listen mode");
		return -1;
	}

	return 0;
}

int
socket_accept(int listenfd, int *datafd)
{
	struct sockaddr_in sock;
	socklen_t socklen;

	socklen = sizeof(sock);
	*datafd = accept(listenfd, (struct sockaddr *) &sock, &socklen);

	if (datafd < 0) {
		log_print_msg(ERR, "accept: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int
socket_read(int datafd, char msgbuf[], int *msgbuflen)
{
	/* get message */
	*msgbuflen = read(datafd, msgbuf, *msgbuflen);

	if (*msgbuflen <= 0 || (strncmp("quit", msgbuf ,4) == 0)) {
		/* close tcp connection */
		close(datafd);
		return -1;
	}

	return 0;
}

int
socket_write(int datafd, char msgbuf[], int msgbuflen)
{
	int ret;

	ret = write(datafd, msgbuf, msgbuflen);
	if (ret != msgbuflen) {
		log_print_msg(ERR, "write: %s", strerror(errno));
	}

	return 0;
}


