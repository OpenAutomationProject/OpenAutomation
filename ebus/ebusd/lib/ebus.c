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
 * @file ebus.c
 * @brief ebus communication functions
 * @author roland.jax@liwest.at
 *
 * @todo missing functions for date, time and ascii
 *
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

#include "ebus.h"


#define EBUS_SYN_ESC_A9         0xA9
#define EBUS_SYN_ESC_01         0x01
#define EBUS_SYN_ESC_00         0x00

#define EBUS_ACK                0x00
#define EBUS_NAK                0xFF
#define EBUS_BROADCAST          0xFE

/**
 * @brief sending data structure
 */
struct send_data {
	unsigned char crc; /**< crc of escaped message */
	unsigned char msg[SERIAL_BUFSIZE]; /**< original message */
	int len; /**< length of original message */
	unsigned char msg_esc[SERIAL_BUFSIZE]; /**< esacaped message */
	int len_esc; /**< length of  esacaped message */
};

static struct send_data send_data;

/**
 * @brief receiving data structure
 */
struct recv_data {
	unsigned char crc_recv; /**< received crc */
	unsigned char crc_calc;	/**< calculated crc */
	unsigned char msg[SERIAL_BUFSIZE]; /**< unescaped message */
	int len; /**< length of unescaped message */
	unsigned char msg_esc[SERIAL_BUFSIZE]; /**< received message */
	int len_esc; /**< length of received message */
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


/**
 * @brief calculate delte time of given time stamps
 * @param [in] *tact newer time stamp
 * @param [in] *tlast older time stamp
 * @param [out] *tdiff calculated time difference
 * @return 0 positive | -1 negative ??? lol
 */
int eb_diff_time(const struct timeval *tact, const struct timeval *tlast,
							struct timeval *tdiff);


/**
 * @brief prepare received data
 * @param [in] *buf pointer to a byte array of received bytes
 * @param [in] buflen length of received bytes
 * @return none
 */
void eb_recv_data_prepare(const unsigned char *buf, int buflen);

/**
 * @brief receive bytes from serial device
 * @param [out] *buf pointer to a byte array of received bytes
 * @param [out] buflen length of received bytes
 * @return 0 ok | -1 error | -2 syn no answer from slave 
 * | -3 SYN received after answer from slave
 */  
int eb_recv_data(unsigned char *buf, int *buflen);

