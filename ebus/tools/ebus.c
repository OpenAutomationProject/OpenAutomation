/*
 * Copyright (C) Roland Jax 2012-2013 <roland.jax@liwest.at>
 * crc calculations from http://www.mikrocontroller.net/topic/75698
 *
 * This file is part of libebus.
 *
 * libebus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libebus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libebus. If not, see http://www.gnu.org/licenses/.
 */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <ctype.h>

#include "ebus.h"

struct send_data {
	unsigned char crc;
	unsigned char msg[SERIAL_BUFSIZE];
	int len;
	unsigned char msg_esc[SERIAL_BUFSIZE];
	int len_esc;
};

static struct send_data send_data;

struct recv_data {
	unsigned char crc_recv;
	unsigned char crc_calc;	
	unsigned char msg[SERIAL_BUFSIZE];
	int len;
	unsigned char msg_esc[SERIAL_BUFSIZE];
	int len_esc;
};

static struct recv_data recv_data;

static long max_wait = EBUS_MAX_WAIT;
static int max_retry = EBUS_MAX_RETRY;
static int skip_ack = EBUS_SKIP_ACK;

static unsigned char qq = EBUS_QQ;

static unsigned char ack = EBUS_ACK;
static unsigned char nak = EBUS_NAK;
static unsigned char syn = EBUS_SYN;

static int sfd;
static struct termios oldtio;


int serial_send(const unsigned char *buf, int buflen);
int serial_recv(unsigned char *buf, int *buflen);

int ebus_diff_time(const struct timeval *tact, const struct timeval *tlast,
							struct timeval *tdiff);

void ebus_recv_data_prepare(const unsigned char *buf, int buflen);
int ebus_recv_data(unsigned char *buf, int *buflen);

int ebus_get_ack(unsigned char *buf, int *buflen);

int ebus_wait_syn(int *skip);
int ebus_get_bus();

void ebus_send_data_prepare(const unsigned char *buf, int buflen);

unsigned char ebus_calc_crc_byte(unsigned char byte, unsigned char init_crc);


/*
 * return value
 * -1 error
 *  0 ok
 */
int
serial_open(const char *dev, int *fd)
{
	int ret;
	struct termios newtio;

	sfd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0)
		return -1;

	ret = fcntl(sfd, F_SETFL, 0);
	if (ret < 0)
		return -1;

	/* save current settings of serial port */
	ret = tcgetattr(sfd, &oldtio);
	if (ret < 0)
		return -1;

	memset(&newtio, '\0', sizeof(newtio));

	newtio.c_cflag = SERIAL_BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag &= ~OPOST;

	newtio.c_cc[VMIN]  = 1;
	newtio.c_cc[VTIME] = 0;

	ret = tcflush(sfd, TCIFLUSH);
	if (ret < 0)
		return -1;

	/* activate new settings of serial port */
	ret = tcsetattr(sfd, TCSANOW, &newtio);
	if (ret < 0)
		return -1;

	*fd = sfd;

	return 0;
}

/*
 * return value
 * -1 error
 *  0 ok
 */
int
serial_close()
{
	int ret;

	/* activate old settings of serial port */
	ret = tcsetattr(sfd, TCSANOW, &oldtio);
	if (ret < 0)
		return -1;

	/* Close file descriptor from serial device */
	ret = close(sfd);
	if (ret < 0)
		return -1;

	return 0;
}

/*
 * return value
 * -1 error
 *  0 ok
 */
int
serial_send(const unsigned char *buf, int buflen)
{
	int ret;
	
	/* write msg to ebus device */
	ret = write(sfd, buf, buflen);
	tcflush(sfd, TCIOFLUSH);
	
	return ret;
}

/*
 * return value
 * -1 error
 *  0 ok
 */
int
serial_recv(unsigned char *buf, int *buflen)
{
	tcflush(sfd, TCIOFLUSH);
	/* read msg from ebus device */
	*buflen = read(sfd, buf, *buflen);

	if (*buflen < 0)
		return -1;

	return 0;
}


/*
 * return value
 * delta time of two time stamps
 */
int
ebus_diff_time(const struct timeval *tact, const struct timeval *tlast,
							struct timeval *tdiff)
{
    long diff;

