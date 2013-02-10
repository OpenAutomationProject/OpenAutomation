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
 * @file ebus-decode.h
 * @brief ebus communication functions
 * @author roland.jax@liwest.at
 * @version 0.1
 */

/*
 * name     type             description              resolution   substitue
 * BCD      CHAR                 0    ... +   99      1              FFh
 * DATA1b   SIGNED CHAR      - 127    ... +  127      1              80h
 * DATA1c   CHAR                 0    ... +  100      0,5            FFh
 * DATA2b   SIGNED INTEGER   - 127,99 ... +  127,99   1/256        8000h
 * DATA2c   SIGNED INTEGER   -2047,9  ... + 2047,9    1/16         8000h
 */

#ifndef EBUS_DECODE_H_
#define EBUS_DECODE_H_


#include "ebus-common.h"

#define CMD_LINELEN          512
#define CMD_DELIMETER         12
#define CMD_FILELEN         1024

#define CMD_GET_SIZE_CLASS     5
#define CMD_GET_SIZE_CMD      30
#define CMD_GET_SIZE_S_ZZ      2
#define CMD_GET_SIZE_S_CMD     4
#define CMD_GET_SIZE_S_MSG    30
#define CMD_GET_SIZE_R_POS    10
#define CMD_GET_SIZE_R_TYPE    3
#define CMD_GET_SIZE_R_UNIT    6
#define CMD_GET_SIZE_COMMENT 256

#define CMD_SET_SIZE_CLASS     5
#define CMD_SET_SIZE_CMD      30
#define CMD_SET_SIZE_S_ZZ      2
#define CMD_SET_SIZE_S_CMD     4
#define CMD_SET_SIZE_S_MSG    30
#define CMD_SET_SIZE_D_POS    10
#define CMD_SET_SIZE_D_TYPE    3
#define CMD_SET_SIZE_D_UNIT    6
#define CMD_SET_SIZE_COMMENT 256


enum enum_cmd_type {GET, SET, CYC};

/**
 * @brief get commando structure
 */
struct cmd_get {
	int key; /**< internal number - do we need this ? */
	char class[CMD_GET_SIZE_CLASS+1]; /**< ci */
	char cmd[CMD_GET_SIZE_CMD+1]; /**< hydraulic */
	int s_type; /**< message type */
	char s_zz[CMD_GET_SIZE_S_ZZ+1]; /**< zz */ 
	char s_cmd[CMD_GET_SIZE_S_CMD+1]; /**< pb sb */
	int s_len; /**< number of send bytes */
	char s_msg[CMD_GET_SIZE_S_MSG+1]; /**< max 15 data bytes */
	int r_len; /**< number of receive bytes */
	char r_pos[CMD_GET_SIZE_R_POS+1]; /**< data position at answer string */
	char r_type[CMD_GET_SIZE_R_TYPE+1]; /**< data type */
	float r_fac; /**< facter */
	char r_unit[CMD_GET_SIZE_R_UNIT+1]; /**< unit of data like °C,...) */
	char com[CMD_GET_SIZE_COMMENT+1]; /**< just a comment */
};

/**
 * @brief set commando structure
 */
struct cmd_set {
	int key; /**< internal number - do we need this ? */
	char class[CMD_SET_SIZE_CLASS+1]; /**< ci */
	char cmd[CMD_SET_SIZE_CMD+1]; /**< hydraulic */
	int s_type; /**< message type */
	char s_zz[CMD_SET_SIZE_S_ZZ+1]; /**< zz */ 
	char s_cmd[CMD_SET_SIZE_S_CMD+1]; /**< pb sb */
	int s_len; /**< number of send bytes */
	char s_msg[CMD_SET_SIZE_S_MSG+1]; /**< max 15 data bytes */
	int d_len; /**< number of data bytes */
	char d_pos[CMD_SET_SIZE_D_POS+1]; /**< position at data string */
	char d_type[CMD_SET_SIZE_D_TYPE+1]; /**< data type */
	float d_fac; /**< facter */
	char d_unit[CMD_SET_SIZE_D_UNIT+1]; /**< unit of data like °C,...) */
	char com[CMD_SET_SIZE_COMMENT+1]; /**< just a comment */
};


/**
 * @brief encode msg
 * @param [in] id is index in command array of sent msg.
 * @param [out] *msg pointer to message array
 * @param [out] *buf pointer to decoded answer
 * @return 0 ok | -1 error at decode answer
 */
int eb_msg_send_cmd_encode(int id, unsigned char *msg, char *buf);

/**
 * @brief decode msg
 * @param [in] id is index in command array of sent msg.
 * @param [out] *msg pointer to message array
 * @param [out] *buf pointer to decoded answer
 * @return 0 ok | -1 error at decode answer
 */
int eb_msg_send_cmd_decode(int id, unsigned char *msg, char *buf);

/**
 * @brief prepare message string for given ebus cmd from array
 * @param [in] id is index in command array to sending msg.
 * @param [out] *msg pointer to message array
 * @param [out] *msglen pointer to message length
 * @param [out] *type pointer to message type
 * @return none
 */
void eb_msg_send_cmd_prepare_set(int id, char *msg, int *msglen, int *type, char *data);

/**
 * @brief prepare message string for given ebus cmd from array
 * @param [in] id is index in command array to sending msg.
 * @param [out] *msg pointer to message array
 * @param [out] *msglen pointer to message length
 * @param [out] *type pointer to message type
 * @return none
 */
void eb_msg_send_cmd_prepare_get(int id, char *msg, int *msglen, int *type);

/**
 * @brief handle send ebus cmd 
 * @param [in] id is index in command array to sending msg.
 * @param [in] type of msg (get/set)
 * @param [in] *data pointer to data bytes for set command
 * @param [out] *buf pointer to answer array
 * @param [out] *buflen point to answer length
 * @param [in] retry send msg
 * @return none
 */
void eb_msg_send_cmd(int id, int type, char *data, char *buf, int *buflen, int retry);

/**
 * @brief decode input data string with command data
 * @param [in] *class pointer to a ebus class array
 * @param [in] *cmd pointer to a ebus command array
 * @return 0-x id of found ebus command in array | -1 command not found
 */
int eb_msg_search_cmd_set(const char *class, const char *cmd);

/**
 * @brief decode input data string with command data
 * @param [in] *class pointer to a ebus class array
 * @param [in] *cmd pointer to a ebus command array
 * @return 0-x id of found ebus command in array | -1 command not found
 */
int eb_msg_search_cmd_get(const char *class, const char *cmd);

/**
 * @brief decode input data string with command data
 * @param [in] *buf pointer to a byte array
 * @param [in] *msgtype pointer which hold the msg type (get/set)
 * @return 0-x id of found ebus command in array | -1 command not found
 */
int eb_msg_search_cmd(char *buf, int *msgtype);



/**
 * @brief fill get command structure
 * @param [in] *tok pointer to given token
 * @return 0 ok | -1 error
 */
int eb_cmd_fill_get(const char *tok);

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
 * @brief calculate integer value of given hex byte
 * @param [in] *buf hex byte
 * @return integer value | -1 if given byte is no hex value
 */ 
int eb_htoi(const char *buf);



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
 * @brief calculate crc of hex byte
 * @param [in] byte byte to calculate
 * @param [in] init_crc start value for calculation
 * @return new calculated crc byte from byte and init crc byte
 */
unsigned char eb_calc_crc_byte(unsigned char byte, unsigned char init_crc);

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



#endif /* EBUS_DECODE_H_ */
