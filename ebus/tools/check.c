/*
 * Copyright (C) Roland Jax 2012-2013 <roland.jax@liwest.at>
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

#include <stdio.h>
#include <string.h>

#include "ebus.h"

int main() {

	int i, j, end, ret;

	int bcd, data1b, data[SERIAL_BUFSIZE];
	float data1c, data2b, data2c;

	char byte;
	
	unsigned char hex, tmp, crc, crc_calc[2] ;

	tmp = 0;
	end = 0;
	do {
		i = 0;
		printf("Input: ");
		while ((byte = fgetc(stdin)) != EOF) {
			
			if (byte == '\n') {
				break;
			}
				
			if (byte == 'q') {
				end = 1;
				break;
			}
			
			if (i < sizeof(data)) {
				ret = ebus_htoi(&byte);
				if (ret != -1) {
					data[i] = ret;
					i++;
				}
			} else {
				break;
			}
		}		
		
		if (!end) {
		
			for (j = 0; j < i; j += 2) {
				
				bcd = 0;
				data1b = 0;
				ret = 0;
				data1c = 0.0;
				data2b = 0.0;
				data2c = 0.0;

				hex = (unsigned char) (data[j]*16 + data[j+1]);
				
				ret = ebus_bcd_to_int(hex, &bcd);
				ret = ebus_data1b_to_int(hex, &data1b);
				
				ret = ebus_data1c_to_float(hex, &data1c);
				printf("hex %02x ->\tbcd: %3d\tdata1b: %4d\tdata1c: %5.1f", hex, bcd, data1b, data1c);

				
				if (j == 2) {
					ret = ebus_data2b_to_float(tmp, hex, &data2b);
					ret = ebus_data2c_to_float(tmp, hex, &data2c);
					crc_calc[0] = tmp;
					crc_calc[1] = hex;
					crc = ebus_calc_crc(crc_calc,2);
					printf("\tdata2b: %8.3f\tdata2c: %10.4f\tcrc: %02x\n", data2b, data2c, crc);
				}
				else {
					tmp = hex;
					printf("\n");
				}
			}
		}

	} while (end == 0);
	
	return 0;
}