    diff = (tact->tv_usec + 1000000 * tact->tv_sec) -
	   (tlast->tv_usec + 1000000 * tlast->tv_sec);
	   
    tdiff->tv_sec = diff / 1000000;
    tdiff->tv_usec = diff % 1000000;

    return (diff < 0);
}


/*
 * return value
 * int value for hex byte
 * -1 if not
 */
int
ebus_htoi(const char *buf)
{
	int ret;
	ret = -1;
	
	if (isxdigit(*buf)) {
		if (isalpha(*buf))
			ret = 55;	
		else
			ret = 48;
			
		ret = toupper(*buf) - ret;
	}

	return ret;
}


void
ebus_set_qq(unsigned char src)
{
	qq = src;
#ifdef DEBUG
fprintf(stdout,"[%s]\t\t qq: %02x\n",__PRETTY_FUNCTION__,qq);
#endif		
}

void
ebus_set_max_wait(long usec)
{
	max_wait = usec;
#ifdef DEBUG
fprintf(stdout,"[%s]\t wait: %ld\n",__PRETTY_FUNCTION__,max_wait);
#endif	
}

void
ebus_set_max_retry(int retry)
{
	max_retry = retry;
#ifdef DEBUG
fprintf(stdout,"[%s]\t retry: %d\n",__PRETTY_FUNCTION__,max_retry);
#endif	
}

void
ebus_set_skip_ack(int skip)
{
	skip_ack = skip;
#ifdef DEBUG
fprintf(stdout,"[%s]\t skip: %d\n",__PRETTY_FUNCTION__,skip_ack);
#endif	
}


void
ebus_print_result()
{
	int i;
	//fprintf(stdout, ">>> ");
	for (i = 0; i < recv_data.len; i++)
		fprintf(stdout, " %02x", recv_data.msg[i]);
	fprintf(stdout, "\n");
}


void
ebus_esc(unsigned char *buf, int *buflen)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, i;

	memset(tmp, '\0', sizeof(tmp));
	i = 0;
	tmplen = 0;
	
	while (i < *buflen) {
		
		if (buf[i] == EBUS_SYN) {
			tmp[tmplen] = EBUS_SYN_ESC_A9;
			tmplen++;
			tmp[tmplen] = EBUS_SYN_ESC_01;
			tmplen++;
		} else if (buf[i] == EBUS_SYN_ESC_A9) {
			tmp[tmplen] = EBUS_SYN_ESC_A9;
			tmplen++;
			tmp[tmplen] = EBUS_SYN_ESC_00;
			tmplen++;
		} else {
			tmp[tmplen] = buf[i];
			tmplen++;
		}
		
		i++;
	}

	memset(buf, '\0', sizeof(buf));
	for (i = 0; i < tmplen; i++)
		buf[i] = tmp[i];

	*buflen = tmplen;
}

void
ebus_unesc(unsigned char *buf, int *buflen)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, i, found;

	memset(tmp, '\0', sizeof(tmp));
	i = 0;
	tmplen = 0;
	found = 0;
	
	while (i < *buflen) {
		
		if (buf[i] == EBUS_SYN_ESC_A9) {
			found = 1;
		} else if (found == 1) {
			if (buf[i] == EBUS_SYN_ESC_01) {
				tmp[tmplen] = EBUS_SYN;
				tmplen++;
			} else {
				tmp[tmplen] = EBUS_SYN_ESC_A9;
				tmplen++;
			}
			
			found = 0;
		} else {
			tmp[tmplen] = buf[i];
			tmplen++;
		}
		
		i++;
	}

	memset(buf, '\0', sizeof(buf));
	for (i = 0; i < tmplen; i++)
		buf[i] = tmp[i];

	*buflen = tmplen;
}



void
ebus_recv_data_prepare(const unsigned char *buf, int buflen)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, crc;

	memset(tmp, '\0', sizeof(tmp));

	/* reset struct */
	memset(&recv_data, '\0', sizeof(recv_data));	

	/* set recv_data.msg_esc */
	memcpy(&recv_data.msg_esc[0], buf, buflen);
	recv_data.len_esc = buflen;
#ifdef DEBUG
int i;
fprintf(stdout, "[%s] po1: ",__PRETTY_FUNCTION__);
for (i = 0; i < recv_data.len_esc; i++)
	fprintf(stdout, " %02x", recv_data.msg_esc[i]);
