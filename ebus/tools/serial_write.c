/*
 * Copyright (C) Roland Jax 2013 <roland.jax@liwest.at>
 *
 * This file is part of serial_write.
 *
 * serial_write is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * serial_write is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with serial_write. If not, see http://www.gnu.org/licenses/.
 */

/* gcc -Wall -o serial_write serial_write.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

#define CRC_YES	1
#define CRC_NO	0

#define EBUS_SYN 	0xAA
#define EBUS_ACK	0x00
#define	EBUS_NAK	0xFF

#define EBUS_MSG_MASTER_MASTER	1
#define EBUS_MSG_MASTER_SLAVE	2
#define EBUS_MSG_BROADCAST		3

#define SERIAL_DEVICE 			"/dev/ttyUSB0"
#define SERIAL_BAUDRATE 		B2400
#define SERIAL_BUFSIZE_READ		1024
#define SERIAL_BUFSIZE_WRITE	20

#define err_ret_if(exp, ret) \
	if(exp) { fprintf(stdout, "%s: %d: %s: Error %s\n", \
			__FILE__, __LINE__, __PRETTY_FUNCTION__, strerror(errno));\
			return(ret); \
	}
	
#define warn_ret_if(exp, msg, ret) \
	if(exp) { fprintf(stdout, "%s: %d: %s: Warning %s\n", \
			__FILE__, __LINE__, __PRETTY_FUNCTION__, msg);\
			return(ret); \
	}

static int serialfd = -1;
static struct termios oldtio;
static const char *serial = SERIAL_DEVICE;
static const char *progname;

void
print_msg(const char pre[], const unsigned char buf[], const int buflen)
{
	int i;
	
	fprintf(stdout, "%s ", pre);
	for (i = 0; i < buflen; i++) {
		fprintf(stdout, "%02x ", buf[i]);
	}
	fprintf(stdout, "\n");
}

unsigned char
calc_crc_byte(unsigned char byte, unsigned char init_crc)
{
	unsigned char crc;
	unsigned char polynom;
	int i;

	crc = init_crc;

	for (i = 0; i < 8; i++) {

		if (crc & 0x80) {
			polynom = (unsigned char) 0x9B;
		} else {
			polynom = (unsigned char) 0;
		}

		crc = (unsigned char) ((crc & ~0x80) << 1);

		if (byte & 0x80) {
			crc = (unsigned char) (crc | 1);
		}

		crc = (unsigned char) (crc ^ polynom);
		byte = (unsigned char) (byte << 1);
	}
	return crc;
}

unsigned char
calc_crc(unsigned char *bytes, int size)
{
	int i;
	unsigned char crc = 0;

	for( i = 0 ; i < size ; i++, bytes++ ) {
		crc = calc_crc_byte(*bytes, crc);
	}
	return crc;
}

int
serial_open(const char *dev, int *fd, struct termios *olddio)
{
	int ret;
	struct termios newtio;

	*fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	err_ret_if(*fd < 0, -1);

	ret = fcntl(*fd, F_SETFL, 0);
	err_ret_if(ret < 0, -1);

	/* save current settings of serial port */
	ret = tcgetattr(*fd, olddio);
	err_ret_if(ret < 0, -1);

	memset(&newtio, '\0', sizeof(newtio));

	/* set new settings of serial port */
	newtio.c_cflag = SERIAL_BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag &= ~OPOST;
	newtio.c_cc[VMIN]  = 1;
	newtio.c_cc[VTIME] = 0;

	ret = tcflush(*fd, TCIFLUSH);
	err_ret_if(ret < 0, -1);

	/* activate new settings of serial port */
	ret = tcsetattr(*fd, TCSANOW, &newtio);
	err_ret_if(ret < 0, -1);

	return 0;
}

int
serial_close(int fd, struct termios *oldtio)
{
	int ret;

	/* activate old settings of serial port */
	ret = tcsetattr(fd, TCSANOW, oldtio);
	err_ret_if(ret < 0, -1);

	/* Close file descriptor from serial device */
	ret = close(fd);
	err_ret_if(ret < 0, -1);

	return 0;
}

