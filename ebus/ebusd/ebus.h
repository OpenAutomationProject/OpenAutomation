/*
 * Copyright (C) Roland Jax 2012 <roland.jax@liwest.at>
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

/*
 * name     type             description              resolution   substitute value
 * BCD      CHAR                 0    ... +   99      1              FFh
 * DATA1b   SIGNED CHAR      - 127    ... +  127      1              80h
 * DATA1c   CHAR                 0    ... +  100      0,5            FFh
 * DATA2b   SIGNED INTEGER   - 127,99 ... +  127,99   1/256        8000h
 * DATA2c   SIGNED INTEGER   -2047,9  ... + 2047,9    1/16         8000h
 */

#define EBUS_SYN              0xAA
#define EBUS_SYN_ESC_A9       0xA9
#define EBUS_SYN_ESC_01       0x01
#define EBUS_SYN_ESC_ESC_00   0x00

#define EBUS_MSG_ACK          0x00
#define EBUS_MSG_BROADCAST    0xFE


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

/*
 * CRC calculation "CRC-8-WCDMA"
 * Polynom "x^8 + x^7 + x^4 + x^3 + x + 1"
 *
 * crc calculations by http://www.mikrocontroller.net/topic/75698
 */

unsigned char calc_crc_byte(unsigned char byte, unsigned char init_crc);
unsigned char calc_crc(unsigned char *bytes, int size);


#endif /* EBUS_H_ */
