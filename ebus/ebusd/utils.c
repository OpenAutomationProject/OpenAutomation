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

	log_print_msg(DBG, "decode_ebus_msg: %s", buf);
}


int
serial_open(const char *dev, int *fd, struct termios *oldtermios)
{

	struct termios newtermios;

	/* open the serial port */
	if (!fd) {
		log_print_msg(ERR, "open serial device %s - errno: %s", dev, strerror(errno));
		return -1;
	}

	*fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);

	if (*fd == -1) {
		log_print_msg(ERR, "open serial device %s - errno: %s", dev, strerror(errno));
		return -1;
	}

	fcntl(*fd, F_SETFL, 0);

	/* save current settings of serial port */
	if (oldtermios) {
		if (tcgetattr(*fd, oldtermios) == -1) {
			log_print_msg(DBG, "get serial device %s settings - errno: %s", dev, strerror(errno));
			return -1;
		}
	}

	memset(&newtermios, '\0', sizeof(newtermios));

	/* set new settings of serial port */
	newtermios.c_cflag = SERIAL_BAUDRATE | CS8 | CLOCAL | CREAD;
	newtermios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtermios.c_iflag = IGNPAR;
	newtermios.c_oflag &= ~OPOST;
	newtermios.c_cc[VMIN]  = 1;
	newtermios.c_cc[VTIME] = 0;

	tcflush(*fd, TCIFLUSH);

	/* activate new settings of serial port */
	if (tcsetattr(*fd, TCSANOW, &newtermios) == -1) {
		log_print_msg(DBG, "set serial device %s settings - errno: %s", dev, strerror(errno));
		return -1;
	}

	return 0;
}

int
serial_close(int *fd, struct termios *oldtermios)
{

	/* Reset serial device to default settings */
	if (oldtermios) {
		if (tcsetattr(*fd, TCSANOW, oldtermios) == -1) {
			log_print_msg(DBG, "set serial settings - errno: %s", strerror(errno));
			return -1;
		}
	}

	/* Close file descriptor from serial device */
	if (close(*fd) == -1) {
		log_print_msg(DBG, "close serial device - errno: %s", strerror(errno));
		return -1;
	}

	return 0;
}


int
pidfile_open(const char *file, int *fd)
{
	char pid[10];

	/* open pidfile */
	*fd = open(file, O_RDWR|O_CREAT, 0600);

	if (*fd == -1) {
		log_print_msg(ERR, "open PID file %s - errno: %s", file, strerror(errno));
		return -1;
	}

	/* lock file */
	if (lockf(*fd, F_TLOCK, 0) == -1) {
		log_print_msg(ERR, "lock PID file %s - errno: %s", file, strerror(errno));
		close(*fd);
		return -1;
	}

	/* write PID into pidfile */
	sprintf(pid, "%d\n", getpid());

	if (write(*fd, pid, strlen(pid)) == -1) {
		log_print_msg(ERR, "write PID file %s - errno: %s", file, strerror(errno));
		close(*fd);
		return -1;
	}
	return 0;
}

int
pidfile_close(const char *file, int fd)
{
	if (close(fd) == -1) {
		log_print_msg(ERR, "Could not close PID file %s - errno: %s", file, strerror(errno));
		return -1;
	}

	if (unlink(file) == -1) {
		log_print_msg(ERR, "Could not delete PID file %s - errno: %s", file, strerror(errno));
		return -1;
	}
	return 0;
}


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