int
serial_ebus_wait_syn(int fd)
{
	unsigned char buf[SERIAL_BUFSIZE_READ];
	int buflen, maxlen, i, found;
	
	maxlen = sizeof(SERIAL_BUFSIZE_READ);
	
	memset(buf, '\0', sizeof(buf));
	buflen = sizeof(buf);
	found = 0;
	
	/* do until SYN read*/
	do {
		buflen = read(fd, buf, buflen);
		//err_ret_if(inlen < 0 || inlen > maxlen, -1);
		
		if (buflen > 0) {
			i = 0;
			while (i < buflen) {
				if (buf[i] == EBUS_SYN && (i + 1) == buflen) {
					found = 1;
					break;
				}
				i++;
			}
		}
	} while (found == 0);
	
	fprintf(stdout, "last SYN found\n");
	return 0;
}

int
serial_ebus_write(int fd, unsigned char buf[], int *buflen, int need_crc, int type)
{
	unsigned char crc, ret;
	
	if (need_crc == CRC_YES) {
		crc = calc_crc(buf, *buflen);
		buf[*buflen] = crc;
		*buflen += 1;
	}
	
	if (type == EBUS_MSG_BROADCAST) {
		buf[*buflen] = EBUS_SYN;
		*buflen += 1;
	}
	
	print_msg("<<<", buf, *buflen);
	
	/* write msg to ebus */
	ret = write(fd, buf, *buflen);
	err_ret_if(ret < 0, -1);
	
	return 0;
}

int
serial_ebus_read(int fd, unsigned char buf[], int *buflen, int type)
{
	unsigned char in[SERIAL_BUFSIZE_READ], tmp[SERIAL_BUFSIZE_WRITE], crc;
	int inlen, tmplen, i, j, found;
	
	memset(in, '\0', sizeof(in));
	memset(tmp, '\0', sizeof(tmp));
	
	inlen = sizeof(in);
	tmplen = 0;
	
	found = 0;
	j = 0;
	
	do {
		inlen = read(fd, in, inlen);
		
		if (inlen > 0) {
			i = 0;
			while (i < inlen) {

				/*
				 * compare input with send - is this possible?
				 * or SYN sign received.
				 */
				if ((in[i] != buf[j] && j < *buflen) ||
					in[i] == EBUS_SYN) {
					found = 2;
					break;
				}
				
				/* copy only slaves answer into tmp */
				if (j >= *buflen) {
					tmp[tmplen] = in[i];
					
					/*
					 *  break loop if
					 * - slave answer is ff
					 * - end of message is reached
					 * - exit if type is Master Master
					 */
					if (tmp[0] == EBUS_NAK || /* */
						(tmplen == (2 + tmp[1]) && tmplen >= 1) ||
						type == EBUS_MSG_MASTER_MASTER) {
						tmplen++;
						found = 1;
						break;
					}
					tmplen++;							
				}
				
				i++;
				j++;
			}
		}
	} while (found == 0);
	
	print_msg(">>>", tmp, tmplen);
	
	if (found == 2) {
		warn_ret_if(1, "received and sent message are different or SYN received.", -1);
	}
	
	/* prepare answer to slave */
	memset(buf, '\0', sizeof(buf));
	
	if (type == EBUS_MSG_MASTER_SLAVE) {
		/* set ACK */
		buf[0] = EBUS_ACK;
		
		/* slave sent ACK */
		if (tmp[0] == EBUS_ACK) {
			crc = calc_crc(&tmp[1], tmplen - 2);

			/* CRC from slave NOT OK */
			if (tmp[tmplen-1] != crc) {	
				buf[0] = EBUS_NAK;
			}
		}
			
		buf[1] = EBUS_SYN;
		*buflen = 2;
		
	} else {
		buf[0] = EBUS_SYN;
		*buflen = 1;
	}

	return 0;
}

