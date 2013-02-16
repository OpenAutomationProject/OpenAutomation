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
 * @file ebus-bus.h
 * @brief ebus communication functions
 * @author roland.jax@liwest.at
 * @version 0.1
 */

#ifndef EBUS_BUS_H_
#define EBUS_BUS_H_



#include <sys/time.h>

#include "ebus-common.h"



#define CMD_LINELEN        512
#define CMD_FILELEN        1024

#define CMD_SIZE_TYPE      3
#define CMD_SIZE_CLASS     5
#define CMD_SIZE_CMD       30
#define CMD_SIZE_COM       256
#define CMD_SIZE_S_ZZ      2
#define CMD_SIZE_S_CMD     4
#define CMD_SIZE_S_MSG     32
#define CMD_SIZE_D_SUB     20
#define CMD_SIZE_D_POS     10
#define CMD_SIZE_D_TYPE    3
#define CMD_SIZE_D_UNIT    6
#define CMD_SIZE_D_VALID   30
#define CMD_SIZE_D_COM     256

#define CMD_DATA_SIZE      256



//~ enum enum_cmd_type {GET, SET, CYC};

/**
 * @brief element structure
 */
struct element {
	char d_sub[CMD_SIZE_D_SUB+1]; /**< pin1 */
	char d_pos[CMD_SIZE_D_POS+1]; /**< data position at bytes */
	char d_type[CMD_SIZE_D_TYPE+1]; /**< data type */
	float d_fac; /**< facter */
	char d_unit[CMD_SIZE_D_UNIT+1]; /**< unit of data like °C,...) */
	char d_valid[CMD_SIZE_D_VALID+1]; /**< valid data */
	char d_com[CMD_SIZE_D_COM+1]; /**< just a comment */
};

/**
 * @brief commands structure
 */
struct commands {
	int key; /**< internal number - do we need this ? */
	char type[CMD_SIZE_TYPE+1]; /**< type of message */
	char class[CMD_SIZE_CLASS+1]; /**< ci */
	char cmd[CMD_SIZE_CMD+1]; /**< hydraulic */
	char com[CMD_SIZE_COM+1]; /**< just a comment */	
	int s_type; /**< message type */
	char s_zz[CMD_SIZE_S_ZZ+1]; /**< zz */ 
	char s_cmd[CMD_SIZE_S_CMD+1]; /**< pb sb */
	int s_len; /**< number of send bytes */
	char s_msg[CMD_SIZE_S_MSG+1]; /**< max 15 data bytes */
	int d_elem; /**< number of elements */
	struct element *elem; /**< pointer of array with elements */
};

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



/**
 * @brief set rawdump
 * @param [in] YES | NO
 * @return none
 */
void eb_set_rawdump(int dump);

/**
 * @brief set showraw
 * @param [in] YES | NO
 * @return none
 */
void eb_set_showraw(int show);

/**
 * @brief set own ebus address
 * @param [in] src ebus address
 * @return none
 */
void eb_set_qq(unsigned char src);

/**
 * @brief set number of retry to get bus
 * @param [in] retry
 * @return none
 */
void eb_set_get_retry(int retry);

/**
 * @brief set number of skipped SYN if an error occurred while getting the bus
 * @param [in] skip is only the start value (skip = skip_ack + retry;)
 * @return none
 */
void eb_set_skip_ack(int skip);

/**
 * @brief set max wait time between send and receive of own address (QQ)
 * @param [in] usec wait time in usec
 * @return none
 */
void eb_set_max_wait(long usec);

/**
 * @brief set number of retry to send message
 * @param [in] retry
 * @return none
 */
void eb_set_send_retry(int retry);



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
 * @brief decode input data string with command data
 * @param [in] *class pointer to a ebus class array
 * @param [in] *cmd pointer to a ebus command array
 * @return 0-x id of found ebus command in array | -1 command not found
 */
int eb_msg_search_cmd_id(const char *type, const char *class, const char *cmd);

/**
 * @brief decode input data string with command data
 * @param [in] *buf pointer to a byte array
 * @param [out] *data pointer to data array for passing to decode/encode
 * @return 0-x id of found ebus command in array | -1 command not found
 */
int eb_msg_search_cmd(char *buf, char *data);



/**
 * @brief decode given element
 * @param [in] id is index in command array of sent msg.
 * @param [in] elem is index of data position
 * @param [in] *data pointer to data bytes for encode
 * @param [out] *msg pointer to message array
 * @param [out] *buf pointer to decoded answer
 * @return 0 ok | -1 error at decode
 */
int eb_cmd_encode_value(int id, int elem, char *data, unsigned char *msg, char *buf);

/**
 * @brief encode msg
 * @param [in] id is index in command array of sent msg.
 * @param [in] *data pointer to data bytes for encode
 * @param [out] *msg pointer to message array
 * @param [out] *buf pointer to decoded answer
 * @return 0 ok | -1 error at encode
 */
int eb_cmd_encode(int id, char *data, unsigned char *msg, char *buf);

/**
 * @brief decode given element
 * @param [in] id is index in command array of sent msg.
 * @param [in] elem is index of data position
 * @param [out] *msg pointer to message array
 * @param [out] *buf pointer to decoded answer
 * @return 0 ok | -1 error at decode
 */