 /**
 * @brief receive ACK byte from serial device
 * @param [out] *buf pointer to a byte array of received bytes
 * @param [out] buflen length of received bytes
 * @return 0 ok | 1 negative ACK | -1 error | -2 send and recv msg are different 
 * | -3 syn received (no answer from slave) | -4 we should never reach this
 */  
int eb_get_ack(unsigned char *buf, int *buflen);


/**
 * @brief receive bytes from serial device until SYN byte was received
 * @param [in] *skip number skipped SYN bytes
 * @return 0 ok | -1 error
 */ 
int eb_wait_syn(int *skip);

/**
 * @brief try to get bus for sending
 * \li wait SYN byte, send our ebus address and wait for minimun of ~4200 usec
 * \li read at least one byte and compare it with sent byte.
 * @param [out] *skip number skipped SYN bytes
 * @return 0 ok | -1 error | 1 max retry was reached
 */  
int eb_get_bus();


/**
 * @brief prepare send data
 * @param [in] *buf pointer to a byte array of send bytes
 * @param [in] buflen length of send bytes
 * @return none
 */
void eb_send_data_prepare(const unsigned char *buf, int buflen);

/**
 * @brief calculate crc of hex byte
 * @param [in] byte byte to calculate
 * @param [in] init_crc start value for calculation
 * @return new calculated crc byte from byte and init crc byte
 */
unsigned char eb_calc_crc_byte(unsigned char byte, unsigned char init_crc);



int
eb_serial_open(const char *dev, int *fd)
{
	int ret;
	struct termios newtio;

	sfd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	if (sfd < 0)
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

int
eb_serial_close()
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

int
eb_serial_send(const unsigned char *buf, int buflen)
{
	int ret;
	
	/* write msg to ebus device */
	ret = write(sfd, buf, buflen);
	tcflush(sfd, TCIOFLUSH);
	
	return ret;
}

int
eb_serial_recv(unsigned char *buf, int *buflen)
{
	//tcflush(sfd, TCIOFLUSH);
	/* read msg from ebus device */
	*buflen = read(sfd, buf, *buflen);

	if (*buflen < 0)
		return -1;

	return 0;
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
eb_htoi(const char *buf)
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


void
eb_print_result()
{
	int i;
	//fprintf(stdout, ">>> ");
	for (i = 0; i < recv_data.len; i++)
		fprintf(stdout, " %02x", recv_data.msg[i]);
	fprintf(stdout, "\n");
}


void
eb_esc(unsigned char *buf, int *buflen)
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
eb_unesc(unsigned char *buf, int *buflen)
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
eb_wait_syn(int *skip)
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
eb_get_bus()
{
	unsigned char buf[SERIAL_BUFSIZE];
	int buflen, ret, skip, retry;
	struct timeval tact, tlast, tdiff;

	skip = 0;
	retry = 0;
	
	do {
		ret = eb_wait_syn(&skip);
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
	ret = eb_get_bus();
	if (ret != 0)
		return -1;

	/* send message to slave */
	ret = eb_serial_send(&send_data.msg_esc[1], send_data.len_esc - 1);
	if (ret < 0)
		return -1;

	if (type == EBUS_MSG_BROADCAST) {
		/* SYN bus */
		ret = eb_serial_send(&syn, 1);
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
		ret = eb_serial_send(&syn, 1);	
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
			ret = eb_serial_send(&syn, 1);	
			if (ret < 0)
				return -1;
		
			return -1;
		}
		
	}

	if (type == EBUS_MSG_MASTER_MASTER) {
		val = ret;
	
		/* free bus */
		ret = eb_serial_send(&syn, 1);
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
			ret = eb_serial_send(&syn, 1);	
			if (ret < 0)
				return -1;
		
			return -1;
		}

		/* get data - dont reset buffer */
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
	ret = eb_serial_send(&syn, 1);;	
	if (ret < 0)
		return -1;
			
	return val;
}


int
eb_bcd_to_int(unsigned char src, int *tgt)
{
	if ((src & 0x0F) > 0x09 || ((src >> 4) & 0x0F) > 0x09) {
		*tgt = (int) (0xFF);
		return 0;
	} else {
		*tgt = (int) ( ( ((src & 0xF0) >> 4) * 10) + (src & 0x0F) );
		return 1;
	}
}

int
eb_int_to_bcd(int src, unsigned char *tgt)
{
	if (src > 99) {
		*tgt = (unsigned char) (0xFF);
		return 0;
	} else {
		*tgt = (unsigned char) ( ((src / 10) << 4) | (src % 10) );
		return 1;
	}
}


int
eb_d1b_to_int(unsigned char src, int *tgt)
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

int
eb_int_to_d1b(int src, unsigned char *tgt)
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


int
eb_d1c_to_float(unsigned char src, float *tgt)
{
	if (src > 0xC8) {
		*tgt = (float) (0xFF);
		return 0;
	} else {
		*tgt = (float) (src / 2.0);
		return 1;
	}
}

int
eb_float_to_d1c(float src, unsigned char *tgt)
{
	if (src < 0.0 || src > 100.0) {
		*tgt = (unsigned char) (0xFF);
		return 0;
	} else {
		*tgt = (unsigned char) (src * 2.0);
		return 1;
	}
}


int
eb_d2b_to_float(unsigned char src_lsb, unsigned char src_msb, float *tgt)
{
	if ((src_msb & 0x80) == 0x80) {
		*tgt = (float)
			(- ( ((unsigned char) (~ src_msb)) +
			(  ( ((unsigned char) (~ src_lsb)) + 1) / 256.0) ) );

		if (src_msb  == 0x80 && src_lsb == 0x00)
			return 0;
		else
			return -1;

	} else {
		*tgt = (float) (src_msb + (src_lsb / 256.0));
		return 1;
	}
}

int
eb_float_to_d2b(float src, unsigned char *tgt_lsb, unsigned char *tgt_msb)
{
	if (src < -127.999 || src > 127.999) {
		*tgt_msb = (unsigned char) (0x80);
		*tgt_lsb = (unsigned char) (0x00);
		return 0;
	} else {
		*tgt_lsb = (unsigned char)
					((src - ((unsigned char) src)) * 256.0);

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


int
eb_d2c_to_float(unsigned char src_lsb, unsigned char src_msb, float *tgt)
{
	if ((src_msb & 0x80) == 0x80) {
		*tgt = (float)
		(- ( ( ( ((unsigned char) (~ src_msb)) * 16.0) ) +
		     ( ( ((unsigned char) (~ src_lsb)) & 0xF0) >> 4) +
		   ( ( ( ((unsigned char) (~ src_lsb)) & 0x0F) +1 ) / 16.0) ) );

		if (src_msb  == 0x80 && src_lsb == 0x00)
			return 0;
		else
			return -1;

	} else {
		*tgt = (float) ( (src_msb * 16.0) + ((src_lsb & 0xF0) >> 4) +
						((src_lsb & 0x0F) / 16.0) );
		return 1;
	}
}

int
eb_float_to_d2c(float src, unsigned char *tgt_lsb, unsigned char *tgt_msb)
{
	if (src < -2047.999 || src > 2047.999) {
		*tgt_msb = (unsigned char) (0x80);
		*tgt_lsb = (unsigned char) (0x00);
		return 0;
	} else {
		*tgt_lsb =
		  ( ((unsigned char) ( ((unsigned char) src) % 16) << 4) +
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



unsigned char
eb_calc_crc_byte(unsigned char byte, unsigned char init_crc)
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
eb_calc_crc(const unsigned char *buf, int buflen)
{
	int i;
	unsigned char crc = 0;

	for (i = 0 ; i < buflen ; i++, buf++)
		crc = eb_calc_crc_byte(*buf, crc);

	return crc;
}

