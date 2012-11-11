/*
 * Author:  Jax Roland
 * Date:    09.11.2012
 * License: GPLv3
 */

#include "ebus.h"

#ifndef VAILLANT_H_
#define VAILLANT_H_

// defines
#define VAILLANT_QQ_10    0x10
#define VAILLANT_ZZ_0A    0x0a
#define VAILLANT_ZZ_ED    0xed
#define VAILLANT_PB_B5    0xb5


// typedefs

typedef unsigned char QQ_t;
typedef unsigned char ZZ_t;
typedef unsigned char PB_t;
typedef unsigned char SB_t;
typedef unsigned char NN_t;
typedef unsigned char TB_t;
typedef unsigned char CRC_t;
typedef unsigned char ACK_t;

typedef struct MSG_HEAD_t {
	QQ_t QQ;
	ZZ_t ZZ;
	PB_t PB;
	SB_t SB;
	NN_t NN;
	TB_t TB;
} MSG_HEAD_t;

typedef struct MSG_t {
	struct MSG_HEAD_t *pstMsgHead;
	unsigned char *pucMsgData;
	CRC_t MsgCRC;
	ACK_t MsgACK;
} MSG_t;

extern int g_iVerbose;
extern int g_iFlagDecodeMsg;

// prototypes
void print_ebus_msg_head(struct MSG_HEAD_t *pstMsgHead);

int is_valid(unsigned char ucByte, const unsigned char *pucData, int iDataSize);

void print_ebus_msg(unsigned char *pucEbusMsg, int iEbusMsgSize);

void decode_ebus_msg(unsigned char *pucDataBuffer, int iDataBufferSize);

#endif /* VAILLANT_H_ */
