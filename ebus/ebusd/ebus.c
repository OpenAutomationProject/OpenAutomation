/*
 * Author:  Jax Roland
 * Date:    09.11.2012
 * License: GPLv3
 */

#include <stdio.h>

#include "ebus.h"

// Name     Typ              Beschreibung             Aufloesung   Ersatzwert
// BCD      CHAR                 0    ... +   99      1              FFh

int bcd_to_int(unsigned char ucSource, int *piTarget)
{
	if ((ucSource & 0x0F) > 0x09 || ((ucSource >> 4) & 0x0F) > 0x09) {
		*piTarget = (int) (0xFF);
		return 0;
	}
	else {
		*piTarget = (int) ( ( ((ucSource & 0xF0) >> 4) * 10) + (ucSource & 0x0F) );
		return 1;
	}
}

int int_to_bcd(int iSource, unsigned char *pucTarget)
{
	if (iSource > 99) {
		*pucTarget = (unsigned char) (0xFF);
		return 0;
	} else {
		*pucTarget = (unsigned char) ( ((iSource / 10) << 4) | (iSource % 10) );
		return 1;
	}
}


// Name     Typ              Beschreibung             Aufloesung   Ersatzwert
// DATA1b   SIGNED CHAR      - 127    ... +  127      1              80h

int data1b_to_int(unsigned char ucDATA1b, int *piTarget)
{
	if ((ucDATA1b & 0x80) == 0x80) {
		*piTarget = (int) (- ( ((unsigned char) (~ ucDATA1b)) + 1) );

		if (*piTarget  == -0x80)
			return 0;
		else
			return -1;
	} else {
		*piTarget = (int) (ucDATA1b);
		return 1;
	}
}

int int_to_data1b(int iSource, unsigned char *pucTarget)
{
	if (iSource < -127 || iSource > 127) {
		*pucTarget = (unsigned char) (0x80);
		return 0;
	} else {
		if (iSource >= 0) {
			*pucTarget = (unsigned char) (iSource);
			return 1;
		} else {
			*pucTarget = (unsigned char) (- (~ (iSource - 1) ) );
			return -1;
		}
	}
}


// Name     Typ              Beschreibung             Aufloesung   Ersatzwert
// DATA1c   CHAR                 0    ... +  100      0,5            FFh

int data1c_to_float(unsigned char ucSource, float *pfTarget)
{
	if (ucSource > 0xC8) {
		*pfTarget = (float) (0xFF);
		return 0;
	} else {
		*pfTarget = (float) (ucSource / 2.0);
		return 1;
	}
}

int float_to_data1c(float fSource, unsigned char *pucTarget)
{
	if (fSource < 0.0 || fSource > 100.0) {
		*pucTarget = (unsigned char) (0xFF);
		return 0;
	} else {
		*pucTarget = (unsigned char) (fSource * 2.0);
		return 1;
	}
}


// Name     Typ              Beschreibung             Aufloesung   Ersatzwert
// DATA2b   SIGNED INTEGER   - 127,99 ... +  127,99   1/256        8000h

int data2b_to_float(unsigned char ucSourceLSB, unsigned char ucSourceMSB, float *pfTarget)
{
	if ((ucSourceMSB & 0x80) == 0x80) {
		*pfTarget = (float) (-   ( ((unsigned char) (~ ucSourceMSB)) +
		                       ( ( ((unsigned char) (~ ucSourceLSB)) + 1) / 256.0) ) );

		if (ucSourceMSB  == 0x80 && ucSourceLSB == 0x00)
			return 0;
		else
			return -1;
	} else {
		*pfTarget = (float) (ucSourceMSB + (ucSourceLSB / 256.0));
		return 1;
	}
}

int float_to_data2b(float fSource, unsigned char *pucTargetMSB, unsigned char *pucTargetLSB)
{
	if (fSource < -127.999 || fSource > 127.999) {
		*pucTargetMSB = (unsigned char) (0x80);
		*pucTargetLSB = (unsigned char) (0x00);
		return 0;
	} else {
		*pucTargetLSB = (unsigned char) ((fSource - ((unsigned char) fSource)) * 256.0);

		if (fSource < 0.0 && *pucTargetLSB != 0x00)
			*pucTargetMSB = (unsigned char) (fSource - 1);
		else
			*pucTargetMSB = (unsigned char) (fSource);

		if (fSource >= 0.0)
			return 1;
		else
			return -1;
	}
}


// Name     Typ              Beschreibung             Aufloesung   Ersatzwert
// DATA2c   SIGNED INTEGER   -2047,9  ... + 2047,9    1/16         8000h

int data2c_to_float(unsigned char ucSourceLSB, unsigned char ucSourceMSB, float *pfTarget)
{
	if ((ucSourceMSB & 0x80) == 0x80) {
		*pfTarget = (float) (- ( ( ( ((unsigned char) (~ ucSourceMSB)) * 16.0) ) +
		                         ( ( ((unsigned char) (~ ucSourceLSB)) & 0xF0) >> 4) +
		                       ( ( ( ((unsigned char) (~ ucSourceLSB)) & 0x0F) +1 ) / 16.0) ) );

		if (ucSourceMSB  == 0x80 && ucSourceLSB == 0x00)
			return 0;
		else
			return -1;
	} else {
		*pfTarget = (float) ( (ucSourceMSB * 16.0) + ((ucSourceLSB & 0xF0) >> 4) + ((ucSourceLSB & 0x0F) / 16.0) );
		return 1;
	}
}

int float_to_data2c(float fSource, unsigned char *pucTargetMSB, unsigned char *pucTargetLSB)
{
	if (fSource < -2047.999 || fSource > 2047.999) {
		*pucTargetMSB = (unsigned char) (0x80);
		*pucTargetLSB = (unsigned char) (0x00);
		return 0;
	} else {
		*pucTargetLSB = ( ((unsigned char) ( ((unsigned char) fSource) % 16) << 4) +
		                  ((unsigned char) ( (fSource - ((unsigned char) fSource)) * 16.0)) );

		if (fSource < 0.0 && *pucTargetLSB != 0x00)
			*pucTargetMSB = (unsigned char) ((fSource / 16.0) - 1);
		else
			*pucTargetMSB = (unsigned char) (fSource / 16.0);

		if (fSource >= 0.0)
			return 1;
		else
			return -1;
	}
}



// CRC Calculation "CRC-8-WCDMA"
// Polynom "x^8 + x^7 + x^4 + x^3 + x + 1"

unsigned char calc_crc_byte(unsigned char ucByte, unsigned char ucInitCRC)
{
	unsigned char ucCRC;
	unsigned char ucPolynom;
	int i;

	ucCRC = ucInitCRC;

	for (i = 0; i < 8; i++) {

		if (ucCRC & 0x80)
			ucPolynom = (unsigned char) 0x9B;
		else
			ucPolynom = (unsigned char) 0;

		ucCRC = (unsigned char) ((ucCRC & ~0x80) << 1);

		if (ucByte & 0x80)
			ucCRC = (unsigned char) (ucCRC | 1);

		ucCRC = (unsigned char) (ucCRC ^ ucPolynom);
		ucByte = (unsigned char) (ucByte << 1);
	}
	return ucCRC;
}

unsigned char calc_crc(unsigned char *pucData, int iDataSize)
{
	int i;
	unsigned char ucCRC = 0;

	for( i = 0 ; i < iDataSize ; i++, pucData++ ) {
		ucCRC = calc_crc_byte(*pucData, ucCRC);
	}
	return ucCRC;
}