int eb_cmd_decode_value(int id, int elem, unsigned char *msg, char *buf);

/**
 * @brief decode msg
 * @param [in] id is index in command array of sent msg.
 * @param [in] *data pointer to data bytes for decode
 * @param [out] *msg pointer to message array
 * @param [out] *buf pointer to decoded answer
 * @return 0 ok | -1 error at decode
 */
int eb_cmd_decode(int id, char *data, unsigned char *msg, char *buf);

/**
 * @brief fill command structure
 * @param [in] *tok pointer to given token
 * @return 0 ok | -1 error
 */
int eb_cmd_fill(const char *tok);

/**
 * @brief prevent buffer overflow
 * 
 * number of tokens must be >= number of columns - 1 of file
 * 
 * @param [in] *buf pointer to a byte array
 * @param [in] delimeter
 * @return number of found delimeters
 */ 
int eb_cmd_num_c(const char *buf, const char c);

/**
 * @brief set cfgdir address
 * @param [in] *file pointer to configuration file with *.csv
 * @return 0 ok | -1 error | -2  read token error
 */
int eb_cmd_file_read(const char *file);

/**
 * @brief get all files with given extension from given configuration directory
 * @param [in] *cfgdir pointer to configuration directory
 * @param [in] *suffix pointer to given extension
 * @return 0 ok | -1 error | 1 read file error | 2 no command files found
 */
int eb_cmd_dir_read(const char *cfgdir, const char *extension);

/**
 * @brief free mem for ebus commands
 * @return none
 */
void eb_cmd_dir_free(void);



/**
 * @brief Open a file in write mode.
 * @param [in] *file device
 * @return 0 ok | -1 error
 */
int eb_raw_file_open(const char *file);

/**
 * @brief close a file.
 * @return 0 ok | -1 error
 */
int eb_raw_file_close(void);

/**
 * @brief write bytes into file
 * @param [in] *buf pointer to a byte array
 * @param [in] buflen length of byte array
 * @return 0 ok | -1 error
 */
int eb_raw_file_write(const unsigned char *buf,  int buflen);

/**
 * @brief print received data with hex format
 * @param [in] *buf pointer to a byte array of received bytes
 * @param [in] buflen length of received bytes
 * @return none
 */
void eb_raw_print_hex(const unsigned char *buf, int buflen);



/**
 * @brief Open serial device in raw mode. 1 Byte is then minimum input length.
 * @param [in] *dev serial device
 * @param [out] *fd file descriptor from opened serial device
 * @return 0 ok | -1 error
 */
int eb_serial_open(const char *dev, int *fd);

/**
 * @brief close serial device and set settings to default.
 * @return 0 ok | -1 error
 */
int eb_serial_close(void);

/**
 * @brief send bytes to serial device
 * @param [in] *buf pointer to a byte array
 * @param [in] buflen length of byte array
 * @return 0 ok | -1 error
 */
int eb_serial_send(const unsigned char *buf, int buflen);

/**
 * @brief receive bytes from serial device
 * @param [out] *buf pointer to a byte array received bytes
 * @param [out] *buflen length of received bytes
 * @return 0 ok | -1 error | -2 received data > BUFFER
 */
int eb_serial_recv(unsigned char *buf, int *buflen);



/**
 * @brief print received results in a specific format
 * @return none
 */
void eb_print_result(void);


/**
 * @brief get received data
 * @param [out] *buf pointer to a byte array of received bytes
 * @param [out] *buflen length of received bytes
 * @return none
 */
void eb_get_recv_data(unsigned char *buf, int *buflen);

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
int eb_wait_bus_syn(int *skip);

/**
 * @brief try to get bus for sending
 * \li wait SYN byte, send our ebus address and wait for minimun of ~4200 usec
 * \li read at least one byte and compare it with sent byte.
 * @return 0 ok | -1 error | 1 max retry was reached
 */  
int eb_wait_bus(void);

/**
 * @brief free bus after syn was sent
 * @return 0 ok | -1 error
 */  
int eb_free_bus(void);



/**
 * @brief prepare send data
 * @param [in] *buf pointer to a byte array of send bytes
 * @param [in] buflen length of send bytes
 * @return none
 */
void eb_send_data_prepare(const unsigned char *buf, int buflen);

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
 * @brief prepare message string for given ebus cmd from array
 * @param [in] id is index in command array to sending msg.
 * @param [in] *data pointer to data bytes for decode/encode
 * @param [out] *msg pointer to message array
 * @param [out] *msglen pointer to message length
 * @param [out] *msgtype pointer to message type 
 * @return none
 */
void eb_send_cmd_prepare(int id, char *data, char *msg, int *msglen, int *msgtype, char *buf);

/**
 * @brief handle send ebus cmd 
 * @param [in] id is index in command array to sending msg.
 * @param [in] *data pointer to data bytes for decode/encode
 * @param [out] *buf pointer to answer array
 * @param [out] *buflen point to answer length
 * @return none
 */
void eb_send_cmd(int id, char *data, char *buf, int *buflen);



/**
 * @brief handle reading cycle ebus data
 * @param [out] *buf pointer to a byte array
 * @param [out] *buflen length of byte array
 * @return 0-x length of collected msg | -1 error
 */
int eb_cyc_data_recv(unsigned char *buf, int *buflen);



#endif /* EBUS_BUS_H_ */
