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
 * @file ebus.h
 * @brief ebus communication functions
 * @author roland.jax@liwest.at
 *
 * @todo missing functions for date, time and ascii
 *
 * @version 0.1
 */

#ifndef EBUS_H_
#define EBUS_H_

#include <sys/time.h>

/*
 * name     type             description              resolution   substitue
 * BCD      CHAR                 0    ... +   99      1              FFh
 * DATA1b   SIGNED CHAR      - 127    ... +  127      1              80h
 * DATA1c   CHAR                 0    ... +  100      0,5            FFh
 * DATA2b   SIGNED INTEGER   - 127,99 ... +  127,99   1/256        8000h
 * DATA2c   SIGNED INTEGER   -2047,9  ... + 2047,9    1/16         8000h
 */

#define EBUS_SYN                0xAA

#define EBUS_QQ                 0xFF
#define EBUS_MAX_RETRY          3
#define EBUS_SKIP_ACK           1
#define EBUS_MAX_WAIT           4000

#define EBUS_MSG_BROADCAST      1
#define EBUS_MSG_MASTER_MASTER  2
#define EBUS_MSG_MASTER_SLAVE   3

#define SERIAL_DEVICE           "/dev/ttyUSB0"
#define SERIAL_BAUDRATE         B2400
#define SERIAL_BUFSIZE          100


/**
 * @brief Open a serial device in raw mode. 1 Byte is then minimum input length.
 * @param [in] *dev serial device
 * @param [out] *fd file descriptor from opened serial device
 * @return 0 ok | -1 error
 */
int serial_open(const char *dev, int *fd);

/**
 * @brief close a serial device and set settings to default.
 * @return 0 ok | -1 error
 */
int serial_close();

/**
 * @brief set own ebus address
 * @param [in] src ebus address
 * @return none
 */
void eb_set_qq(unsigned char src);

/**
 * @brief set max wait time between send and receive of own address (QQ)
 * @param [in] usec wait time in usec
 * @return none
 */
void eb_set_max_wait(long usec);

/**
 * @brief set number of retry to get bus
 * @param [in] retry
 * @return none
 */
void eb_set_max_retry(int retry);

/**
 * @brief set number of skipped SYN if an error occurred while getting the bus
 * @param [in] skip is only the start value (skip = skip_ack + retry;)
 * @return none
 */
void eb_set_skip_ack(int skip);


/**
 * @brief calculate integer value of given hex byte
 * @param [in] *buf hex byte
 * @return integer value | -1 if given byte is no hex value
 */ 
int eb_htoi(const char *buf);

/**
 * @brief print received results in a specific format
 * @return none
 */
void eb_print_result();


/**
 * @brief handle sending ebus data
 *
 * \li Given input data will be prepared (escape, crc) and send to ebus.
 * \li Answer will be received, prepared (unescape, crc) and return as result.
 * 
 * @param [in] *buf pointer to a byte array
 * @param [in] buflen length of byte array
 * @param [in] type is the type of message to send
 * @return 0 ok | 1 neg. ACK from Slave | -1 error
 */ 
int eb_send_data(const unsigned char *buf, int buflen, int type);

/**
 * @brief scan ebus for cycle ebus data
 *
 * \li Given input data will be prepared (escape, crc) and send to ebus.
 * \li Answer will be received, prepared (unescape, crc) and return result.
 * 
 * @param [out] *buf pointer to a byte array
 * @param [out] buflen length of byte array
 * @param [out] *msg pointer to a byte array with full ebus msg
 * @param [out] msglen length of byte array with full ebus msg
 * @return 0 ok | 1 msg complete | -1 error
 */ 
int eb_scan_bus(unsigned char *buf, int *buflen, unsigned char *msg, int *msglen);


/**
 * @brief print received results in a specific format
 * @param [out] *buf pointer to a byte array
 * @param [out] *buflen length of byte array
 * @return none
 */
void eb_esc(unsigned char *buf, int *buflen);