fprintf(stdout, "\n");
#endif
	/* set recv_data.crc_calc and .crc_recv */
	if (buf[buflen - 2] == EBUS_SYN_ESC_A9) {
		recv_data.crc_calc = ebus_calc_crc(buf, buflen - 2);
		if (buf[buflen - 1] == EBUS_SYN_ESC_01)
			recv_data.crc_recv = EBUS_SYN;
		else
			recv_data.crc_recv = EBUS_SYN_ESC_A9;

		crc = 2;

	} else {
		recv_data.crc_calc = ebus_calc_crc(buf, buflen - 1);
		recv_data.crc_recv = buf[buflen - 1];

		crc = 1;
	}
#ifdef DEBUG
fprintf(stdout, "[%s] crc_calc: %02x crc_recv: %02x\n",__PRETTY_FUNCTION__,recv_data.crc_calc,recv_data.crc_recv);
#endif
	/* set recv_data.msg */
	memcpy(tmp, buf, buflen - crc);
	tmplen = buflen - crc;

	ebus_unesc(tmp, &tmplen);

	memcpy(&recv_data.msg[0], tmp, tmplen);
	recv_data.len = tmplen;
#ifdef DEBUG
fprintf(stdout, "[%s] po2: ",__PRETTY_FUNCTION__);
for (i = 0; i < recv_data.len; i++)
	fprintf(stdout, " %02x", recv_data.msg[i]);
fprintf(stdout, "\n");
#endif
}

/*
 * return value
 * -3 syn received after anser from slave
 * -2 syn received (no answer from slave)
 * -1 recv error
 *  0 got ack and end of msg is received
 */
