/*
 * Author:  Jax Roland
 * Date:    09.11.2012
 * License: GPLv3
 */

#ifndef EBUS_H_
#define EBUS_H_

// defines
#define EBUS_SYN              0xAA
#define EBUS_SYN_ESC_A9       0xA9
#define EBUS_SYN_ESC_01       0x01
#define EBUS_SYN_ESC_ESC_00   0x00

#define EBUS_MSG_ACK          0x00
#define EBUS_MSG_BROADCAST    0xFE

// buffer size
#define BUFFER_SIZE    1024


// prototypes

int bcd_to_int(unsigned char ucSource, int *piTarget);
int int_to_bcd(int iSource, unsigned char *pucTarget);

int data1b_to_int(unsigned char ucDATA1b, int *piTarget);
int int_to_data1b(int iSource, unsigned char *pucTarget);

int data1c_to_float(unsigned char ucSource, float *pfTarget);
int float_to_data1c(float fSource, unsigned char *pucTarget);

int data2b_to_float(unsigned char ucSourceLSB, unsigned char ucSourceMSB, float *pfTarget);
int float_to_data2b(float fSource, unsigned char *pucTargetMSB, unsigned char *pucTargetLSB);

int data2c_to_float(unsigned char ucSourceLSB, unsigned char ucSourceMSB, float *pfTarget);
int float_to_data2c(float fSource, unsigned char *pucTargetMSB, unsigned char *pucTargetLSB);

unsigned char calc_crc_byte(unsigned char ucByte, unsigned char ucInitCRC);
unsigned char calc_crc(unsigned char *pucData, int iDataSize);


#endif /* EBUS_H_ */
