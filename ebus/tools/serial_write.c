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

#define EBUS_SYN 			0xAA
#define EBUS_ACK			0x00
#define	EBUS_NAK			0xFF

#define SERIAL_DEVICE 		"/dev/ttyUSB0"
#define SERIAL_BAUDRATE 	B2400
#define SERIAL_BUFSIZE 		1024

#define err_ret_if(exp, ret) \
	if(exp) { fprintf(stdout, "%s: %d: %s: Error %s\n", \
			__FILE__, __LINE__, __PRETTY_FUNCTION__, strerror(errno));\
			return(ret); \
	}


static int serialfd = -1;
static struct termios oldtio;
static const char *serial = SERIAL_DEVICE;
static const char *progname;


int
data2b_to_float(unsigned char source_lsb, unsigned char source_msb, float *target)
{
	if ((source_msb & 0x80) == 0x80) {
		*target = (float) (-   ( ((unsigned char) (~ source_msb)) +
		                     ( ( ((unsigned char) (~ source_lsb)) + 1) / 256.0) ) );

		if (source_msb  == 0x80 && source_lsb == 0x00) {
			return 0;
		} else {
			return -1;
		}
	} else {
		*target = (float) (source_msb + (source_lsb / 256.0));
		return 1;
	}
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
serial_close(int *fd, struct termios *oldtio)
{
	int ret;

	/* activate old settings of serial port */
	ret = tcsetattr(*fd, TCSANOW, oldtio);
	err_ret_if(ret < 0, -1);

	/* Close file descriptor from serial device */
	ret = close(*fd);
	err_ret_if(ret < 0, -1);

	return 0;
}

int
serial_catch_syn()
{
	unsigned char in[SERIAL_BUFSIZE];
	int inlen, maxlen, i, found;
	
	maxlen = sizeof(SERIAL_BUFSIZE);
	
	memset(in, '\0', sizeof(in));
	inlen = sizeof(in);
	found = 0;
	
	/* wait until ebus SYN sign catched*/
	do {
		inlen = read(serialfd, in, inlen);
		//err_ret_if(inlen < 0 || inlen > maxlen, -1);
		
		if (inlen > 0) {
			i = 0;
			while (i < inlen) {
				if (in[i] == EBUS_SYN && (i + 1) == inlen) {
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
serial_write(int fd, unsigned char buf[], int buflen, int rawdump)
{
	unsigned char in[SERIAL_BUFSIZE], out[SERIAL_BUFSIZE], tmp[SERIAL_BUFSIZE], crc;
	int inlen, outlen, tmplen, i, j, found, ret;

	
	if (!serial_catch_syn()) {
		
		
		/* prepare message for slave */
		memset(out, '\0', sizeof(out));
		outlen = sizeof(buf);
		memcpy(out, buf, outlen);
		crc = calc_crc(&out[0], outlen);
		out[outlen] = crc;
		outlen++;

		/* write msg to ebus */
		ret = write(serialfd, out, outlen);
		err_ret_if(ret < 0, -1);
		
		fprintf(stdout, "<<< ");
		for (i = 0; i < outlen; i++) {
			fprintf(stdout, "%02x ", out[i]);
		}
		fprintf(stdout, "\n");
	
	
	
	
		/* wait for answer from slave */
		memset(in, '\0', sizeof(in));
		inlen = sizeof(in);
		memset(tmp, '\0', sizeof(tmp));
		tmplen = 0;
		found = 0;
		j = 0;
		
		do {
			inlen = read(serialfd, in, inlen);
			
			if (inlen > 0) {
				i = 0;
				while (i < inlen) {
					/* copy only slaves answer into tmp */
					if (j >= outlen) {
						tmp[tmplen] = in[i];
						
						/* break loop if slave answer is ff or end of message is reached. */
						if (tmp[0] == EBUS_NAK || (tmplen == (2 + tmp[1]) && tmplen >= 1)) {
							found = 1;
							break;
						}
						tmplen++;							
					}
					
					/* do we need this? */
					if (in[i] == EBUS_SYN) {
						fprintf(stdout, "Hope, we never saw this.");
						found = 1;
						break;
					}
					
					i++;
					j++;
				}
			}
		} while (found == 0);
		
		fprintf(stdout, ">>> ");
		for (i = 0; i <= tmplen; i++) {
			fprintf(stdout, "%02x ", tmp[i]);
		}
		fprintf(stdout, "\n");
	
	
	
	
		/* prepare answer to slave */
		if (tmp[0] == EBUS_ACK) {
			crc = calc_crc(&tmp[1], tmplen - 1);
			if (tmp[tmplen] == crc) {		
				/* send ACK and SYN */
				memset(out, '\0', sizeof(out));
				out[0] = EBUS_ACK;
				out[1] = EBUS_SYN;
				outlen = 2;
			}
		} else {
			/* send NAK and SYN */
			memset(out, '\0', sizeof(out));
			out[0] = EBUS_NAK;
			out[1] = EBUS_SYN;
			outlen = 2;
		}
		
		fprintf(stdout, "<<< ");
		for (i = 0; i < outlen; i++) {
			fprintf(stdout, "%02x ", out[i]);
		}
		fprintf(stdout, "\n");
			
		/* write msg to ebus */
		ret = write(serialfd, out, outlen);
		err_ret_if(ret < 0, -1);
		
		
		if (!serial_catch_syn()) {
			return 0;
		}

	}
			
	return -1;
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
	unsigned char msg[20];
	
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
				
				fprintf(stdout, "\n<<< ");
				for (i = 0; i < k; i++) {
					fprintf(stdout, "%02x ", msg[i]);
				}
				fprintf(stdout, "\n\n");
				
				serial_write(serialfd, msg, sizeof(msg), 0);
				
				fprintf(stdout, "\n");
			}
			
		} while (end == 0);
		
		
		
		
		ret = serial_close(&serialfd, &oldtio);
		if (ret == 0) {
			fprintf(stdout, "serial device %s successfully closed.\n", serial);
		}
	}
	

	

	
	
	
	return 0;
}
