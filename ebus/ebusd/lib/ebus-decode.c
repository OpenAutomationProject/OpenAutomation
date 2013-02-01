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
 * @file ebus-decode.c
 * @brief ebus communication functions
 * @author roland.jax@liwest.at
 * @version 0.1
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>

#include "log.h"
#include "ebus-decode.h"



static struct cmd_get *get = NULL;
static int getlen = 0;



int
eb_cmd_fill_get(const char *tok)
{

	get = realloc(get, (getlen + 1) * sizeof(struct cmd_get));
	err_ret_if(get == NULL, -1);

	memset(get + getlen, '\0', sizeof(struct cmd_get));
		
	/* key */
	get[getlen].key = getlen;
	
	/* cmd */
	tok = strtok(NULL, ";");
	strncpy(get[getlen].class, tok, strlen(tok));

	/* sub */
	tok = strtok(NULL, ";");
	strncpy(get[getlen].cmd, tok, strlen(tok));
	
	/* s_zz */
	tok = strtok(NULL, ";");
	strncpy(get[getlen].s_zz, tok, strlen(tok));

	/* s_cmd */
	tok = strtok(NULL, ";");
	strncpy(get[getlen].s_cmd, tok, strlen(tok));

	/* s_len */
	tok = strtok(NULL, ";");
	get[getlen].s_len = atoi(tok);

	/* s_msg */
	tok = strtok(NULL, ";");
	strncpy(get[getlen].s_msg, tok, strlen(tok));

	/* r_len */
	tok = strtok(NULL, ";");
	get[getlen].r_len = atoi(tok);

	/* r_pos */
	tok = strtok(NULL, ";");
	strncpy(get[getlen].r_pos, tok, strlen(tok));

	/* r_type */
	tok = strtok(NULL, ";");
	strncpy(get[getlen].r_type, tok, strlen(tok));
	
	/* r_fac */
	tok = strtok(NULL, ";");
	get[getlen].r_fac = atof(tok);

	/* r_unit */
	tok = strtok(NULL, ";");
	strncpy(get[getlen].r_unit, tok, strlen(tok));
	
	/* com */
	tok = strtok(NULL, ";");
	strncpy(get[getlen].com, tok, strlen(tok)-1);

	getlen++;

	return 0;	
}

int
eb_cmd_num_c(const char *str, const char c)
{
	const char *p = str;
	int i = 0;

	do {
		if (*p == c)
			i++;
	} while (*(p++));

	return i;
}

int
eb_cmd_file_read(const char *file)
{
	int ret, i;
	char line[CMD_LINELEN + 1];
	char *tok;
	FILE *fp = NULL;

	log_print(L_NOT, "%s", file);

	/* open config file */
	fp = fopen(file, "r");
	err_ret_if(fp == NULL, -1);			

	/* read each line and fill cmd array */
	while (fgets(line, CMD_LINELEN, fp) != NULL) {
		i++;
		/* prevent buffer overflow !!!
		 * number of tokens must be >= number of columns - 1 of file */
		if (eb_cmd_num_c(line, ';') >= CMD_DELIMETER) {
			tok = strtok(line, ";\n");
			if (tok != NULL && tok[0] != '#' ) {
				/* type */
				if (strncasecmp(tok, "get", 3) == 0) {			
					ret = eb_cmd_fill_get(tok);
					if (ret < 0)
						return -2;
					
				} else if (strncasecmp(tok, "set", 3) == 0) {
					//~ eb_cmd_fill_set(j, tok);
					if (ret < 0)
						return -2;
					
				} else if (strncasecmp(tok, "cyc", 3) == 0) {
					//~ eb_cmd_fill_cyc(k, tok);
					if (ret < 0)
						return -2;
				}
			}
		}
	}
	
	/* close config file */
	ret = fclose(fp);
	err_ret_if(ret == EOF, -1);

	log_print(L_NOT, "%s success", file);

	return 0;
}

