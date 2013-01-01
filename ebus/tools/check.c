/*
 * Copyright (C) Roland Jax 2012 <roland.jax@liwest.at>
 *
 * This file is part of check.
 *
 * check is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * check is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with check. If not, see http://www.gnu.org/licenses/.
 */
 
 /* gcc -Wall -o check check.c */

#include <stdio.h>
#include <string.h>

int bcd_to_int(unsigned char ucSource, int *piTarget) {
	if ((ucSource & 0x0F) > 0x09 || ((ucSource >> 4) & 0x0F) > 0x09) {
		*piTarget = (int) (0xFF);	
		return 0;
	}
	else {
		*piTarget = (int) ( ( ((ucSource & 0xF0) >> 4) * 10) + (ucSource & 0x0F) );
		return 1;
	}
}

int data1b_to_int(unsigned char ucDATA1b, int *piTarget) {
	if ((ucDATA1b & 0x80) == 0x80) {
		*piTarget = (int) (- ( ((unsigned char) (~ ucDATA1b)) + 1) );
		
		if (*piTarget  == -0x80)
			return 0;
		else
			return -1;
	}
	else {
		*piTarget = (int) (ucDATA1b);
		return 1;
	}
}

int data1c_to_float(unsigned char ucSource, float *pfTarget) {
	if (ucSource > 0xC8) {
		*pfTarget = (float) (0xFF);	
		return 0;
	}
	else {
		*pfTarget = (float) (ucSource / 2.0);	
		return 1;
	}
}

int data2b_to_float(unsigned char ucSourceLSB, unsigned char ucSourceMSB, float *pfTarget) {
	if ((ucSourceMSB & 0x80) == 0x80) {                       
		*pfTarget = (float) (-   ( ((unsigned char) (~ ucSourceMSB)) +
		                       ( ( ((unsigned char) (~ ucSourceLSB)) + 1) / 256.0) ) );

		if (ucSourceMSB  == 0x80 && ucSourceLSB == 0x00)
			return 0;
		else
			return -1;
	}
	else {
		*pfTarget = (float) (ucSourceMSB + (ucSourceLSB / 256.0));	
		return 1;
	}
}

int data2c_to_float(unsigned char ucSourceLSB, unsigned char ucSourceMSB, float *pfTarget) {
	if ((ucSourceMSB & 0x80) == 0x80) {
		*pfTarget = (float) (- ( ( ( ((unsigned char) (~ ucSourceMSB)) * 16.0) ) +
		                         ( ( ((unsigned char) (~ ucSourceLSB)) & 0xF0) >> 4) +
		                       ( ( ( ((unsigned char) (~ ucSourceLSB)) & 0x0F) +1 ) / 16.0) ) );
		                       
		if (ucSourceMSB  == 0x80 && ucSourceLSB == 0x00)
			return 0;
		else
			return -1;
	}
	else {
		*pfTarget = (float) ( (ucSourceMSB * 16.0) + ((ucSourceLSB & 0xF0) >> 4) + ((ucSourceLSB & 0x0F) / 16.0) );
		return 1;
	}
}

int htoi(char s[]) {
    int i = 0;
    int n = 0;
 
    // Remove "0x" or "0X"
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        i = 2;
 
    while (s[i] != '\0') {
        int t;
 
        if (s[i] >= 'A' && s[i] <= 'F')
            t = s[i] - 'A' + 10;
        else if (s[i] >= 'a' && s[i] <= 'f')
            t = s[i] - 'a' + 10;
        else if (s[i] >= '0' && s[i] <= '9')
            t = s[i] - '0';
        else if (s[i] == ' ')
			return -1;
        else
            return n;
 
        n = 16 * n + t;
        ++i;
    }
 
    return n;
}

int main() {

	int ii = 0;
	int jj;
	
	int iBCD;
	int iData1b;
	int iReturn;
	float fData1c;
	float fData2b;
	float fData2c;
	
	int iData[10];
	
	char cByte;
	
	unsigned char ucHexByte;
	unsigned char ucTmp;
	
	while(1) {
		ii = 0;
		printf("Input: ");
		while ((cByte = fgetc(stdin)) != EOF) {
			if (cByte == '\n')
				break;
				
			if (ii < sizeof(iData)) {
				iReturn = htoi(&cByte);
				if (iReturn != -1) {
					iData[ii] = iReturn;
					ii++;
				}
			}
			else
				break;
		}		
		
		for(jj=0; jj<ii; jj+=2) {
			
			iBCD = 0;
			iData1b = 0;
			iReturn = 0;
			fData1c = 0.0;
			fData2b = 0.0;
			fData2c = 0.0;
			
			
			
			ucHexByte = (unsigned char) (iData[jj]*16 + iData[jj+1]);
			
			
			iReturn = bcd_to_int(ucHexByte, &iBCD);
			iReturn = data1b_to_int(ucHexByte, &iData1b);
			
			iReturn = data1c_to_float(ucHexByte, &fData1c);
			printf("HEX %02x ->\tiBCD: %3d\tiData1b: %4d\tfData1c: %5.1f", ucHexByte, iBCD, iData1b, fData1c);
			
			if (jj == 2) {
				iReturn = data2b_to_float(ucTmp, ucHexByte, &fData2b);
				iReturn = data2c_to_float(ucTmp, ucHexByte, &fData2c);
				printf("\tfData2b: %8.3f\tfData2c: %10.4f\n", fData2b, fData2c);
			}
			else {
				ucTmp = ucHexByte;
				printf("\n");
			}
		}

	}
	return 0;
}
