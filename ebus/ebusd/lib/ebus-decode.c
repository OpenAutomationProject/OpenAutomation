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
#include "ebus-bus.h"
#include "ebus-decode.h"



static struct cmd_get *get = NULL;
static int getlen = 0;

int
eb_msg_decode_result(int id, unsigned char *msg, int msglen, char *buf)
{
	char *c1, *c2, *c3;
	int ret, i, p1, p2, p3;
	float f;

	c1 = strtok(get[id].r_pos, " ,\n");
	c2 = strtok(NULL, " ,\n");
	c3 = strtok(NULL, " ,\n");

	p1 = c1 ? atoi(c1) : 0;
	p2 = c2 ? atoi(c2) : 0;
	p3 = c3 ? atoi(c3) : 0;
	
	if (strncasecmp(get[id].r_type, "int", 3) == 0) {
		if (p1 > 0) {
			ret = eb_bcd_to_int(msg[p1], &i);

			i *= get[id].r_fac;
			sprintf(buf, "%3d\n", i);
		} else {
			goto on_error;
		}
				
	} else if (strncasecmp(get[id].r_type, "d1b", 3) == 0) {
		if (p1 > 0) {
			ret = eb_d1b_to_int(msg[p1], &i);

			i *= get[id].r_fac;
			sprintf(buf, "%4d\n", i);
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(get[id].r_type, "d1c", 3) == 0) {
		if (p1 > 0) {
			ret = eb_d1c_to_float(msg[p1], &f);

			f *= get[id].r_fac;
			sprintf(buf, "%5.1f\n", f);
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(get[id].r_type, "d2b", 3) == 0) {
		
		if (p1 > 0 && p2 > 0) {
			if (p1 > p2)
				ret = eb_d2b_to_float(msg[p2], msg[p1], &f);
			else
				ret = eb_d2b_to_float(msg[p1], msg[p2], &f);
				
			f *= get[id].r_fac;
			sprintf(buf, "%8.3f\n", f);
			
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(get[id].r_type, "d2c", 3) == 0) {
		
		if (p1 > 0 && p2 > 0) {
			if (p1 > p2)
				ret = eb_d2c_to_float(msg[p2], msg[p1], &f);
			else	
				ret = eb_d2c_to_float(msg[p1], msg[p2], &f);
			
			f *= get[id].r_fac;
			sprintf(buf, "%10.4f\n", f);
		} else {
			goto on_error;
		}			
		
	}

	return 0;

on_error:
	strcpy(buf, "error decode answer\n");
	return -1;

}

void
eb_msg_send_cmd_prepare(int id, char *msg, int *msglen, int *type)
{
	char str[CMD_GET_SIZE_S_ZZ + CMD_GET_SIZE_S_CMD + 2 + CMD_GET_SIZE_S_MSG];
	memset(str, '\0', sizeof(str));
	
	sprintf(str, "%s%s%02X%s",
		get[id].s_zz, get[id].s_cmd, get[id].s_len, get[id].s_msg);
	
	char byte;
	int ret, i, j, k;
	int in[SERIAL_BUFSIZE];

	memset(in, '\0', sizeof(in));
	i = 0;
	j = 0;
	
	while (str[j] != '\0') {
		byte = str[j];
		if (i < sizeof(in)) {

			ret = eb_htoi(&byte);
			if (ret != -1) {
				in[i] = ret;
				i++;
			}
		}
		j++;
	}


	memset(msg, '\0', sizeof(msg));
	for (j = 0, k = 0; j < i; j += 2, k++)
		msg[k] = (unsigned char) (in[j]*16 + in[j+1]);

	*msglen = k;

	*type = get[id].s_type;
}

void
eb_msg_send_cmd(int id, char *buf, int *buflen)
{
	unsigned char msg[SERIAL_BUFSIZE];
	int msglen, msgtype, ret;

	memset(msg, '\0', sizeof(msg));
	msglen = sizeof(msg);

	msgtype = UNSET;			

	/* prepare command */
	eb_msg_send_cmd_prepare(id, msg, &msglen, &msgtype);
	eb_raw_print_hex(msg, msglen);
	
	/* send data to bus */
	ret = eb_send_data(msg, msglen, msgtype);

	if (ret >= 0) {
		
		if (msgtype == EBUS_MSG_BROADCAST)
			strcpy(buf, "broadcast done\n");

		if (msgtype == EBUS_MSG_MASTER_MASTER) {
			if (ret == 0) {
				strcpy(buf, "ACK\n");
			} else {
				strcpy(buf, "NAK\n");
			}
		}

		if (msgtype == EBUS_MSG_MASTER_SLAVE) {
			if (ret == 0) {
				memset(msg, '\0', sizeof(msg));
				eb_get_recv_data(msg, &msglen);
				eb_raw_print_hex(msg, msglen);

				/* decode */
				ret = eb_msg_decode_result(id, msg, msglen, buf);
				
			} else {
				strcpy(buf, "NAK\n");
			}
		}

	} else {
		strcpy(buf, "error send ebus msg\n");
	}
	*buflen = strlen(buf);
}

int
eb_msg_search_cmd_table(const char *class, const char *cmd)
{
	int i;

	for (i = 0; i < getlen; i++) {
		if ((strncasecmp(class, get[i].class, strlen(get[i].class)) == 0)
		   && (strncasecmp(cmd, get[i].cmd, strlen(get[i].cmd)) == 0)) {

			log_print(L_NOT, " found: %s%s%02X%s type: %d ==> id: %d",
				get[i].s_zz, get[i].s_cmd, get[i].s_len,
				get[i].s_msg, get[i].s_type, i);
			return i;
		}
			
	}
	
	return -1;
}

int
eb_msg_search_cmd(char *buf)
{
	char *type, *class, *cmd;
	int ret;

	type = strtok(buf, " ");
	class = strtok(NULL, " .");
	cmd = strtok(NULL, " ");	
	
	if ((strncasecmp(buf, "get", 3) == 0) && class != NULL && cmd != NULL) {
		log_print(L_NOT, "search: %s %s.%s", type, class, cmd);

		/* search command */
		ret = eb_msg_search_cmd_table(class, cmd);
		return ret;

	}
	
	return -1;
}



int
eb_cmd_fill_get(const char *tok)
{

	get = (struct cmd_get *) realloc(get, (getlen + 1) * sizeof(struct cmd_get));
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

	/* s_type */
	tok = strtok(NULL, ";");
	get[getlen].s_type = atoi(tok);	
	
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
	char file[CMD_FILELEN];
	char *ext;

	files = scandir(cfgdir, &dir, 0, alphasort);
	if (files < 0) {
		log_print(L_WAR, "configuration directory %s not found.", cfgdir);
		return 1;
	}
	
	i = 0;
	j = 0;
	while (i < files) {
		ext = strrchr(dir[i]->d_name, '.');
			if (ext != NULL) {
				if (strlen(ext) == 4
				    && dir[i]->d_type == DT_REG
				    && strncasecmp(ext, extension, 4) == 0 ) {
					memset(file, '\0', sizeof(file));
					sprintf(file, "%s/%s", cfgdir,
								dir[i]->d_name);
									
					ret = eb_cmd_file_read(file);
					if (ret < 0)
						return -1;

					j++;
				}
			}
			
		free(dir[i]);
		i++;
	}
	
	free(dir);

	if (j == 0) {
		log_print(L_WAR, "no command files found ==> decode disabled.");
		return 2;
	}
	
	return 0;
}

void
eb_cmd_dir_free(void)
{
	if (getlen > 0)
		free(get);

	//~ if (setlen > 0)
		//~ free(set);

	//~ if (cyclen > 0)
		//~ free(cyc);				
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