int
serial_ebus_send_msg(int fd, unsigned char buf[], int buflen, int type)
{
	int ret;

	/* wait next SYN */
	ret = serial_ebus_wait_syn(fd);
	err_ret_if(ret < 0, -1);
	
	/* send message */
	ret = serial_ebus_write(fd, buf, &buflen, CRC_YES, type);
	err_ret_if(ret < 0, -1);
	
	if (type < EBUS_MSG_BROADCAST) {
		/* recive answer from slave */
		ret = serial_ebus_read(fd, buf, &buflen, type);
		err_ret_if(ret < 0, -1);

		/* send answer to slave */
		ret = serial_ebus_write(fd, buf, &buflen, CRC_NO, type);
		err_ret_if(ret < 0, -1);
	}

	/* wait next SYN */
	ret = serial_ebus_wait_syn(fd);
	err_ret_if(ret < 0, -1);
	
	return 0;
}

int
htoi(char s[])
{
    int i = 0;
    int n = 0;
 
    // Remove "0x" or "0X"
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        i = 2;
	}
 
    while (s[i] != '\0') {
        int t;
 
        if (s[i] >= 'A' && s[i] <= 'F') {
            t = s[i] - 'A' + 10;
		} else if (s[i] >= 'a' && s[i] <= 'f') {
            t = s[i] - 'a' + 10;
        } else if (s[i] >= '0' && s[i] <= '9') {
            t = s[i] - '0';
        } else if (s[i] == ' ') {
			return -1;
        } else {
            return n;
		}
 
        n = 16 * n + t;
        ++i;
    }
 
    return n;
}

void
cmdline(int *argc, char ***argv)
{
	static struct option opts[] = {
		{"serial",     required_argument, NULL, 's'},
		{"help",       no_argument,       NULL, 'h'},
		{NULL,         no_argument,       NULL,  0 },
	};

	for (;;) {
		int i;
		i = getopt_long(*argc, *argv, "s:h", opts, NULL);
		if (i == -1) {
			break;
		}
		switch (i) {
			case 's':
				serial = optarg;
				break;
			case 'h':
			default:
				fprintf(stdout, "\nUsage: %s [OPTIONS]\n"
								"  -s --serial       use a specified serial device. (%s)\n"
								"  -h --help         print this message.\n"
								"\n",
								progname,
								SERIAL_DEVICE);
				exit(EXIT_FAILURE);
				break;
		}
	}
}

int
main(int argc, char *argv[])
{
	int i, j, k, end, in[20], ret;
	char byte;
	unsigned char msg[SERIAL_BUFSIZE_WRITE];
	
	progname = (const char *)strrchr(argv[0], '/');
	progname = progname ? (progname + 1) : argv[0];

	cmdline(&argc, &argv);
	
	ret = serial_open(serial, &serialfd, &oldtio);
	
	if (ret == 0) {
		fprintf(stdout, "serial device %s successfully opened.\n", serial);
		
		end = 0;
		do {
			i = 0;
			printf("Input: ");
			while ((byte = fgetc(stdin)) != EOF) {
				
				if (byte == '\n') {
					break;
				}
				
				if (byte == 'q') {
					end = 1;
					break;
				}
					
				if (i < sizeof(in)) {
					ret = htoi(&byte);
					if (ret != -1) {
						in[i] = ret;
						i++;
					}
				} else {
					break;
				}
			}		
			
			if (!end) {
				memset(msg, '\0', sizeof(msg));
				
				for(j = 0, k = 0; j < i; j += 2, k++) {
					msg[k] = (unsigned char) (in[j]*16 + in[j+1]);
				}
				
				fprintf(stdout, "\nin: ");
				for (i = 0; i < k; i++) {
					fprintf(stdout, "%02x ", msg[i]);
				}
				fprintf(stdout, "\n\n");
				
				ret = serial_ebus_send_msg(serialfd, msg, i, EBUS_MSG_MASTER_SLAVE);
				if (ret < 0) {
					fprintf(stdout, "Error during sending ebus message.\n");
				}
				
				fprintf(stdout, "\n");
			}
			
		} while (end == 0);
		
		ret = serial_close(serialfd, &oldtio);
		if (ret == 0) {
			fprintf(stdout, "serial device %s successfully closed.\n", serial);
		}
	}
	
	return 0;
}
