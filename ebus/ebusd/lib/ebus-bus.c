/*
 * Copyright (C) Roland Jax 2012-2013 <roland.jax@liwest.at>
 * crc calculations from http://www.mikrocontroller.net/topic/75698
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

/**
 * @file ebus-bus.c
 * @brief ebus communication functions
 * @author roland.jax@liwest.at
 * @version 0.1
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>

#include "log.h"
#include "ebus-bus.h"



static struct send_data send_data;

static struct recv_data recv_data;

static int rawdump = NO;
static int showraw = NO;

static long max_wait = EBUS_MAX_WAIT;
static int max_retry = EBUS_MAX_RETRY;
static int skip_ack = EBUS_SKIP_ACK;

static unsigned char qq = EBUS_QQ;

static unsigned char ack = EBUS_ACK;
static unsigned char nak = EBUS_NAK;
static unsigned char syn = EBUS_SYN;

static int sfd;
static struct termios oldtio;

static FILE *rawfp = NULL;



void
eb_set_rawdump(int dump)
{
	rawdump = dump;	
}

void
eb_set_showraw(int show)
{
	showraw = show;	
}

void
eb_set_qq(unsigned char src)
{
	qq = src;	
}

void
eb_set_max_wait(long usec)
{
	max_wait = usec;
}

void
eb_set_max_retry(int retry)
{
	max_retry = retry;
}

void
eb_set_skip_ack(int skip)
{
	skip_ack = skip;
}



int
eb_diff_time(const struct timeval *tact, const struct timeval *tlast,
							struct timeval *tdiff)
{
    long diff;

    diff = (tact->tv_usec + 1000000 * tact->tv_sec) -
	   (tlast->tv_usec + 1000000 * tlast->tv_sec);
	   
    tdiff->tv_sec = diff / 1000000;
    tdiff->tv_usec = diff % 1000000;

    return (diff < 0);
}



int
eb_raw_file_open(const char *file)
{
	rawfp = fopen(file, "w");
	err_ret_if(rawfp == NULL, -1);

	return 0;
}

int
eb_raw_file_close(void)
{
	int ret;

	ret = fflush(rawfp);
	err_ret_if(ret == EOF, -1);

	ret = fclose(rawfp);
	err_ret_if(ret == EOF, -1);

	return 0;
}

int
eb_raw_file_write(const unsigned char *buf, int buflen)
{
	int ret, i;

	for (i = 0; i < buflen; i++) {		
		ret = fputc(buf[i], rawfp);
		err_ret_if(ret == EOF, -1);
	}

	ret = fflush(rawfp);
	err_ret_if(ret == EOF, -1);

	return 0;
}

void
eb_raw_print_hex(const unsigned char *buf, int buflen)
{
	int i = 0;
	char msg[SERIAL_BUFSIZE];
	char tmp[4];

	memset(tmp, '\0', sizeof(tmp));
	memset(msg, '\0', sizeof(msg));
	
	for (i = 0; i < buflen; i++) {
		sprintf(tmp, " %02x", buf[i]);
		strncat(msg, tmp, 3);
	}
	log_print(L_EBH, "%s", msg);
}



int
eb_serial_open(const char *dev, int *fd)
{
	int ret;
	struct termios newtio;

	sfd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	err_ret_if(sfd < 0, -1);

	ret = fcntl(sfd, F_SETFL, 0);
	err_ret_if(ret < 0, -1);

	/* save current settings of serial port */
	ret = tcgetattr(sfd, &oldtio);
	err_ret_if(ret < 0, -1);

	memset(&newtio, '\0', sizeof(newtio));

	newtio.c_cflag = SERIAL_BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag &= ~OPOST;

	newtio.c_cc[VMIN]  = 1;
	newtio.c_cc[VTIME] = 0;

	ret = tcflush(sfd, TCIFLUSH);
	err_ret_if(ret < 0, -1);

	/* activate new settings of serial port */
	ret = tcsetattr(sfd, TCSANOW, &newtio);
	err_ret_if(ret < 0, -1);

	*fd = sfd;

	return 0;
}

int
eb_serial_close(void)
{
	int ret;

	/* activate old settings of serial port */
	ret = tcsetattr(sfd, TCSANOW, &oldtio);
	err_ret_if(ret < 0, -1);

	/* Close file descriptor from serial device */
	ret = close(sfd);
	err_ret_if(ret < 0, -1);

	return 0;
}

int
eb_serial_send(const unsigned char *buf, int buflen)
{
	int ret, val;
	
	/* write msg to ebus device */
	val = write(sfd, buf, buflen);
	err_ret_if(val < 0, -1);
	
	ret = tcflush(sfd, TCIOFLUSH);
	err_ret_if(ret < 0, -1);
	
	return val;
}