int
eb_cmd_dir_read(const char *cfgdir, const char *extension)
{
	struct dirent **dir;
	int ret, i, j, files;
	char *ext, file[CMD_FILELEN];

	files = scandir(cfgdir, &dir, 0, alphasort);
	if (files < 0) {
		log_print(L_WAR, "configuration directory %s not found.", cfgdir);
		return -1;
	}
	
	i = 0;
	j = 0;
	while (i < files) {
		ext = strrchr(dir[i]->d_name, '.');
			if (ext != NULL) {
				if (strlen(ext) == 4
				    && dir[i]->d_type == DT_REG
				    && strncasecmp(ext, extension, 4) == 0 ) {
					memset(file, '\0', strlen(file));
					sprintf(file, "%s%s", cfgdir,
								dir[i]->d_name);
									
					ret = eb_cmd_file_read(file);
					if (ret < 0)
						return -2;

					j++;
				}
			}
			
		free(dir[i]);
		i++;
	}
	
	free(dir);

	if (j == 0) {
		log_print(L_WAR, "command files not found ==> decode disabled.");
		return -3;
	}
	
	return 0;
}



int
eb_htoi(const char *buf)
{
	int ret;
	ret = -1;
	
	if (isxdigit(*buf)) {
		if (isalpha(*buf))
			ret = 55;	
		else
			ret = 48;
			
		ret = toupper(*buf) - ret;
	}

	return ret;
}


void
eb_esc(unsigned char *buf, int *buflen)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, i;

	memset(tmp, '\0', sizeof(tmp));
	i = 0;
	tmplen = 0;
	
	while (i < *buflen) {
		
		if (buf[i] == EBUS_SYN) {
			tmp[tmplen] = EBUS_SYN_ESC_A9;
			tmplen++;
			tmp[tmplen] = EBUS_SYN_ESC_01;
			tmplen++;
		} else if (buf[i] == EBUS_SYN_ESC_A9) {
			tmp[tmplen] = EBUS_SYN_ESC_A9;
			tmplen++;
			tmp[tmplen] = EBUS_SYN_ESC_00;
			tmplen++;
		} else {
			tmp[tmplen] = buf[i];
			tmplen++;
		}
		
		i++;
	}

	memset(buf, '\0', sizeof(buf));
	for (i = 0; i < tmplen; i++)
		buf[i] = tmp[i];

	*buflen = tmplen;
}

void
eb_unesc(unsigned char *buf, int *buflen)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, i, found;

	memset(tmp, '\0', sizeof(tmp));
	i = 0;
	tmplen = 0;
	found = 0;
	
	while (i < *buflen) {
		
		if (buf[i] == EBUS_SYN_ESC_A9) {
			found = 1;
		} else if (found == 1) {
			if (buf[i] == EBUS_SYN_ESC_01) {
				tmp[tmplen] = EBUS_SYN;
				tmplen++;
			} else {
				tmp[tmplen] = EBUS_SYN_ESC_A9;
				tmplen++;
			}
			
			found = 0;
		} else {
			tmp[tmplen] = buf[i];
			tmplen++;
		}
		
		i++;
	}

	memset(buf, '\0', sizeof(buf));
	for (i = 0; i < tmplen; i++)
		buf[i] = tmp[i];

	*buflen = tmplen;
}



int
eb_bcd_to_int(unsigned char src, int *tgt)
{
	if ((src & 0x0F) > 0x09 || ((src >> 4) & 0x0F) > 0x09) {
		*tgt = (int) (0xFF);
		return 0;
	} else {
		*tgt = (int) ( ( ((src & 0xF0) >> 4) * 10) + (src & 0x0F) );
		return 1;
	}
}

int
eb_int_to_bcd(int src, unsigned char *tgt)
{
	if (src > 99) {
		*tgt = (unsigned char) (0xFF);
		return 0;
	} else {
		*tgt = (unsigned char) ( ((src / 10) << 4) | (src % 10) );
		return 1;
	}
}



int
eb_d1b_to_int(unsigned char src, int *tgt)
{
	if ((src & 0x80) == 0x80) {
		*tgt = (int) (- ( ((unsigned char) (~ src)) + 1) );

		if (*tgt  == -0x80)
			return 0;
		else
			return -1;

	} else {
		*tgt = (int) (src);
		return 1;
	}
}

int
eb_int_to_d1b(int src, unsigned char *tgt)
{
	if (src < -127 || src > 127) {
		*tgt = (unsigned char) (0x80);
		return 0;
	} else {
		if (src >= 0) {
			*tgt = (unsigned char) (src);
			return 1;
		} else {
			*tgt = (unsigned char) (- (~ (src - 1) ) );
			return -1;
		}
	}
}



