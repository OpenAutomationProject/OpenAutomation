/*
 * Author:  Jax Roland
 * Date:    09.11.2012
 * License: GPLv3
 */

#include <stdio.h>

#include "ebus.h"

/*
 * Name     Typ              Beschreibung             Aufloesung   Ersatzwert
 * BCD      CHAR                 0    ... +   99      1              FFh
 */

int bcd_to_int(unsigned char source, int *target)
{
	if ((source & 0x0F) > 0x09 || ((source >> 4) & 0x0F) > 0x09) {
		*target = (int) (0xFF);
		return 0;
	}
	else {
		*target = (int) ( ( ((source & 0xF0) >> 4) * 10) + (source & 0x0F) );
		return 1;
	}
}

int int_to_bcd(int source, unsigned char *target)
{
	if (source > 99) {
		*target = (unsigned char) (0xFF);
		return 0;
	}
	else {
		*target = (unsigned char) ( ((source / 10) << 4) | (source % 10) );
		return 1;
	}
}


/*
 * Name     Typ              Beschreibung             Aufloesung   Ersatzwert
 * DATA1b   SIGNED CHAR      - 127    ... +  127      1              80h
 */

int data1b_to_int(unsigned char source, int *target)
{
	if ((source & 0x80) == 0x80) {
		*target = (int) (- ( ((unsigned char) (~ source)) + 1) );

		if (*target  == -0x80) {
			return 0;
		}
		else {
			return -1;
		}
	} else {
		*target = (int) (source);
		return 1;
	}
}

int int_to_data1b(int source, unsigned char *target)
{
	if (source < -127 || source > 127) {
		*target = (unsigned char) (0x80);
		return 0;
	}
	else {
		if (source >= 0) {
			*target = (unsigned char) (source);
			return 1;
		}
		else {
			*target = (unsigned char) (- (~ (source - 1) ) );
			return -1;
		}
	}
}


/*
 * Name     Typ              Beschreibung             Aufloesung   Ersatzwert
 * DATA1c   CHAR                 0    ... +  100      0,5            FFh
 */

int data1c_to_float(unsigned char source, float *target)
{
	if (source > 0xC8) {
		*target = (float) (0xFF);
		return 0;
	}
	else {
		*target = (float) (source / 2.0);
		return 1;
	}
}

int float_to_data1c(float source, unsigned char *target)
{
	if (source < 0.0 || source > 100.0) {
		*target = (unsigned char) (0xFF);
		return 0;
	}
	else {
		*target = (unsigned char) (source * 2.0);
		return 1;
	}
}


/*
 * Name     Typ              Beschreibung             Aufloesung   Ersatzwert
 * DATA2b   SIGNED INTEGER   - 127,99 ... +  127,99   1/256        8000h
 */

int data2b_to_float(unsigned char source_lsb, unsigned char source_msb, float *target)
{
	if ((source_msb & 0x80) == 0x80) {
		*target = (float) (-   ( ((unsigned char) (~ source_msb)) +
		                     ( ( ((unsigned char) (~ source_lsb)) + 1) / 256.0) ) );

		if (source_msb  == 0x80 && source_lsb == 0x00) {
			return 0;
		}
		else {
			return -1;
		}
	}
	else {
		*target = (float) (source_msb + (source_lsb / 256.0));
		return 1;
	}
}

int float_to_data2b(float source, unsigned char *target_lsb, unsigned char *target_msb)
{
	if (source < -127.999 || source > 127.999) {
		*target_msb = (unsigned char) (0x80);
		*target_lsb = (unsigned char) (0x00);
		return 0;
	}
	else {
		*target_lsb = (unsigned char) ((source - ((unsigned char) source)) * 256.0);

		if (source < 0.0 && *target_lsb != 0x00) {
			*target_msb = (unsigned char) (source - 1);
		}
		else {
			*target_msb = (unsigned char) (source);
		}

		if (source >= 0.0) {
			return 1;
		}
		else {
			return -1;
		}
	}
}


/*
 * Name     Typ              Beschreibung             Aufloesung   Ersatzwert
 * DATA2c   SIGNED INTEGER   -2047,9  ... + 2047,9    1/16         8000h
 */

int data2c_to_float(unsigned char source_lsb, unsigned char source_msb, float *target)
{
	if ((source_msb & 0x80) == 0x80) {
		*target = (float) (- ( ( ( ((unsigned char) (~ source_msb)) * 16.0) ) +
		                       ( ( ((unsigned char) (~ source_lsb)) & 0xF0) >> 4) +
		                     ( ( ( ((unsigned char) (~ source_lsb)) & 0x0F) +1 ) / 16.0) ) );

		if (source_msb  == 0x80 && source_lsb == 0x00) {
			return 0;
		}
		else {
			return -1;
		}
	}
	else {
		*target = (float) ( (source_msb * 16.0) + ((source_lsb & 0xF0) >> 4) + ((source_lsb & 0x0F) / 16.0) );
		return 1;
	}
}

int float_to_data2c(float source, unsigned char *target_lsb, unsigned char *target_msb)
{
	if (source < -2047.999 || source > 2047.999) {
		*target_msb = (unsigned char) (0x80);
		*target_lsb = (unsigned char) (0x00);
		return 0;
	}
	else {
		*target_lsb = ( ((unsigned char) ( ((unsigned char) source) % 16) << 4) +
		                ((unsigned char) ( (source - ((unsigned char) source)) * 16.0)) );

		if (source < 0.0 && *target_lsb != 0x00) {
			*target_msb = (unsigned char) ((source / 16.0) - 1);
		}
		else {
			*target_msb = (unsigned char) (source / 16.0);
		}

		if (source >= 0.0) {
			return 1;
		}
		else {
			return -1;
		}
	}
}



/*
 * CRC Calculation "CRC-8-WCDMA"
 * Polynom "x^8 + x^7 + x^4 + x^3 + x + 1"
 */

unsigned char calc_crc_byte(unsigned char byte, unsigned char init_crc)
{
	unsigned char crc;
	unsigned char polynom;
	int i;

	crc = init_crc;

	for (i = 0; i < 8; i++) {

		if (crc & 0x80) {
			polynom = (unsigned char) 0x9B;
		}
		else {
			polynom = (unsigned char) 0;
		}

		crc = (unsigned char) ((crc & ~0x80) << 1);

		if (byte & 0x80) {
			crc = (unsigned char) (crc | 1);
		}

		crc = (unsigned char) (crc ^ polynom);
		byte = (unsigned char) (byte << 1);
	}
	return crc;
}

unsigned char calc_crc(unsigned char *bytes, int size)
{
	int i;
	unsigned char crc = 0;

	for( i = 0 ; i < size ; i++, bytes++ ) {
		crc = calc_crc_byte(*bytes, crc);
	}
	return crc;
}