int
eb_serial_recv(unsigned char *buf, int *buflen)
{
	int ret;
	
	//tcflush(sfd, TCIOFLUSH);
	/* read msg from ebus device */
	*buflen = read(sfd, buf, *buflen);
	err_if(*buflen < 0);

	if (*buflen < 0) {
		log_print(L_WAR, "error read serial device");
		return -1;
	}

	if (*buflen > SERIAL_BUFSIZE) {
		log_print(L_WAR, "read data len > %d", SERIAL_BUFSIZE);
		return -2;
	}

	/* print bus */
	if (showraw == YES)
		eb_raw_print_hex(buf, *buflen);

	/* dump raw data*/
	if (rawdump == YES) {
		ret = eb_raw_file_write(buf, *buflen);
		if (ret < 0)
			log_print(L_WAR, "can't write rawdata");
	}

	return 0;
}



void
eb_print_result(void)
{
	int i;
	//fprintf(stdout, ">>> ");
	for (i = 0; i < recv_data.len; i++)
		fprintf(stdout, " %02x", recv_data.msg[i]);
	fprintf(stdout, "\n");
}



void
eb_get_recv_data(unsigned char *buf, int *buflen)
{
	strncpy(buf, recv_data.msg, recv_data.len);	
	*buflen = recv_data.len;
}

void
eb_recv_data_prepare(const unsigned char *buf, int buflen)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, crc;

	memset(tmp, '\0', sizeof(tmp));

	/* reset struct */
	memset(&recv_data, '\0', sizeof(recv_data));	

	/* set recv_data.msg_esc */
	memcpy(&recv_data.msg_esc[0], buf, buflen);
	recv_data.len_esc = buflen;

	/* set recv_data.crc_calc and .crc_recv */
	if (buf[buflen - 2] == EBUS_SYN_ESC_A9) {
		recv_data.crc_calc = eb_calc_crc(buf, buflen - 2);
		if (buf[buflen - 1] == EBUS_SYN_ESC_01)
			recv_data.crc_recv = EBUS_SYN;
		else
			recv_data.crc_recv = EBUS_SYN_ESC_A9;

		crc = 2;

	} else {
		recv_data.crc_calc = eb_calc_crc(buf, buflen - 1);
		recv_data.crc_recv = buf[buflen - 1];

		crc = 1;
	}

	/* set recv_data.msg */
	memcpy(tmp, buf, buflen - crc);
	tmplen = buflen - crc;

	eb_unesc(tmp, &tmplen);

	memcpy(&recv_data.msg[0], tmp, tmplen);
	recv_data.len = tmplen;

}

int
eb_recv_data(unsigned char *buf, int *buflen)
{
	unsigned char tmp[SERIAL_BUFSIZE], msg[SERIAL_BUFSIZE];
	int tmplen, msglen, ret, i, esc, found;

	memset(msg, '\0', sizeof(msg));
	msglen = 0;
	
	esc = 0;
	found = 99;
	
	/* do until found necessary string*/
	do {
		memset(tmp, '\0', sizeof(tmp));
		tmplen = sizeof(tmp);

		ret = eb_serial_recv(tmp, &tmplen);
		if (ret < 0)
			return -1;
		
		if (tmplen > 0) {

			/* preset tmp buffer with not read bytes from get_bus */
			if (*buflen > 0) {
				
				/* save temporarily tmp in msg buffer */
				memcpy(msg, tmp, tmplen);
				msglen = tmplen;

				/* copy input data into tmp buffer */
				memset(tmp, '\0', sizeof(tmp));
				memcpy(tmp, buf, *buflen);
				tmplen = *buflen;

				/* set input data buffer len to 0 */
				*buflen = 0;

				/* copy saved bus data back to tmp buffer */
				memcpy(&tmp[tmplen], msg, msglen);
				tmplen += msglen;

				/* reset msg buffer */
				memset(msg, '\0', sizeof(msg));
				msglen = 0;							
			}
			
			
			i = 0;
			while (i < tmplen) {

				msg[msglen] = tmp[i];
				msglen++;
						
				/* SYN */
				if (msg[0] == EBUS_SYN) {
					found = -2;
					break;
				}

				/* get end of message */
				if (msglen > 1) {

					if (msg[msglen - 1] == EBUS_SYN_ESC_A9)
						esc++;

					if (msglen == (2 + msg[0] + esc)) {
						found = 0;
						break;
					}
				}

				/*
				 * something went wrong - plz tell me
				 * we got some SYN during answer or
				 * msglen is out of spec. 1 
				 */
				if (msg[msglen] == EBUS_SYN || msg[0] > 16) {

					found = -3;
					break;
				}

				i++;
			}
		}
	} while (found == 99);

	*buflen = msglen;

	memset(buf, '\0', sizeof(buf));
	for (i = 0; i < msglen; i++)
		buf[i] = msg[i];
		
	return found;
}



