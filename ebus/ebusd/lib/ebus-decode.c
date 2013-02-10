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

static struct cmd_set *set = NULL;
static int setlen = 1;

int
eb_msg_send_cmd_encode(int id, unsigned char *msg, char *buf)
{
	char *c1, *c2;
	char d_pos[CMD_SET_SIZE_D_POS+1];
	unsigned char bcd, d1b, d1c, d2b[2], d2c[2];
	int ret, i, p1, p2;
	float f;

	memset(d_pos, '\0', sizeof(d_pos));
	strncpy(d_pos, set[id].d_pos, strlen(set[id].d_pos));

	c1 = strtok(d_pos, " ,\n");
	c2 = strtok(NULL, " ,\n");

	p1 = c1 ? atoi(c1) : 0;
	p2 = c2 ? atoi(c2) : 0;

	log_print(L_DBG, "id: %d d_pos: %s p1: %d p2: %d",
						id, set[id].d_pos, p1, p2);
	
	if (strncasecmp(set[id].d_type, "bcd", 3) == 0) {
		if (p1 > 0) {
			i = (int) (atof(buf) / set[id].d_fac);

			ret = eb_int_to_bcd(i, &bcd);
			sprintf(msg, "%02x\n", bcd);
		} else {
			goto on_error;
		}
				
	} else if (strncasecmp(set[id].d_type, "d1b", 3) == 0) {
		if (p1 > 0) {
			i = (int) (atof(buf) / set[id].d_fac);
			
			ret = eb_int_to_d1b(i, &d1b);
			sprintf(msg, "%02x", d1b);
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(set[id].d_type, "d1c", 3) == 0) {
		if (p1 > 0) {
			f = (atof(buf) / set[id].d_fac);
			
			ret = eb_float_to_d1c(f, &d1c);
			sprintf(msg, "%02x", d1c);
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(set[id].d_type, "d2b", 3) == 0) {
		if (p1 > 0 && p2 > 0) {
			f = (atof(buf) / set[id].d_fac);
			
			if (p1 > p2)
				ret = eb_float_to_d2b(f, &d2b[1], &d2b[0]);
			else
				ret = eb_float_to_d2b(f, &d2b[0], &d2b[1]);
				
			sprintf(msg, "%02x%02x\n", d2b[0], d2b[1]);	
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(set[id].d_type, "d2c", 3) == 0) {
		
		if (p1 > 0 && p2 > 0) {
			f = (atof(buf) / set[id].d_fac);
			
			if (p1 > p2)
				ret = eb_float_to_d2c(f, &d2c[1], &d2c[0]);
			else
				ret = eb_float_to_d2c(f, &d2c[0], &d2c[1]);
				
			sprintf(msg, "%02x%02x\n", d2c[0], d2c[1]);
		} else {
			goto on_error;
		}			
		
	}

	return 0;

on_error:
	strcpy(buf, "error encode\n");
	return -1;

}

int
eb_msg_send_cmd_decode(int id, unsigned char *msg, char *buf)
{
	char *c1, *c2;
	char r_pos[CMD_GET_SIZE_R_POS+1];
	int ret, i, p1, p2;
	float f;

	memset(r_pos, '\0', sizeof(r_pos));
	strncpy(r_pos, get[id].r_pos, strlen(get[id].r_pos));

	c1 = strtok(r_pos, " ,\n");
	c2 = strtok(NULL, " ,\n");

	p1 = c1 ? atoi(c1) : 0;
	p2 = c2 ? atoi(c2) : 0;

	log_print(L_DBG, "id: %d r_pos: %s p1: %d p2: %d",
						id, get[id].r_pos, p1, p2);
	
	if (strncasecmp(get[id].r_type, "bcd", 3) == 0) {
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

			f = i * get[id].r_fac;
			sprintf(buf, "%6.2f\n", f);
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(get[id].r_type, "d1c", 3) == 0) {
		if (p1 > 0) {
			ret = eb_d1c_to_float(msg[p1], &f);

			f *= get[id].r_fac;
			sprintf(buf, "%6.2f\n", f);			
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
	strcpy(buf, "error decode\n");
	return -1;

}

void
eb_msg_send_cmd_prepare_set(int id, char *msg, int *msglen, int *type, char *data)
{
	unsigned char tmp[CMD_SET_SIZE_S_MSG];
	char str[CMD_SET_SIZE_S_ZZ + CMD_SET_SIZE_S_CMD + 2 + CMD_SET_SIZE_S_MSG];
	char byte;
	int in[SERIAL_BUFSIZE];
	int ret, i, j, k;
	
	/* encode msg */
	memset(tmp, '\0', sizeof(tmp));
	ret = eb_msg_send_cmd_encode(id, tmp, data);
	
	memset(str, '\0', sizeof(str));
	sprintf(str, "%s%s%02X%s%s",
		set[id].s_zz, set[id].s_cmd, set[id].s_len, set[id].s_msg, tmp);

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

	*type = set[id].s_type;
}

void
eb_msg_send_cmd_prepare_get(int id, char *msg, int *msglen, int *type)
{
	char str[CMD_GET_SIZE_S_ZZ + CMD_GET_SIZE_S_CMD + 2 + CMD_GET_SIZE_S_MSG];
	char byte;
	int ret, i, j, k;
	int in[SERIAL_BUFSIZE];	

	memset(str, '\0', sizeof(str));
	sprintf(str, "%s%s%02X%s",
		get[id].s_zz, get[id].s_cmd, get[id].s_len, get[id].s_msg);

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
eb_msg_send_cmd(int id, int msgtype, char *data, char *buf, int *buflen, int retry)
{
	unsigned char msg[SERIAL_BUFSIZE];
	int msglen, type, ret, send_retry;

	memset(msg, '\0', sizeof(msg));
	msglen = sizeof(msg);

	type = UNSET;			

	/* prepare command */
	if (msgtype == GET)
		eb_msg_send_cmd_prepare_get(id, msg, &msglen, &type);
	else if (msgtype = SET)
		eb_msg_send_cmd_prepare_set(id, msg, &msglen, &type, data);
		
	eb_raw_print_hex(msg, msglen);
	
	/* send data to bus */
	ret = -1;
	send_retry = 0;

	while (ret < 0 && send_retry < retry) {
		if (send_retry > 0)
			log_print(L_NOT, "send retry: %d", send_retry);
				
		ret = eb_send_data(msg, msglen, type);
		send_retry++;
	}

	if (ret >= 0) {
		
		if (type == EBUS_MSG_BROADCAST)
			strcpy(buf, " broadcast done\n");

		if (type == EBUS_MSG_MASTER_MASTER) {
			if (ret == 0) {
				strcpy(buf, " ACK\n");
			} else {
				strcpy(buf, " NAK\n");
			}
		}

		if (type == EBUS_MSG_MASTER_SLAVE) {
			if (ret == 0) {
				memset(msg, '\0', sizeof(msg));
				msglen = sizeof(msg);
				eb_get_recv_data(msg, &msglen);
				eb_raw_print_hex(msg, msglen);

				/* decode */
				if (msgtype == SET)
					strcpy(buf, " ACK\n");
				else
					ret = eb_msg_send_cmd_decode(id, msg, buf);
				
			} else {
				strcpy(buf, " NAK\n");
			}
		}

	} else {
		strcpy(buf, " error send ebus msg\n");
	}
	*buflen = strlen(buf);
}


int
eb_msg_search_cmd_set(const char *class, const char *cmd)
{
	int i;

	for (i = 0; i < setlen; i++) {
		if ((strncasecmp(class, set[i].class, strlen(set[i].class)) == 0)
		   && (strncasecmp(cmd, set[i].cmd, strlen(set[i].cmd)) == 0)			   
		   && strlen(cmd) == strlen(set[i].cmd)) {

			log_print(L_NOT, " found: %s%s%02X%s type: %d ==> id: %d",
				set[i].s_zz, set[i].s_cmd, set[i].s_len,
				set[i].s_msg, set[i].s_type, i);
			return i;
		}
	
	}
	
	return -1;
}

int
eb_msg_search_cmd_get(const char *class, const char *cmd)
{
	int i;

	for (i = 0; i < getlen; i++) {
		if ((strncasecmp(class, get[i].class, strlen(get[i].class)) == 0)
		   && (strncasecmp(cmd, get[i].cmd, strlen(get[i].cmd)) == 0)			   
		   && strlen(cmd) == strlen(get[i].cmd)) {

			log_print(L_NOT, " found: %s%s%02X%s type: %d ==> id: %d",
				get[i].s_zz, get[i].s_cmd, get[i].s_len,
				get[i].s_msg, get[i].s_type, i);
			return i;
		}
	
	}
	
	return -1;
}

int
eb_msg_search_cmd(char *buf, int *msgtype)
{
	char *type, *class, *cmd, *data;
	int ret;

	type = strtok(buf, " ");
	class = strtok(NULL, " .");
	cmd = strtok(NULL, " \n\r\t");
	data = strtok(NULL, " \n\r\t");

	if (class != NULL && cmd != NULL) {
		
		if (strncasecmp(buf, "get", 3) == 0) {
			log_print(L_NOT, "search: %s %s.%s", type, class, cmd);
			/* search command */
			ret = eb_msg_search_cmd_get(class, cmd);
			memset(buf, '\0', sizeof(buf));
			strncpy(buf, "-", 1);
			*msgtype = GET;
			return ret;

		} else if (strncasecmp(buf, "set", 3) == 0) {
			log_print(L_NOT, "search: %s %s.%s %s",
							type, class, cmd, data);
			/* search command */
			ret = eb_msg_search_cmd_set(class, cmd);
			
			if (data != NULL) {
				*msgtype = SET;
				memset(buf, '\0', sizeof(buf));
				strncpy(buf, data, strlen(data));
				return ret;
			} else {
				return -1;
			}
		}		
	}
	
	return -1;
}



int
eb_cmd_fill_set(const char *tok)
{

	set = (struct cmd_set *) realloc(set, (setlen + 1) * sizeof(struct cmd_set));
	err_ret_if(set == NULL, -1);

	memset(set + setlen, '\0', sizeof(struct cmd_set));
		
	/* key */
	set[setlen].key = setlen;
	
	/* class */
	tok = strtok(NULL, ";");
	strncpy(set[setlen].class, tok, strlen(tok));

	/* cmd */
	tok = strtok(NULL, ";");
	strncpy(set[setlen].cmd, tok, strlen(tok));
	
	/* com */
	tok = strtok(NULL, ";");
	strncpy(set[setlen].com, tok, strlen(tok));	

	/* s_type */
	tok = strtok(NULL, ";");
	set[setlen].s_type = atoi(tok);	
	
	/* s_zz */
	tok = strtok(NULL, ";");
	strncpy(set[setlen].s_zz, tok, strlen(tok));

	/* s_cmd */
	tok = strtok(NULL, ";");
	strncpy(set[setlen].s_cmd, tok, strlen(tok));

	/* s_len */
	tok = strtok(NULL, ";");
	set[setlen].s_len = atoi(tok);

	/* s_msg */
	tok = strtok(NULL, ";");
	strncpy(set[setlen].s_msg, tok, strlen(tok));

	/* d_len */
	tok = strtok(NULL, ";");
	set[setlen].d_len = atoi(tok);

	/* d_pos */
	tok = strtok(NULL, ";");
	strncpy(set[setlen].d_pos, tok, strlen(tok));

	/* d_type */
	tok = strtok(NULL, ";");
	strncpy(set[setlen].d_type, tok, strlen(tok));
	
	/* d_fac */
	tok = strtok(NULL, ";");
	set[setlen].d_fac = atof(tok);

	/* d_unit */
	tok = strtok(NULL, ";");
	strncpy(set[setlen].d_unit, tok, strlen(tok)-1);

	setlen++;

	return 0;	
}


int
eb_cmd_fill_get(const char *tok)
{

	get = (struct cmd_get *) realloc(get, (getlen + 1) * sizeof(struct cmd_get));
	err_ret_if(get == NULL, -1);

	memset(get + getlen, '\0', sizeof(struct cmd_get));
		
	/* key */
	get[getlen].key = getlen;
	
	/* class */
	tok = strtok(NULL, ";");
	strncpy(get[getlen].class, tok, strlen(tok));

	/* cmd */
	tok = strtok(NULL, ";");
	strncpy(get[getlen].cmd, tok, strlen(tok));

	/* com */
	tok = strtok(NULL, ";");
	strncpy(get[getlen].com, tok, strlen(tok));

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
	strncpy(get[getlen].r_unit, tok, strlen(tok)-1);

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
					ret = eb_cmd_fill_set(tok);
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

	if (setlen > 0)
		free(set);

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

