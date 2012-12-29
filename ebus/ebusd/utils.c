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
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>

#include "log.h"
#include "ebus.h"
#include "utils.h"

void
decode_ebus_msg(unsigned char *data, int size)
{
	int k = 0;
	char buf[SERIAL_BUFSIZE];
	char tmp[4];

	memset(tmp, '\0', sizeof(tmp));
	memset(buf, '\0', sizeof(buf));

	for (k = 0; k < size; k++) {
		sprintf(tmp, " %02x", data[k]);
		strncat(buf, tmp, 3);
	}

	log_print_msg(DBG, "%s", buf);
}


int
serial_open(const char *dev, int *fd, struct termios *oldtermios)
{
	int ret;
	struct termios newtermios;

	*fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	err_ret_if(*fd < 0, -1);

	ret = fcntl(*fd, F_SETFL, 0);
	err_ret_if(ret < 0, -1);

	/* save current settings of serial port */
	ret = tcgetattr(*fd, oldtermios);
	err_ret_if(ret < 0, -1);

	memset(&newtermios, '\0', sizeof(newtermios));

	/* set new settings of serial port */
	newtermios.c_cflag = SERIAL_BAUDRATE | CS8 | CLOCAL | CREAD;
	newtermios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtermios.c_iflag = IGNPAR;
	newtermios.c_oflag &= ~OPOST;
	newtermios.c_cc[VMIN]  = 1;
	newtermios.c_cc[VTIME] = 0;

	ret = tcflush(*fd, TCIFLUSH);
	err_ret_if(ret < 0, -1);

	/* activate new settings of serial port */
	ret = tcsetattr(*fd, TCSANOW, &newtermios);
	err_ret_if(ret < 0, -1);

	return 0;
}

int
serial_close(int *fd, struct termios *oldtermios)
{
	int ret;

	/* activate old settings of serial port */
	ret = tcsetattr(*fd, TCSANOW, oldtermios);
	err_ret_if(ret < 0, -1);

	/* Close file descriptor from serial device */
	ret = close(*fd);
	err_ret_if(ret < 0, -1);

	return 0;
}

int
serial_read(int fd, unsigned char buf[], int *buflen, unsigned char tmpbuf[], int *tmppos)
{
	int maxlen, i;

	maxlen = *buflen;

	*buflen = read(fd, buf, *buflen);
	err_ret_if(*buflen < 0 || *buflen > maxlen, -1);

	i = 0;
	while (i < *buflen) {

		if (buf[i] == EBUS_SYN) {
			/* skip 0xAA entries */
			if (*tmppos > 0) {
				return 1;
			}
		} else {

			/* copy input data into buffer */
			tmpbuf[*tmppos] = buf[i];
			*tmppos = *tmppos + 1;
		}
		i++;
	}

	return 0;
}


int
pidfile_open(const char *file, int *fd)
{
	int ret;
	char pid[10];

	*fd = open(file, O_RDWR|O_CREAT, 0600);
	err_ret_if(*fd < 0, -1);

	ret = lockf(*fd, F_TLOCK, 0);
	err_ret_if(ret < 0, -1);

	sprintf(pid, "%d\n", getpid());
	ret = write(*fd, pid, strlen(pid));
	err_ret_if(ret < 0, -1);

	return 0;
}

int
pidfile_close(const char *file, int fd)
{
	int ret;

	ret = close(fd);
	err_ret_if(ret < 0, -1);

	ret = unlink(file);
	err_ret_if(ret < 0, -1);

	return 0;
}


int
socket_open(int port, int *fd)
{
	int ret, opt;
	struct sockaddr_in sock;

	*fd = socket(PF_INET, SOCK_STREAM, 0);
	err_ret_if(fd < 0, -1);

	//todo: verify if this realy work
	/* prevent "Error Address already in use" error message */
	opt = 1;
	ret = setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
	err_ret_if(ret < 0, -1);

	memset((char *) &sock, 0, sizeof(sock));
	sock.sin_family = AF_INET;
	sock.sin_addr.s_addr = htonl(INADDR_ANY);
	sock.sin_port = htons(port);

	ret = bind(*fd, (struct sockaddr *) &sock, sizeof(sock));
	err_ret_if(ret < 0, -1);

	ret = listen(*fd, 5);
	err_ret_if(ret < 0, -1);

	return 0;
}

int
socket_close(int fd)
{
	int ret;

	ret = close(fd);
	err_ret_if(ret < 0, -1);

	return 0;
}

int
socket_accept(int listenfd, int *datafd)
{
	struct sockaddr_in sock;
	socklen_t socklen;

	socklen = sizeof(sock);

	*datafd = accept(listenfd, (struct sockaddr *) &sock, &socklen);
	err_ret_if(*datafd < 0, -1);

	return 0;
}

int
socket_read(int fd, char buf[], int *buflen)
{
	*buflen = read(fd, buf, *buflen);
	err_ret_if(*buflen < 0, -1);

	if (strncmp("quit", buf ,4) == 0) {
		/* close tcp connection */
		close(fd);
		return -1;
	}

	return 0;
}

int
socket_write(int fd, char buf[], int buflen)
{
	int ret;

	ret = write(fd, buf, buflen);
	err_ret_if(ret < 0, -1);

	return 0;
}
