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

#ifndef EBUS_H_
#define EBUS_H_

#include <sys/time.h>

/*
 * name     type             description              resolution   substitute value
 * BCD      CHAR                 0    ... +   99      1              FFh
 * DATA1b   SIGNED CHAR      - 127    ... +  127      1              80h
 * DATA1c   CHAR                 0    ... +  100      0,5            FFh
 * DATA2b   SIGNED INTEGER   - 127,99 ... +  127,99   1/256        8000h
 * DATA2c   SIGNED INTEGER   -2047,9  ... + 2047,9    1/16         8000h
 */

#define EBUS_SYN                0xAA

#define EBUS_QQ                 0xFF
#define EBUS_MAX_WAIT           4000
#define EBUS_MAX_RETRY          3
#define EBUS_SKIP_ACK           1

#define EBUS_MSG_BROADCAST      1
#define EBUS_MSG_MASTER_MASTER  2
#define EBUS_MSG_MASTER_SLAVE   3

#define SERIAL_DEVICE           "/dev/ttyUSB0"
#define SERIAL_BAUDRATE         B2400
#define SERIAL_BUFSIZE          100

/*
 * return value
 * -1 error
 *  0 ok
 */
int serial_open(const char *dev, int *fd);
int serial_close();

void ebus_set_qq(unsigned char src);
void ebus_set_max_wait(long usec);
void ebus_set_max_retry(int retry);
void ebus_set_skip_ack(int skip);

/*
 * return value
 * int value for hex byte
 * -1 if not
 */
int ebus_htoi(const char *buf);

void ebus_print_result();

/*
 * return value
 * -1 error
 *  0 ACK
 *  1 NAK
 */
int ebus_send_data(const unsigned char *buf, int buflen, int type);

void ebus_esc(unsigned char *buf, int *buflen);
void ebus_unesc(unsigned char *buf, int *buflen);

/*
 *  return value
 *  0 - substitute value
 *  1 - positive value
 */
int ebus_bcd_to_int(unsigned char src, int *tgt);
int ebus_int_to_bcd(int src, unsigned char *tgt);

/*
 *  return value
 * -1 - negative value
 *  0 - substitute value
 *  1 - positive value
 */
int ebus_data1b_to_int(unsigned char src, int *tgt);
int ebus_int_to_data1b(int src, unsigned char *tgt);

int ebus_data1c_to_float(unsigned char src, float *tgt);
int ebus_float_to_data1c(float src, unsigned char *tgt);

int ebus_data2b_to_float(unsigned char src_lsb, unsigned char src_msb, float *tgt);
int ebus_float_to_data2b(float src, unsigned char *tgt_lsb, unsigned char *tgt_msb);

int ebus_data2c_to_float(unsigned char src_lsb, unsigned char src_msb, float *tgt);
int ebus_float_to_data2c(float src, unsigned char *tgt_lsb, unsigned char *tgt_msb);

/*
 * CRC calculation "CRC-8-WCDMA"
 * Polynom "x^8 + x^7 + x^4 + x^3 + x + 1"
 *
 * crc calculations by http://www.mikrocontroller.net/topic/75698
 *
 * return value
 * calculated crc byte
 */

unsigned char ebus_calc_crc(const unsigned char *bytes, int size);

#endif /* EBUS_H_ */