int
ebus_recv_data(unsigned char *buf, int *buflen)
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

		ret = serial_recv(tmp, &tmplen);
		if (ret < 0)
			return -1;
		
		if (tmplen > 0) {

			/* preset tmp buffer with not read bytes from get_bus */
			if (*buflen > 0) {

#ifdef DEBUG
fprintf(stdout,"[%s]\t input data from get_bus buflen: %d ",__PRETTY_FUNCTION__,*buflen);
for (i = 0; i < *buflen; i++)
	fprintf(stdout, " %02x", buf[i]);
fprintf(stdout, "\n");
#endif
				
				/* save temporary tmp in msg buffer */
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
#ifdef DEBUG
fprintf(stdout,"[%s]\t tmp buffer inkl. preset values tmplen: %d ",__PRETTY_FUNCTION__,tmplen);
for (i = 0; i < tmplen; i++)
	fprintf(stdout, " %02x", tmp[i]);
fprintf(stdout, "\n");
#endif								
			}
			
			
			i = 0;
			while (i < tmplen) {

				msg[msglen] = tmp[i];
				msglen++;
#ifdef DEBUG				
fprintf(stdout,"[%s]\t tmplen: %d tmp [%d]: %02x - msg [%d]: %02x esc: %d\n",__PRETTY_FUNCTION__,tmplen,i,tmp[i],msglen-1,msg[msglen-1],esc);
#endif							
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
#ifdef DEBUG					
fprintf(stdout,"[%s]\t msg: %02x msglen: %d msg[1]: %02x %d\n",__PRETTY_FUNCTION__,msg[msglen], msglen, msg[1],msg[1]);
#endif
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



/*
 * return value
 * -4 should be never seen
 * -3 syn received (no answer from slave)
 * -2 send and recv msg are different
 * -1 recv error
 *  0 ack
 *  1 nak 
 */
int
ebus_get_ack(unsigned char *buf, int *buflen)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, ret, i, j, found;
	
	j = 0;
	found = 99;

	/* do until found necessary string */
	do {
		memset(tmp, '\0', sizeof(tmp));
		tmplen = sizeof(tmp);

		ret = serial_recv(tmp, &tmplen);
		if (ret < 0)
			return -1;
		
		if (tmplen > 0) {		
			i = 0;
			while (i < tmplen) {
#ifdef DEBUG			
fprintf(stdout,"[%s]\t\t tmplen: %d tmp [%d]: %02x - buf [%d]: %02x\n",__PRETTY_FUNCTION__,tmplen,i,tmp[i],j,buf[j]);
#endif				
				/* compare recv with sent  - is this possible */
				if (tmp[i] != buf[j] && j < *buflen)
					return -2;
				
				/* compare only slaves answer */
				if (j > (*buflen - 1)) {					
#ifdef DEBUG					
fprintf(stdout,"[%s]\t\t j: %d buflen: %d tmp[%d]: %02x\n",__PRETTY_FUNCTION__,j,*buflen,i,tmp[i]);
#endif					

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



/*
 * return value
 * -1 error
 *  0 ok
 */
int
ebus_wait_syn(int *skip)
{
	unsigned char buf[SERIAL_BUFSIZE];
	int buflen, ret, i, found;
	
	found = 99;

	/* do until SYN read*/
	do {
		memset(buf, '\0', sizeof(buf));
		buflen = sizeof(buf);

		ret = serial_recv(buf, &buflen);
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
#ifdef DEBUG
fprintf(stdout, "[%s]\t\t skip: %d found: %d\n",__PRETTY_FUNCTION__, *skip, found);
#endif
			if (*skip > 0)
				*skip -= 1;
		}
	} while (found == 99);
	
	return 0;
}

/*
 * return value
 * -1 error
 *  0 get bus
 *  1 reached max retry
 */
int
ebus_get_bus()
{
	unsigned char buf[SERIAL_BUFSIZE];
	int buflen, ret, skip, retry;
	struct timeval tact, tlast, tdiff;

	skip = 0;
	retry = 0;
	
	do {
		ret = ebus_wait_syn(&skip);
		if (ret < 0)
			return -1;	

		/* remember start time */
		gettimeofday(&tlast, NULL);

		/* send QQ */
		ret = serial_send(&qq, 1);
		if (ret < 0)
			return -1;

		gettimeofday(&tact, NULL);
		ebus_diff_time(&tact, &tlast, &tdiff);
#ifdef DEBUG		
fprintf(stdout, "[%s]\t\t write: %ld.%06ld\n",__PRETTY_FUNCTION__, tdiff.tv_sec, tdiff.tv_usec);
#endif
		/* wait ~4200 usec */
		usleep(max_wait - tdiff.tv_usec);

		gettimeofday(&tact, NULL);
		ebus_diff_time(&tact, &tlast, &tdiff);
#ifdef DEBUG
fprintf(stdout, "[%s]\t\t wait : %ld.%06ld \n",__PRETTY_FUNCTION__, tdiff.tv_sec, tdiff.tv_usec);
#endif

		/* receive 1 byte - must be QQ */
		memset(buf, '\0', sizeof(buf));
		buflen = sizeof(buf);
		ret = serial_recv(buf, &buflen);
		if (ret < 0)
			return -1;

		/* is sent and read qq byte is equal */
		if (buf[0] == qq && buflen == 1)
			return 0;

		skip = skip_ack;
		retry++;
#ifdef DEBUG
fprintf(stdout, "[%s]\t\t retry: %d\n",__PRETTY_FUNCTION__, retry);
#endif		
	} while (retry < max_retry);

#ifdef DEBUG
fprintf(stdout, "[%s]\t\t max retry %d reached, can't get bus.\n",__PRETTY_FUNCTION__,max_retry);
#endif	

	/* reached max retry */
	return 1;
}


void
ebus_send_data_prepare(const unsigned char *buf, int buflen)
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
	
	ebus_esc(tmp, &tmplen);

	memcpy(&send_data.msg_esc[0], &qq, 1);
	memcpy(&send_data.msg_esc[1], tmp, tmplen);
	tmplen++;

	memset(crc, '\0', sizeof(crc));
	send_data.crc = ebus_calc_crc(&send_data.msg_esc[0], tmplen);
	crc[0] = send_data.crc;

	if (crc[0] == EBUS_SYN || crc[0] == EBUS_SYN_ESC_A9) {
		/* esc crc */
		ebus_esc(crc, &crclen);
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

/*
 * return value
 * -1 error
 *  0 ACK
 *  1 NAK
 */
int
ebus_send_data(const unsigned char *buf, int buflen, int type)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, ret, val, i;
	ret = 0;
	i = 0;

	ebus_send_data_prepare(buf, buflen);
	
	/* fetch AA and send QQ */
	ret = ebus_get_bus();
	if (ret != 0)
		return -1;
		
#ifdef DEBUG
fprintf(stdout, "[%s]\t got bus\n",__PRETTY_FUNCTION__);


fprintf(stdout, "[%s]\t <<<  ",__PRETTY_FUNCTION__);
for (i = 0; i < send_data.len; i++)
	fprintf(stdout, " %02x", send_data.msg[i]);
fprintf(stdout, "\n");
#endif

	/* send message to slave */
	ret = serial_send(&send_data.msg_esc[1], send_data.len_esc - 1);
	if (ret < 0)
		return -1;

	if (type == EBUS_MSG_BROADCAST) {
		/* SYN bus */
		ret = serial_send(&syn, 1);
		if (ret < 0)
			return -1;
		
		return 0;
	}

	/* get data from bus (we got our sent message too) */
	memset(tmp, '\0', sizeof(tmp));
	memcpy(tmp, &send_data.msg_esc[1], send_data.len_esc - 1);
	tmplen = send_data.len_esc - 1;

	ret = ebus_get_ack(tmp, &tmplen);
#ifdef DEBUG
fprintf(stdout,"[%s]\t ret: %d tmplen: %d ",__PRETTY_FUNCTION__,ret,tmplen);
for (i = 0; i < tmplen; i++)
	fprintf(stdout, " %02x", tmp[i]);
fprintf(stdout, "\n");
#endif
	if (ret < 0 || ret > 1) {
		/* free bus */
		ret = serial_send(&syn, 1);	
		if (ret < 0)
			return -1;
	
		return -1;
	}

	/* first answer from slave is NAK - send message again (inkl. QQ) */
	if (ret == 1) {
			
		/* send message to slave */
		ret = serial_send(&send_data.msg_esc[0], send_data.len_esc);
		if (ret < 0)
			return -1;

		/* get ack from bus (we got our sent message too) */
		memset(tmp, '\0', sizeof(tmp));
		memcpy(tmp, &send_data.msg_esc[0], send_data.len_esc);
		tmplen = send_data.len_esc;

		ret = ebus_get_ack(tmp, &tmplen);		
#ifdef DEBUG
fprintf(stdout,"[%s]\t ret: %d tmplen: %d ",__PRETTY_FUNCTION__,ret,tmplen);
for (i = 0; i < tmplen; i++)
	fprintf(stdout, " %02x", tmp[i]);
fprintf(stdout, "\n");
#endif		
		if (ret == 1) {
			/* free bus */
			ret = serial_send(&syn, 1);	
			if (ret < 0)
				return -1;
		
			return -1;
		}
		
	}

	if (type == EBUS_MSG_MASTER_MASTER) {
		val = ret;
	
		/* free bus */
		ret = serial_send(&syn, 1);
		if (ret < 0)
			return -1;

		return val;
	}

	/* get data - dont reset buffer */	
	ret = ebus_recv_data(tmp, &tmplen);
#ifdef DEBUG
fprintf(stdout,"[%s]\t ret: %d\n",__PRETTY_FUNCTION__,ret);
#endif	
	if (ret < 0)
		return -1;
#ifdef DEBUG	
fprintf(stdout, "[%s]\t re%d: ",__PRETTY_FUNCTION__, ret);
for (i = 0; i < buflen; i++)
	fprintf(stdout, " %02x", buf[i]);
fprintf(stdout, "\n");
#endif
	ebus_recv_data_prepare(tmp, tmplen);

	/* check crc's from recv_data */
	if (recv_data.crc_calc != recv_data.crc_recv) {
		/* send message to slave */
		ret = serial_send(&nak, 1);
		if (ret < 0)
			return -1;

		/* get data from bus (we got our sent message too) */
		memset(tmp, '\0', sizeof(tmp));
		tmplen = 0;

		ret = ebus_get_ack(tmp, &tmplen);		
#ifdef DEBUG
fprintf(stdout,"[%s]\t ret: %d tmplen: %d ",__PRETTY_FUNCTION__,ret,tmplen);
for (i = 0; i < tmplen; i++)
	fprintf(stdout, " %02x", tmp[i]);
fprintf(stdout, "\n");
#endif
		/* we compare against nak ! */
		if (ret != 1) {
			/* free bus */
			ret = serial_send(&syn, 1);	
			if (ret < 0)
				return -1;
		
			return -1;
		}

		/* get data - dont reset buffer */
		ret = ebus_recv_data(tmp, &tmplen);
#ifdef DEBUG
fprintf(stdout,"[%s]\t ret: %d\n",__PRETTY_FUNCTION__,ret);
#endif		
		if (ret < 0)
			return -1;

#ifdef DEBUG
fprintf(stdout, "[%s]\t re%d: ",__PRETTY_FUNCTION__, ret);
for (i = 0; i < buflen; i++)
	fprintf(stdout, " %02x", buf[i]);
fprintf(stdout, "\n");
#endif
		ebus_recv_data_prepare(tmp, tmplen);
		
	}

	if (recv_data.crc_calc != recv_data.crc_recv) {
		ret = serial_send(&nak, 1);		
		if (ret < 0)
			return -1;
		
		val = 1;
	} else {
		ret = serial_send(&ack, 1);
		if (ret < 0)
			return -1;
			
		val = 0;
	}

	/* free bus */
	ret = serial_send(&syn, 1);;	
	if (ret < 0)
		return -1;
			
#ifdef DEBUG	
fprintf(stdout, "[%s]\t >>>  ",__PRETTY_FUNCTION__);
for (i = 0; i < recv_data.len; i++)
	fprintf(stdout, " %02x", recv_data.msg[i]);
fprintf(stdout, "\n");
#endif
	return val;
}



/*
 *  return value
 *  0 - substitute value
 *  1 - positive value
 */
int
ebus_bcd_to_int(unsigned char src, int *tgt)
{
	if ((src & 0x0F) > 0x09 || ((src >> 4) & 0x0F) > 0x09) {
		*tgt = (int) (0xFF);
		return 0;
	} else {
		*tgt = (int) ( ( ((src & 0xF0) >> 4) * 10) + (src & 0x0F) );
		return 1;
	}
}

/*
 *  return value
 *  0 - substitute value
 *  1 - positive value
 */
int
ebus_int_to_bcd(int src, unsigned char *tgt)
{
	if (src > 99) {
		*tgt = (unsigned char) (0xFF);
		return 0;
	} else {
		*tgt = (unsigned char) ( ((src / 10) << 4) | (src % 10) );
		return 1;
	}
}

/*
 *  return value
 * -1 - negative value
 *  0 - substitute value
 *  1 - positive value
 */
int
ebus_data1b_to_int(unsigned char src, int *tgt)
{
	if ((src & 0x80) == 0x80) {
		*tgt = (int) (- ( ((unsigned char) (~ src)) + 1) );

		if (*tgt  == -0x80)
			return 0;
		else
			return -1;

	} else {
		*tgt = (int) (src);
		return 1;
	}
}

/*
 *  return value
 * -1 - negative value
 *  0 - substitute value
 *  1 - positive value
 */
int
ebus_int_to_data1b(int src, unsigned char *tgt)
{
	if (src < -127 || src > 127) {
		*tgt = (unsigned char) (0x80);
		return 0;
	} else {
		if (src >= 0) {
			*tgt = (unsigned char) (src);
			return 1;
		} else {
			*tgt = (unsigned char) (- (~ (src - 1) ) );
			return -1;
		}
	}
}

/*
 *  return value
 * -1 - negative value
 *  0 - substitute value
 *  1 - positive value
 */
int
ebus_data1c_to_float(unsigned char src, float *tgt)
{
	if (src > 0xC8) {
		*tgt = (float) (0xFF);
		return 0;
	} else {
		*tgt = (float) (src / 2.0);
		return 1;
	}
}

/*
 *  return value
 * -1 - negative value
 *  0 - substitute value
 *  1 - positive value
 */
int
ebus_float_to_data1c(float src, unsigned char *tgt)
{
	if (src < 0.0 || src > 100.0) {
		*tgt = (unsigned char) (0xFF);
		return 0;
	} else {
		*tgt = (unsigned char) (src * 2.0);
		return 1;
	}
}

/*
 *  return value
 * -1 - negative value
 *  0 - substitute value
 *  1 - positive value
 */
int
ebus_data2b_to_float(unsigned char src_lsb, unsigned char src_msb, float *tgt)
{
	if ((src_msb & 0x80) == 0x80) {
		*tgt = (float) (-   ( ((unsigned char) (~ src_msb)) +
				  ( ( ((unsigned char) (~ src_lsb)) + 1) / 256.0) ) );

		if (src_msb  == 0x80 && src_lsb == 0x00)
			return 0;
		else
			return -1;

	} else {
		*tgt = (float) (src_msb + (src_lsb / 256.0));
		return 1;
	}
}

/*
 *  return value
 * -1 - negative value
 *  0 - substitute value
 *  1 - positive value
 */
int
ebus_float_to_data2b(float src, unsigned char *tgt_lsb, unsigned char *tgt_msb)
{
	if (src < -127.999 || src > 127.999) {
		*tgt_msb = (unsigned char) (0x80);
		*tgt_lsb = (unsigned char) (0x00);
		return 0;
	} else {
		*tgt_lsb = (unsigned char) ((src - ((unsigned char) src)) * 256.0);

		if (src < 0.0 && *tgt_lsb != 0x00)
			*tgt_msb = (unsigned char) (src - 1);
		else
			*tgt_msb = (unsigned char) (src);

		if (src >= 0.0)
			return 1;
		else
			return -1;

	}
}

/*
 *  return value
 * -1 - negative value
 *  0 - substitute value
 *  1 - positive value
 */
int
ebus_data2c_to_float(unsigned char src_lsb, unsigned char src_msb, float *tgt)
{
	if ((src_msb & 0x80) == 0x80) {
		*tgt = (float) (- ( ( ( ((unsigned char) (~ src_msb)) * 16.0) ) +
		                    ( ( ((unsigned char) (~ src_lsb)) & 0xF0) >> 4) +
		                  ( ( ( ((unsigned char) (~ src_lsb)) & 0x0F) +1 ) / 16.0) ) );

		if (src_msb  == 0x80 && src_lsb == 0x00)
			return 0;
		else
			return -1;

	} else {
		*tgt = (float) ( (src_msb * 16.0) + ((src_lsb & 0xF0) >> 4) + ((src_lsb & 0x0F) / 16.0) );
		return 1;
	}
}

/*
 *  return value
 * -1 - negative value
 *  0 - substitute value
 *  1 - positive value
 */
int
ebus_float_to_data2c(float src, unsigned char *tgt_lsb, unsigned char *tgt_msb)
{
	if (src < -2047.999 || src > 2047.999) {
		*tgt_msb = (unsigned char) (0x80);
		*tgt_lsb = (unsigned char) (0x00);
		return 0;
	} else {
		*tgt_lsb = ( ((unsigned char) ( ((unsigned char) src) % 16) << 4) +
		             ((unsigned char) ( (src - ((unsigned char) src)) * 16.0)) );

		if (src < 0.0 && *tgt_lsb != 0x00)
			*tgt_msb = (unsigned char) ((src / 16.0) - 1);
		else
			*tgt_msb = (unsigned char) (src / 16.0);

		if (src >= 0.0)
			return 1;
		else
			return -1;
	}
}


/*
 * CRC calculation "CRC-8-WCDMA"
 * Polynom "x^8 + x^7 + x^4 + x^3 + x + 1"
 *
 * crc calculations by http://www.mikrocontroller.net/topic/75698
 * 
 * return value
 * calculated crc byte
 */

unsigned char
ebus_calc_crc_byte(unsigned char byte, unsigned char init_crc)
{
	unsigned char crc, polynom;
	int i;

	crc = init_crc;

	for (i = 0; i < 8; i++) {

		if (crc & 0x80)
			polynom = (unsigned char) 0x9B;
		else
			polynom = (unsigned char) 0;

		crc = (unsigned char) ((crc & ~0x80) << 1);

		if (byte & 0x80)
			crc = (unsigned char) (crc | 1);

		crc = (unsigned char) (crc ^ polynom);
		byte = (unsigned char) (byte << 1);
	}
	return crc;
}

unsigned char
ebus_calc_crc(const unsigned char *buf, int buflen)
{
	int i;
	unsigned char crc = 0;

	for (i = 0 ; i < buflen ; i++, buf++)
		crc = ebus_calc_crc_byte(*buf, crc);

	return crc;
}