int
eb_get_ack(unsigned char *buf, int *buflen)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, ret, i, j, found;
	
	j = 0;
	found = 99;

	/* do until found necessary string */
	do {
		memset(tmp, '\0', sizeof(tmp));
		tmplen = sizeof(tmp);

		ret = eb_serial_recv(tmp, &tmplen);
		if (ret < 0)
			return -1;	
		
		if (tmplen > 0) {		
			i = 0;
			while (i < tmplen) {
			
				/* compare recv with sent  - is this possible */
				if (tmp[i] != buf[j] && j < *buflen)
					return -2;
				
				/* compare only slaves answer */
				if (j > (*buflen - 1)) {									

					/* ACK */
					if (tmp[i] == EBUS_ACK)
						found = 0;
					
					/* NAK */
					else if (tmp[i] == EBUS_NAK)
						found = 0;
					
					/* SYN */
					else if (tmp[i] == EBUS_SYN)
						found = -3;
					
					/* ??? */
					else 
						found = -4;
					i++;
					break;
					
				}
				i++;
				j++;
			}
		}
	} while (found == 99);

	*buflen = tmplen - i;

	memset(buf, '\0', sizeof(buf));
	for (j = 0; i < tmplen; i++, j++)
		buf[j] = tmp[i];

	return found;
}



int
eb_wait_bus_syn(int *skip)
{
	unsigned char buf[SERIAL_BUFSIZE];
	int buflen, ret, i, found;
	
	found = 99;

	/* do until SYN read*/
	do {
		memset(buf, '\0', sizeof(buf));
		buflen = sizeof(buf);

		ret = eb_serial_recv(buf, &buflen);
		if (ret < 0)
			return -1;		

		if (buflen > 0) {
			
			i = 0;
			while (i < buflen) {
				/* break if byte = SYN and it is last byte */
				if (buf[i] == EBUS_SYN && (i + 1) == buflen) {
					found = 1;
					break;
				}
				i++;
			}

			if (*skip > 0)
				*skip -= 1;
		}
	} while (found == 99);
	
	return 0;
}

int
eb_wait_bus(void)
{
	unsigned char buf[SERIAL_BUFSIZE];
	int buflen, ret, skip, retry;
	struct timeval tact, tlast, tdiff;

	skip = 0;
	retry = 0;
	
	do {
		ret = eb_wait_bus_syn(&skip);
		if (ret < 0)
			return -1;	

		/* remember start time */
		gettimeofday(&tlast, NULL);

		/* send QQ */
		ret = eb_serial_send(&qq, 1);
		if (ret < 0)
			return -1;

		gettimeofday(&tact, NULL);
		eb_diff_time(&tact, &tlast, &tdiff);

		/* wait ~4200 usec */
		usleep(max_wait - tdiff.tv_usec);

		gettimeofday(&tact, NULL);
		eb_diff_time(&tact, &tlast, &tdiff);

		/* receive 1 byte - must be QQ */
		memset(buf, '\0', sizeof(buf));
		buflen = sizeof(buf);
		ret = eb_serial_recv(buf, &buflen);
		if (ret < 0)
			return -1;		

		/* is sent and read qq byte is equal */
		if (buf[0] == qq && buflen == 1)
			return 0;

		retry++;
		skip = skip_ack + retry;
	
	} while (retry < max_retry);

	/* reached max retry */
	return 1;
}

int
eb_free_bus(void)
{
	int ret, skip;
	ret = 0;
	skip = 0;

	ret = eb_serial_send(&syn, 1);
	if (ret < 0)
		return -1;
		
	ret = eb_wait_bus_syn(&skip);
	if (ret < 0)
		return -1;			

	return 0;
}




void
eb_send_data_prepare(const unsigned char *buf, int buflen)
{
	unsigned char crc[2], tmp[SERIAL_BUFSIZE];
	int tmplen, crclen;

	/* reset struct */
	memset(&send_data, '\0', sizeof(send_data));	

	/* set send_data.msg */
	memcpy(&send_data.msg[0], buf, buflen);
	send_data.len = buflen;


	/* set send_data.msg_esc + send_data.crc */
	memset(tmp, '\0', sizeof(tmp));
	memcpy(tmp, buf, buflen);
	tmplen = buflen;	
	
	eb_esc(tmp, &tmplen);

	memcpy(&send_data.msg_esc[0], &qq, 1);
	memcpy(&send_data.msg_esc[1], tmp, tmplen);
	tmplen++;

	memset(crc, '\0', sizeof(crc));
	send_data.crc = eb_calc_crc(&send_data.msg_esc[0], tmplen);
	crc[0] = send_data.crc;
	crclen = 1;

	if (crc[0] == EBUS_SYN || crc[0] == EBUS_SYN_ESC_A9) {
		/* esc crc */
		eb_esc(crc, &crclen);
		send_data.msg_esc[tmplen] = crc[0];
		tmplen++;
		send_data.msg_esc[tmplen] = crc[1];
		tmplen++;
	} else {
		send_data.msg_esc[tmplen] = crc[0];
		tmplen++;
	}

	send_data.len_esc = tmplen;

}

