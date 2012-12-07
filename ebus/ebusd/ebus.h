/*
 * Author:  Jax Roland
 * Date:    09.11.2012
 * License: GPLv3
 */

#ifndef EBUS_H_
#define EBUS_H_

#define EBUS_SYN              0xAA
#define EBUS_SYN_ESC_A9       0xA9
#define EBUS_SYN_ESC_01       0x01
#define EBUS_SYN_ESC_ESC_00   0x00

#define EBUS_MSG_ACK          0x00
#define EBUS_MSG_BROADCAST    0xFE

#define BUFFER_SIZE    1024


int bcd_to_int(unsigned char source, int *target);
int int_to_bcd(int source, unsigned char *target);

int data1b_to_int(unsigned char source, int *target);
int int_to_data1b(int source, unsigned char *target);

int data1c_to_float(unsigned char source, float *target);
int float_to_data1c(float source, unsigned char *target);

int data2b_to_float(unsigned char source_lsb, unsigned char source_msb, float *target);
int float_to_data2b(float source, unsigned char *target_lsb, unsigned char *target_msb);

int data2c_to_float(unsigned char source_lsb, unsigned char source_msb, float *target);
int float_to_data2c(float source, unsigned char *target_lsb, unsigned char *target_msb);

unsigned char calc_crc_byte(unsigned char byte, unsigned char init_crc);
unsigned char calc_crc(unsigned char *bytes, int size);


#endif /* EBUS_H_ */