int
eb_d1c_to_float(unsigned char src, float *tgt)
{
	if (src > 0xC8) {
		*tgt = (float) (0xFF);
		return 0;
	} else {
		*tgt = (float) (src / 2.0);
		return 1;
	}
}

int
eb_float_to_d1c(float src, unsigned char *tgt)
{
	if (src < 0.0 || src > 100.0) {
		*tgt = (unsigned char) (0xFF);
		return 0;
	} else {
		*tgt = (unsigned char) (src * 2.0);
		return 1;
	}
}



int
eb_d2b_to_float(unsigned char src_lsb, unsigned char src_msb, float *tgt)
{
	if ((src_msb & 0x80) == 0x80) {
		*tgt = (float)
			(- ( ((unsigned char) (~ src_msb)) +
			(  ( ((unsigned char) (~ src_lsb)) + 1) / 256.0) ) );

		if (src_msb  == 0x80 && src_lsb == 0x00)
			return 0;
		else
			return -1;

	} else {
		*tgt = (float) (src_msb + (src_lsb / 256.0));
		return 1;
	}
}

int
eb_float_to_d2b(float src, unsigned char *tgt_lsb, unsigned char *tgt_msb)
{
	if (src < -127.999 || src > 127.999) {
		*tgt_msb = (unsigned char) (0x80);
		*tgt_lsb = (unsigned char) (0x00);
		return 0;
	} else {
		*tgt_lsb = (unsigned char)
					((src - ((unsigned char) src)) * 256.0);

		if (src < 0.0 && *tgt_lsb != 0x00)
			*tgt_msb = (unsigned char) (src - 1);
		else
			*tgt_msb = (unsigned char) (src);

		if (src >= 0.0)
			return 1;
		else
			return -1;

	}
}



int
eb_d2c_to_float(unsigned char src_lsb, unsigned char src_msb, float *tgt)
{
	if ((src_msb & 0x80) == 0x80) {
		*tgt = (float)
		(- ( ( ( ((unsigned char) (~ src_msb)) * 16.0) ) +
		     ( ( ((unsigned char) (~ src_lsb)) & 0xF0) >> 4) +
		   ( ( ( ((unsigned char) (~ src_lsb)) & 0x0F) +1 ) / 16.0) ) );

		if (src_msb  == 0x80 && src_lsb == 0x00)
			return 0;
		else
			return -1;

	} else {
		*tgt = (float) ( (src_msb * 16.0) + ((src_lsb & 0xF0) >> 4) +
						((src_lsb & 0x0F) / 16.0) );
		return 1;
	}
}

int
eb_float_to_d2c(float src, unsigned char *tgt_lsb, unsigned char *tgt_msb)
{
	if (src < -2047.999 || src > 2047.999) {
		*tgt_msb = (unsigned char) (0x80);
		*tgt_lsb = (unsigned char) (0x00);
		return 0;
	} else {
		*tgt_lsb =
		  ( ((unsigned char) ( ((unsigned char) src) % 16) << 4) +
		    ((unsigned char) ( (src - ((unsigned char) src)) * 16.0)) );

		if (src < 0.0 && *tgt_lsb != 0x00)
			*tgt_msb = (unsigned char) ((src / 16.0) - 1);
		else
			*tgt_msb = (unsigned char) (src / 16.0);

		if (src >= 0.0)
			return 1;
		else
			return -1;
	}
}



unsigned char
eb_calc_crc_byte(unsigned char byte, unsigned char init_crc)
{
	unsigned char crc, polynom;
	int i;

	crc = init_crc;

	for (i = 0; i < 8; i++) {

		if (crc & 0x80)
			polynom = (unsigned char) 0x9B;
		else
			polynom = (unsigned char) 0;

		crc = (unsigned char) ((crc & ~0x80) << 1);

		if (byte & 0x80)
			crc = (unsigned char) (crc | 1);

		crc = (unsigned char) (crc ^ polynom);
		byte = (unsigned char) (byte << 1);
	}
	return crc;
}

unsigned char
eb_calc_crc(const unsigned char *buf, int buflen)
{
	int i;
	unsigned char crc = 0;

	for (i = 0 ; i < buflen ; i++, buf++)
		crc = eb_calc_crc_byte(*buf, crc);

	return crc;
}