int
eb_send_data(const unsigned char *buf, int buflen, int type)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, ret, val, i;
	ret = 0;
	i = 0;

	eb_send_data_prepare(buf, buflen);
	
	/* fetch AA and send QQ */
	ret = eb_wait_bus();
	if (ret != 0)
		return -1;
	
	/* send message to slave */
	ret = eb_serial_send(&send_data.msg_esc[1], send_data.len_esc - 1);
	if (ret < 0)
		return -1;

	if (type == EBUS_MSG_BROADCAST) {
		/* free bus */
		ret = eb_free_bus();
		if (ret < 0)
			return -1;		
		
		return 0;
	}

	/* get data from bus (we got our sent message too) */
	memset(tmp, '\0', sizeof(tmp));
	memcpy(tmp, &send_data.msg_esc[1], send_data.len_esc - 1);
	tmplen = send_data.len_esc - 1;

	ret = eb_get_ack(tmp, &tmplen);

	if (ret < 0 || ret > 1) {
		/* free bus */
		ret = eb_free_bus();
		if (ret < 0)
			return -1;		
	
		return -1;
	}

	/* first answer from slave is NAK - send message again (inkl. QQ) */
	if (ret == 1) {
			
		/* send message to slave */
		ret = eb_serial_send(&send_data.msg_esc[0], send_data.len_esc);
		if (ret < 0)
			return -1;

		/* get ack from bus (we got our sent message too) */
		memset(tmp, '\0', sizeof(tmp));
		memcpy(tmp, &send_data.msg_esc[0], send_data.len_esc);
		tmplen = send_data.len_esc;

		ret = eb_get_ack(tmp, &tmplen);		
	
		if (ret == 1) {
			/* free bus */
			ret = eb_free_bus();
			if (ret < 0)
				return -1;
	
			return -1;
		}
		
	}

	if (type == EBUS_MSG_MASTER_MASTER) {
		val = ret;
	
		/* free bus */
		ret = eb_free_bus();
		if (ret < 0)
			return -1;		

		return val;
	}

	/* get data - dont reset buffer */	
	ret = eb_recv_data(tmp, &tmplen);
	if (ret < 0)
		return -1;

	eb_recv_data_prepare(tmp, tmplen);

	/* check crc's from recv_data */
	if (recv_data.crc_calc != recv_data.crc_recv) {
		/* send message to slave */
		ret = eb_serial_send(&nak, 1);
		if (ret < 0)
			return -1;

		/* get data from bus (we got our sent message too) */
		memset(tmp, '\0', sizeof(tmp));
		tmplen = 0;

		ret = eb_get_ack(tmp, &tmplen);		

		/* we compare against nak ! */
		if (ret != 1) {
			/* free bus */
			ret = eb_free_bus();
			if (ret < 0)
				return -1;			
		
			return -1;
		}

		/* get data - don't reset buffer */
		ret = eb_recv_data(tmp, &tmplen);	
		if (ret < 0)
			return -1;

		eb_recv_data_prepare(tmp, tmplen);
		
	}

	if (recv_data.crc_calc != recv_data.crc_recv) {
		ret = eb_serial_send(&nak, 1);		
		if (ret < 0)
			return -1;
		
		val = 1;
	} else {
		ret = eb_serial_send(&ack, 1);
		if (ret < 0)
			return -1;
			
		val = 0;
	}

	/* free bus */
	ret = eb_free_bus();
	if (ret < 0)
		return -1;	
			
	return val;
}



int
eb_cyc_data_recv(unsigned char *buf, int *buflen)
{
	static unsigned char msg[SERIAL_BUFSIZE];
	static int msglen = 0;
	int ret, i;

	if (msglen == 0)
		memset(msg, '\0', sizeof(msg));

	/* get new data */
	ret = eb_serial_recv(buf, buflen);
	if (ret < 0)
		return -1;

	i = 0;
	while (i < *buflen) {
		
		if (buf[i] != EBUS_SYN) {
			msg[msglen] = buf[i];
			msglen++;
		}

		/* ebus syn sign is reached - decode ebus message */
		if (buf[i] == EBUS_SYN && msglen > 0) {
			eb_raw_print_hex(msg, msglen);
			memset(msg, '\0', sizeof(msg));
			msglen = 0;
		}
		
		i++;
	}

	return msglen;
}