/**
 * @brief print received results in a specific format
 * @param [out] *buf pointer to a byte array
 * @param [out] *buflen length of byte array
 * @return none
 */
void eb_unesc(unsigned char *buf, int *buflen);


/**
 * @brief convert bcd hex byte to int
 * @param [in] src bcd hex byte
 * @param [out] *tgt pointer to int value
 * @return 0 substitute value | 1 positive value
 */ 
int eb_bcd_to_int(unsigned char src, int *tgt);

/**
 * @brief convert int to bcd hex byte
 * @param [in] src int value
 * @param [out] *tgt pointer to hex byte
 * @return 0 substitute value | 1 positive value
 */ 
int eb_int_to_bcd(int src, unsigned char *tgt);


/**
 * @brief convert data1b hex byte to int
 * @param [in] src data1b hex byte
 * @param [out] *tgt pointer to int value
 * @return 0 substitute value | 1 positive value | -1 negative value
 */ 
int eb_d1b_to_int(unsigned char src, int *tgt);

/**
 * @brief convert int to data1b hex byte
 * @param [in] src int value
 * @param [out] *tgt pointer to data1b hex byte
 * @return 0 substitute value | 1 positive value | -1 negative value
 */ 
int eb_int_to_d1b(int src, unsigned char *tgt);


/**
 * @brief convert data1c hex byte to float
 * @param [in] src data1c hex byte
 * @param [out] *tgt pointer to float value
 * @return 0 substitute value | 1 positive value | -1 negative value
 */ 
int eb_d1c_to_float(unsigned char src, float *tgt);

/**
 * @brief convert float to data1c hex byte
 * @param [in] src float value
 * @param [out] *tgt pointer to data1c hex byte
 * @return 0 substitute value | 1 positive value | -1 negative value
 */ 
int eb_float_to_d1c(float src, unsigned char *tgt);


/**
 * @brief convert data2b hex bytes to float
 * @param [in] src_lsb least significant data2b hex byte
 * @param [in] src_msb most significant data2b hex byte
 * @param [out] *tgt pointer to float value
 * @return 0 substitute value | 1 positive value | -1 negative value
 */ 
int eb_d2b_to_float(unsigned char src_lsb, unsigned char src_msb, float *tgt);

/**
 * @brief convert float to data2b hex bytes
 * @param [in] src float value
 * @param [out] *tgt_lsb pointer to least significant data2b hex byte
 * @param [out] *tgt_msb pointer to most significant data2b hex byte
 * @return 0 substitute value | 1 positive value | -1 negative value
 */ 
int eb_float_to_d2b(float src, unsigned char *tgt_lsb, unsigned char *tgt_msb);


/**
 * @brief convert data2c hex bytes to float
 * @param [in] src_lsb least significant data2c hex byte
 * @param [in] src_msb most significant data2c hex byte
 * @param [out] *tgt pointer to float value
 * @return 0 substitute value | 1 positive value | -1 negative value
 */ 
int eb_d2c_to_float(unsigned char src_lsb, unsigned char src_msb, float *tgt);

/**
 * @brief convert float to data2c hex bytes
 * @param [in] src float value
 * @param [out] *tgt_lsb pointer to least significant data2c hex byte
 * @param [out] *tgt_msb pointer to most significant data2c hex byte
 * @return 0 substitute value | 1 positive value | -1 negative value
 */ 
int eb_float_to_d2c(float src, unsigned char *tgt_lsb, unsigned char *tgt_msb);


/**
 * @brief calculate crc of given hex array
 *
 * \li crc calculation "CRC-8-WCDMA" with Polynom "x^8+x^7+x^4+x^3+x+1"
 * \li crc calculations by http://www.mikrocontroller.net/topic/75698
 *
 * @param [in] *bytes pointer to a byte array
 * @param [in] size length of given bytes
 * @return calculated crc byte
 */
unsigned char eb_calc_crc(const unsigned char *bytes, int size);

#endif /* EBUS_H_ */
