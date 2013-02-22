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
 * @file ebus-bus.c
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

static struct cycbuf *cyc = NULL;
static int cyclen = 0;

static struct commands *com = NULL;
static int comlen = 0;

static struct send_data send_data;

static struct recv_data recv_data;

static int rawdump = NO;
static int showraw = NO;

static int get_retry = EBUS_GET_RETRY;
static int skip_ack = EBUS_SKIP_ACK;
static long max_wait = EBUS_MAX_WAIT;
static int send_retry = EBUS_SEND_RETRY;

static unsigned char qq = EBUS_QQ;

static unsigned char ack = EBUS_ACK;
static unsigned char nak = EBUS_NAK;
static unsigned char syn = EBUS_SYN;

static int sfd;
static struct termios oldtio;

static FILE *rawfp = NULL;



void
eb_set_rawdump(int dump)
{
	rawdump = dump;	
}

void
eb_set_showraw(int show)
{
	showraw = show;	
}

void
eb_set_qq(unsigned char src)
{
	qq = src;	
}

void
eb_set_get_retry(int retry)
{
	get_retry = retry;
}

void
eb_set_skip_ack(int skip)
{
	skip_ack = skip;
}

void
eb_set_max_wait(long usec)
{
	max_wait = usec;
}

void
eb_set_send_retry(int retry)
{
	send_retry = retry;
}


int
eb_diff_time(const struct timeval *tact, const struct timeval *tlast,
							struct timeval *tdiff)
{
    long diff;

    diff = (tact->tv_usec + 1000000 * tact->tv_sec) -
	   (tlast->tv_usec + 1000000 * tlast->tv_sec);
	   
    tdiff->tv_sec = diff / 1000000;
    tdiff->tv_usec = diff % 1000000;

    return (diff < 0);
}



int
eb_search_cyc(const unsigned char *hex, int hexlen)
{
	int i;
	unsigned char hlp[CMD_SIZE_S_MSG+1];		

	memset(hlp, '\0', sizeof(hlp));
	for (i = 0; i < hexlen; i++)
		sprintf(&hlp[2 * i], "%02X", hex[i]);

	for (i = 0; i < cyclen; i++) {
		if (memcmp(hlp, cyc[i].msg, strlen(cyc[i].msg)) == 0) {
			log_print(L_NOT, " found: %s type: %d ==> id: %d",
				cyc[i].msg, com[cyc[i].id].s_type, cyc[i].id);
					
			return cyc[i].id;
		}
	}
	
	return -1;
}

int
eb_search_cmd_id(const char *type, const char *class, const char *cmd)
{
	int i;

	for (i = 0; i < comlen; i++) {
		if ((strncasecmp(type, com[i].type, strlen(com[i].type)) == 0)
		   && (strncasecmp(class, com[i].class, strlen(com[i].class)) == 0)
		   && (strncasecmp(cmd, com[i].cmd, strlen(com[i].cmd)) == 0)			   
		   && strlen(cmd) == strlen(com[i].cmd)) {

			log_print(L_NOT, " found: %s%s%02X%s type: %d ==> id: %d",
				com[i].s_zz, com[i].s_cmd, com[i].s_len,
				com[i].s_msg, com[i].s_type, i);
			return i;
		}
	
	}
	
	return -1;
}

int
eb_search_cmd(char *buf, char *data)
{
	char *type, *class, *cmd, *tok;
	char tmp[CMD_DATA_SIZE];
	int ret;

	type = strtok(buf, " ");
	class = strtok(NULL, " .");
	cmd = strtok(NULL, " .\n\r\t");
	
	if (class != NULL && cmd != NULL) {
	
		if (strncasecmp(type, "get", 3) == 0 ||
		    strncasecmp(type, "set", 3) == 0 ||
		    strncasecmp(type, "cyc", 3) == 0) {
			log_print(L_NOT, "search: %s %s.%s", type, class, cmd);
			
			ret = eb_search_cmd_id(type, class, cmd);
			if (ret < 0)
				return -1;
			
			tok = strtok(NULL, "\n\r");
			
			if (tok == NULL)
				strncpy(data, "-", 1);
			else
				strncpy(data, tok, strlen(tok));			
			
			log_print(L_NOT, "  data: %s", data);
			return ret;

		}
	}	
	
	return -1;
}



int
eb_cmd_encode_value(int id, int elem, char *data, unsigned char *msg, char *buf)
{
	char *c1, *c2, *c3;
	char d_pos[CMD_SIZE_D_POS+1];
	unsigned char bcd, d1b, d1c, d2b[2], d2c[2];
	int ret, i, p1, p2, p3;
	float f;

	memset(d_pos, '\0', sizeof(d_pos));
	strncpy(d_pos, com[id].elem[elem].d_pos, strlen(com[id].elem[elem].d_pos));

	c1 = strtok(d_pos, " ,\n");
	c2 = strtok(NULL, " ,\n");
	c3 = strtok(NULL, " ,\n");
	
	p1 = c1 ? atoi(c1) : 0;
	p2 = c2 ? atoi(c2) : 0;
	p3 = c3 ? atoi(c3) : 0;
	
	log_print(L_DBG, "id: %d elem: %d p1: %d p2: %d p3: %d data: %s",
						id, elem, p1, p2, p3, data);
					
	if (strncasecmp(com[id].elem[elem].d_type, "asc", 3) == 0) {
		for (i = 0; i < strlen(data); i++)
			sprintf(&msg[i * 2], "%02x", data[i]);

	} else if (strncasecmp(com[id].elem[elem].d_type, "bcd", 3) == 0) {
		if (p1 > 0) {
			i = (int) (atof(data) / com[id].elem[elem].d_fac);

			ret = eb_int_to_bcd(i, &bcd);
			sprintf(msg, "%02x", bcd);
			
		} else {
			goto on_error;
		}
				
	} else if (strncasecmp(com[id].elem[elem].d_type, "d1b", 3) == 0) {
		if (p1 > 0) {
			i = (int) (atof(data) / com[id].elem[elem].d_fac);
			
			ret = eb_int_to_d1b(i, &d1b);
			sprintf(msg, "%02x", d1b);
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(com[id].elem[elem].d_type, "d1c", 3) == 0) {
		if (p1 > 0) {
			f = (atof(data) / com[id].elem[elem].d_fac);
			
			ret = eb_float_to_d1c(f, &d1c);
			sprintf(msg, "%02x", d1c);
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(com[id].elem[elem].d_type, "d2b", 3) == 0) {
		if (p1 > 0 && p2 > 0) {
			f = (atof(data) / com[id].elem[elem].d_fac);
			
			if (p1 > p2)
				ret = eb_float_to_d2b(f, &d2b[1], &d2b[0]);
			else
				ret = eb_float_to_d2b(f, &d2b[0], &d2b[1]);
				
			sprintf(msg, "%02x%02x", d2b[0], d2b[1]);	
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(com[id].elem[elem].d_type, "d2c", 3) == 0) {
		if (p1 > 0 && p2 > 0) {
			f = (atof(data) / com[id].elem[elem].d_fac);
			
			if (p1 > p2)
				ret = eb_float_to_d2c(f, &d2c[1], &d2c[0]);
			else
				ret = eb_float_to_d2c(f, &d2c[0], &d2c[1]);
				
			sprintf(msg, "%02x%02x", d2c[0], d2c[1]);
		} else {
			goto on_error;
		}
					
	} else if (strncasecmp(com[id].elem[elem].d_type, "hda", 3) == 0) {
		if (p1 > 0 && p2 > 0 && p3 > 0) {
			int dd, mm, yy;

			dd = atoi(strtok(data, " .\n"));
			mm = atoi(strtok(NULL, " .\n"));
			yy = atoi(strtok(NULL, " .\n"));
					
			ret = eb_str_to_dat(dd, mm, yy, msg);
			if (ret < 0)
				sprintf(buf, " error ==> %d.%d.%d ", dd, mm, yy);
			
		} else {
			goto on_error;
		}
			
	} else if (strncasecmp(com[id].elem[elem].d_type, "hti", 3) == 0) {
		if (p1 > 0 && p2 > 0 && p3 > 0) {
			int hh, mm, ss;

			hh = atoi(strtok(data, " :\n"));
			mm = atoi(strtok(NULL, " :\n"));
			ss = atoi(strtok(NULL, " :\n"));
					
			ret = eb_str_to_tim(hh, mm, ss, msg);
			if (ret < 0)
				sprintf(buf, " error ==> %d:%d:%d ", hh, mm, ss);

		} else {
			goto on_error;
		}

	} else if (strncasecmp(com[id].elem[elem].d_type, "hdy", 3) == 0) {
		if (p1 > 0) {
			int day;

			day = atoi(data);
			sprintf(msg, "%02x", day);

		} else {
			goto on_error;
		}

	}
		
	return 0;

on_error:
	strcpy(buf, "error encode");
	return -1;
	
}

int
eb_cmd_encode(int id, char *data, unsigned char *msg, char *buf)
{
	char *tok, *toksave;
	unsigned char hlp[CMD_SIZE_S_MSG+1];
	int ret, i;
		
	tok = strtok_r(data, " \n\r\t", &toksave);
	
	for (i = 0; i < com[id].d_elem; i++) {
		if (tok != NULL) {					
			memset(hlp, '\0', sizeof(hlp));		
			ret = eb_cmd_encode_value(id, i, tok, hlp, buf);
			if (ret < 0) {
				strncat(buf, "\n", 1);
				return -1;
			}

			strncat(msg, hlp, strlen(hlp));
	
		} else {
			return -1;
		}
		
		tok = strtok_r(NULL, " \n\r\t", &toksave);	
	}

	if (strlen(buf) > 0)
		strncat(buf, "\n", 1);
		
	return 0;
}

int
eb_cmd_decode_value(int id, int elem, unsigned char *msg, char *buf)
{
	char *c1, *c2, *c3, byte;
	char d_pos[CMD_SIZE_D_POS+1];
	int ret, i, p1, p2, p3;
	float f;	

	memset(d_pos, '\0', sizeof(d_pos));
	strncpy(d_pos, com[id].elem[elem].d_pos, strlen(com[id].elem[elem].d_pos));

	c1 = strtok(d_pos, " ,\n");
	c2 = strtok(NULL, " ,\n");
	c3 = strtok(NULL, " ,\n");

	p1 = c1 ? atoi(c1) : 0;
	p2 = c2 ? atoi(c2) : 0;
	p3 = c3 ? atoi(c3) : 0;

	log_print(L_DBG, "id: %d elem: %d p1: %d p2: %d p3: %d",
							id, elem, p1, p2, p3);	


	if (strncasecmp(com[id].elem[elem].d_type, "asc", 3) == 0) {
		sprintf(buf, "%s ", &msg[1]);

	} else if (strncasecmp(com[id].elem[elem].d_type, "bcd", 3) == 0) {
		if (p1 > 0) {
			ret = eb_bcd_to_int(msg[p1], &i);

			i *= com[id].elem[elem].d_fac;
			sprintf(buf, "%3d ", i);
		} else {
			goto on_error;
		}
				
	} else if (strncasecmp(com[id].elem[elem].d_type, "d1b", 3) == 0) {
		if (p1 > 0) {
			ret = eb_d1b_to_int(msg[p1], &i);

			f = i * com[id].elem[elem].d_fac;
			sprintf(buf, "%6.2f ", f);
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(com[id].elem[elem].d_type, "d1c", 3) == 0) {
		if (p1 > 0) {
			ret = eb_d1c_to_float(msg[p1], &f);

			f *= com[id].elem[elem].d_fac;
			sprintf(buf, "%6.2f ", f);			
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(com[id].elem[elem].d_type, "d2b", 3) == 0) {
		if (p1 > 0 && p2 > 0) {
			if (p1 > p2)
				ret = eb_d2b_to_float(msg[p2], msg[p1], &f);
			else
				ret = eb_d2b_to_float(msg[p1], msg[p2], &f);
				
			f *= com[id].elem[elem].d_fac;
			sprintf(buf, "%8.3f ", f);		
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(com[id].elem[elem].d_type, "d2c", 3) == 0) {
		if (p1 > 0 && p2 > 0) {
			if (p1 > p2)
				ret = eb_d2c_to_float(msg[p2], msg[p1], &f);
			else	
				ret = eb_d2c_to_float(msg[p1], msg[p2], &f);
			
			f *= com[id].elem[elem].d_fac;
			sprintf(buf, "%10.4f ", f);
		} else {
			goto on_error;
		}
					
	} else if (strncasecmp(com[id].elem[elem].d_type, "bda", 3) == 0) {
		if (p1 > 0 && p2 > 0 && p3 > 0) {
			int dd, mm, yy;
			ret = eb_bcd_to_int(msg[p1], &dd);
			ret = eb_bcd_to_int(msg[p2], &mm);
			ret = eb_bcd_to_int(msg[p3], &yy);
			
			ret = eb_dat_to_str(dd, mm, yy, buf);
			if (ret < 0)
				sprintf(buf, " error %s ==> %02x %02x %02x ",
				com[id].elem[elem].d_sub, msg[p1], msg[p2], msg[p3]);
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(com[id].elem[elem].d_type, "hda", 3) == 0) {
		if (p1 > 0 && p2 > 0 && p3 > 0) {
			ret = eb_dat_to_str(msg[p1], msg[p2], msg[p3], buf);
			if (ret < 0)
				sprintf(buf, " error %s ==> %02x %02x %02x ",
				com[id].elem[elem].d_sub, msg[p1], msg[p2], msg[p3]);
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(com[id].elem[elem].d_type, "bti", 3) == 0) {
		if (p1 > 0 && p2 > 0 && p3 > 0) {
			int hh, mm, ss;

			ret = eb_bcd_to_int(msg[p1], &hh);
			ret = eb_bcd_to_int(msg[p2], &mm);
			ret = eb_bcd_to_int(msg[p3], &ss);
			
			ret = eb_tim_to_str(hh, mm, ss, buf);
			if (ret < 0)
				sprintf(buf, " error %s ==> %02x %02x %02x ",
				com[id].elem[elem].d_sub, msg[p1], msg[p2], msg[p3]);
		} else {
			goto on_error;
		}
					
	} else if (strncasecmp(com[id].elem[elem].d_type, "hti", 3) == 0) {
		if (p1 > 0 && p2 > 0 && p3 > 0) {
			ret = eb_tim_to_str(msg[p1], msg[p2], msg[p3], buf);
			if (ret < 0)
				sprintf(buf, " error %s ==> %02x %02x %02x ",
				com[id].elem[elem].d_sub, msg[p1], msg[p2], msg[p3]);
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(com[id].elem[elem].d_type, "bdy", 3) == 0) {
		if (p1 > 0)
			ret = eb_day_to_str(msg[p1], buf);
		else
			goto on_error;
			
	} else if (strncasecmp(com[id].elem[elem].d_type, "hdy", 3) == 0) {
		if (p1 > 0) {
			msg[p1] = msg[p1] - 0x01;
			ret = eb_day_to_str(msg[p1], buf);
		} else {
			goto on_error;
		}
		
	} else if (strncasecmp(com[id].elem[elem].d_type, "hex", 3) == 0) {
		for (i = 0; i < msg[0]; i++)
			sprintf(&buf[3 * i], "%02x ", msg[i + 1]);

	//~ } else if (strncasecmp(com[id].elem[elem].d_type, "mmt", 3) == 0) {
		//~ if (p1 > 0) {
			//~ if (msg[p1] == 0x00) {
				//~ strcpy(buf, "ACK ");
			//~ } else {
				//~ strcpy(buf, "NAK ");
			//~ }
		//~ }

	}
		
	return 0;

on_error:
	strcpy(buf, "error decode");
	return -1;

}

int
eb_cmd_decode(int id, char *data, unsigned char *msg, char *buf)
{
	char *tok;
	char tmp[CMD_DATA_SIZE], hlp[CMD_DATA_SIZE];
	int ret, i, found;

	for (i = 0; i < com[id].d_elem; i++) {
		memset(tmp, '\0', sizeof(tmp));
		strncpy(tmp, data, strlen(data));

		tok = strtok(tmp, " \n\r\t");
		if (strncasecmp(tok, "-", 1) == 0)
			found = YES;
		else
			found = NO;
		
		while (tok != NULL && found == NO) {		
			if (strncasecmp(com[id].elem[i].d_sub, tok,
					strlen(com[id].elem[i].d_sub)) == 0) {
				found = YES;
				break;
			}
				
			tok = strtok(NULL, " \n\r\t");
		}

		if (found == YES) {
			memset(hlp, '\0', sizeof(hlp));
			ret = eb_cmd_decode_value(id, i, msg, hlp);
			if (ret < 0) {
				strncat(buf, "\n", 1);
				return -1;
			}
			
			strncat(buf, hlp, strlen(hlp));
		}				
	}
		
	return 0;
}

void
eb_cmd_print(const char *type, int all, int detail)
{
	int i, j;

	for (i = 0; i < comlen; i++) {

		if (strncasecmp(com[i].type, type, 1) == 0 || all) {

			log_print(L_INF, "[%03d] %s : %5s.%-32s\t(type: %d)" \
					 " %s%s%-10s (len: %d) [%d] ==> %s",
				com[i].id,
				com[i].type,
				com[i].class,
				com[i].cmd,
				com[i].s_type,
				com[i].s_zz,
				com[i].s_cmd,
				com[i].s_msg,
				com[i].s_len,
				com[i].d_elem,		
				com[i].com			
				);

			if (detail) {
				for (j = 0; j < com[i].d_elem; j++) {
					log_print(L_INF, "\t\t  %-20s pos: " \
						"%-10s\t%s [%5.2f] [%s] \t%s\t%s",
						com[i].elem[j].d_sub,
						com[i].elem[j].d_pos,
						com[i].elem[j].d_type,
						com[i].elem[j].d_fac,
						com[i].elem[j].d_unit,
						com[i].elem[j].d_valid,
						com[i].elem[j].d_com
				
						);
				}
				log_print(L_INF, "");
			}

			
		}
	}
}

int
eb_cmd_fill(const char *tok)
{
	int i;
	
	com = (struct commands *) realloc(com, (comlen + 1) * sizeof(struct commands));
	err_ret_if(com == NULL, -1);

	memset(com + comlen, '\0', sizeof(struct commands));
	
	/* id */
	com[comlen].id = comlen;

	/* type */
	strncpy(com[comlen].type, tok, strlen(tok));
	
	/* class */
	tok = strtok(NULL, ";");
	strncpy(com[comlen].class, tok, strlen(tok));

	
	/* cmd */
	tok = strtok(NULL, ";");
	strncpy(com[comlen].cmd, tok, strlen(tok));

	/* com */
	tok = strtok(NULL, ";");
	strncpy(com[comlen].com, tok, strlen(tok));
	
	/* s_type */
	tok = strtok(NULL, ";");
	com[comlen].s_type = atoi(tok);	
	
	/* s_zz */
	tok = strtok(NULL, ";");
	strncpy(com[comlen].s_zz, tok, strlen(tok));
	
	/* s_cmd */
	tok = strtok(NULL, ";");
		strncpy(com[comlen].s_cmd, tok, strlen(tok));
	
	/* s_len */
	tok = strtok(NULL, ";");
	com[comlen].s_len = atoi(tok);

	/* s_msg */
	tok = strtok(NULL, ";");
	if (strncasecmp(tok, "-", 1) != 0)
		strncpy(com[comlen].s_msg, tok, strlen(tok));
	
	/* d_elem */
	tok = strtok(NULL, ";");
	com[comlen].d_elem = atoi(tok);
	
	com[comlen].elem = (struct element *) malloc(com[comlen].d_elem * sizeof(struct element));
	err_ret_if(com[comlen].elem == NULL, -1);

	memset(com[comlen].elem, '\0', com[comlen].d_elem * sizeof(struct element));	

	for (i = 0; i < com[comlen].d_elem; i++) {
		
		/* d_sub */
		tok = strtok(NULL, ";");
		if (strncasecmp(tok, "-", 1) != 0)
			strncpy(com[comlen].elem[i].d_sub, tok, strlen(tok));
		
		/* d_pos */
		tok = strtok(NULL, ";");	
		strncpy(com[comlen].elem[i].d_pos, tok, strlen(tok));
		
		/* d_type */
		tok = strtok(NULL, ";");
		strncpy(com[comlen].elem[i].d_type, tok, strlen(tok));				
		
		/* d_fac */
		tok = strtok(NULL, ";");	
		com[comlen].elem[i].d_fac = atof(tok);
		
		/* d_unit */
		tok = strtok(NULL, ";");	
		strncpy(com[comlen].elem[i].d_unit, tok, strlen(tok));

		/* d_valid */
		tok = strtok(NULL, ";");	
		strncpy(com[comlen].elem[i].d_valid, tok, strlen(tok));

		/* d_com */
		tok = strtok(NULL, ";");
		strncpy(com[comlen].elem[i].d_com, tok, strlen(tok));
		com[comlen].elem[i].d_com[strcspn(com[comlen].elem[i].d_com, "\n")] = '\0';
		
	}
	
	if (strncasecmp(com[comlen].type, "cyc", 3) == 0) {

		cyc = (struct cycbuf *) realloc(cyc, (cyclen + 1) * sizeof(struct cycbuf));
		err_ret_if(cyc == NULL, -1);

		memset(cyc + cyclen, '\0', sizeof(struct cycbuf));		

		/* id */
		cyc[cyclen].id = com[comlen].id;

		/* msg */
		sprintf(cyc[cyclen].msg, "%s%s%02X%s",
					com[comlen].s_zz, com[comlen].s_cmd,
					com[comlen].s_len, com[comlen].s_msg);
			
		cyclen++;
	}

	comlen++;

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
	int ret;
	char line[CMD_LINELEN + 1];
	char *tok;
	FILE *fp = NULL;

	log_print(L_NOT, "%s", file);

	/* open config file */
	fp = fopen(file, "r");
	err_ret_if(fp == NULL, -1);			

	/* read each line and fill cmd array */
	while (fgets(line, CMD_LINELEN, fp) != NULL) {
		tok = strtok(line, ";\n");
			
		if (tok != NULL && tok[0] != '#' ) {

			ret = eb_cmd_fill(tok);
			if (ret < 0)
				return -2;
				
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
	int i;

	if (comlen > 0) {
		for (i = 0; i < comlen; i++)
			free(&com[i].elem[0]);
	
		free(com);
	}

	if (cyclen > 0)
		free(cyc);
	
}



int
eb_raw_file_open(const char *file)
{
	rawfp = fopen(file, "w");
	err_ret_if(rawfp == NULL, -1);

	return 0;
}

int
eb_raw_file_close(void)
{
	int ret;

	ret = fflush(rawfp);
	err_ret_if(ret == EOF, -1);

	ret = fclose(rawfp);
	err_ret_if(ret == EOF, -1);

	return 0;
}

int
eb_raw_file_write(const unsigned char *buf, int buflen)
{
	int ret, i;

	for (i = 0; i < buflen; i++) {		
		ret = fputc(buf[i], rawfp);
		err_ret_if(ret == EOF, -1);
	}

	ret = fflush(rawfp);
	err_ret_if(ret == EOF, -1);

	return 0;
}

void
eb_raw_print_hex(const unsigned char *buf, int buflen)
{
	int i = 0;
	char msg[SERIAL_BUFSIZE];
	char tmp[4];

	memset(tmp, '\0', sizeof(tmp));
	memset(msg, '\0', sizeof(msg));
	
	for (i = 0; i < buflen; i++) {
		sprintf(tmp, " %02x", buf[i]);
		strncat(msg, tmp, 3);
	}
	log_print(L_EBH, "%s", msg);
}



int
eb_serial_open(const char *dev, int *fd)
{
	int ret;
	struct termios newtio;

	sfd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
	err_ret_if(sfd < 0, -1);

	ret = fcntl(sfd, F_SETFL, 0);
	err_ret_if(ret < 0, -1);

	/* save current settings of serial port */
	ret = tcgetattr(sfd, &oldtio);
	err_ret_if(ret < 0, -1);

	memset(&newtio, '\0', sizeof(newtio));

	newtio.c_cflag = SERIAL_BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag &= ~OPOST;

	newtio.c_cc[VMIN]  = 1;
	newtio.c_cc[VTIME] = 0;

	ret = tcflush(sfd, TCIFLUSH);
	err_ret_if(ret < 0, -1);

	/* activate new settings of serial port */
	ret = tcsetattr(sfd, TCSANOW, &newtio);
	err_ret_if(ret < 0, -1);

	*fd = sfd;

	return 0;
}

int
eb_serial_close(void)
{
	int ret;

	/* activate old settings of serial port */
	ret = tcsetattr(sfd, TCSANOW, &oldtio);
	err_ret_if(ret < 0, -1);

	/* Close file descriptor from serial device */
	ret = close(sfd);
	err_ret_if(ret < 0, -1);

	return 0;
}

int
eb_serial_send(const unsigned char *buf, int buflen)
{
	int ret, val;
	
	/* write msg to ebus device */
	val = write(sfd, buf, buflen);
	err_ret_if(val < 0, -1);
	
	ret = tcflush(sfd, TCIOFLUSH);
	err_ret_if(ret < 0, -1);
	
	return val;
}

int
eb_serial_recv(unsigned char *buf, int *buflen)
{
	int ret;
	
	//tcflush(sfd, TCIOFLUSH);
	/* read msg from ebus device */
	*buflen = read(sfd, buf, *buflen);
	err_if(*buflen < 0);

	if (*buflen < 0) {
		log_print(L_WAR, "error read serial device");
		return -1;
	}

	if (*buflen > SERIAL_BUFSIZE) {
		log_print(L_WAR, "read data len > %d", SERIAL_BUFSIZE);
		return -2;
	}

	/* print bus */
	if (showraw == YES)
		eb_raw_print_hex(buf, *buflen);

	/* dump raw data*/
	if (rawdump == YES) {
		ret = eb_raw_file_write(buf, *buflen);
		if (ret < 0)
			log_print(L_WAR, "can't write rawdata");
	}

	return 0;
}



void
eb_print_result(void)
{
	int i;
	//fprintf(stdout, ">>> ");
	for (i = 0; i < recv_data.len; i++)
		fprintf(stdout, " %02x", recv_data.msg[i]);
	fprintf(stdout, "\n");
}



void
eb_recv_data_get(unsigned char *buf, int *buflen)
{
	strncpy(buf, recv_data.msg, recv_data.len);	
	*buflen = recv_data.len;
}

void
eb_recv_data_prepare(const unsigned char *buf, int buflen)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, crc;

	memset(tmp, '\0', sizeof(tmp));

	/* reset struct */
	memset(&recv_data, '\0', sizeof(recv_data));	

	/* set recv_data.msg_esc */
	memcpy(&recv_data.msg_esc[0], buf, buflen);
	recv_data.len_esc = buflen;

	/* set recv_data.crc_calc and .crc_recv */
	if (buf[buflen - 2] == EBUS_SYN_ESC_A9) {
		recv_data.crc_calc = eb_calc_crc(buf, buflen - 2);
		if (buf[buflen - 1] == EBUS_SYN_ESC_01)
			recv_data.crc_recv = EBUS_SYN;
		else
			recv_data.crc_recv = EBUS_SYN_ESC_A9;

		crc = 2;

	} else {
		recv_data.crc_calc = eb_calc_crc(buf, buflen - 1);
		recv_data.crc_recv = buf[buflen - 1];

		crc = 1;
	}

	/* set recv_data.msg */
	memcpy(tmp, buf, buflen - crc);
	tmplen = buflen - crc;

	eb_unesc(tmp, &tmplen);

	memcpy(&recv_data.msg[0], tmp, tmplen);
	recv_data.len = tmplen;

}

int
eb_recv_data(unsigned char *buf, int *buflen)
{
	unsigned char tmp[SERIAL_BUFSIZE], msg[SERIAL_BUFSIZE];
	int tmplen, msglen, ret, i, esc, found;

	memset(msg, '\0', sizeof(msg));
	msglen = 0;
	
	esc = 0;
	found = 99;
	
	/* do until found necessary string*/
	do {
		memset(tmp, '\0', sizeof(tmp));
		tmplen = sizeof(tmp);

		ret = eb_serial_recv(tmp, &tmplen);
		if (ret < 0)
			return -1;
		
		if (tmplen > 0) {

			/* preset tmp buffer with not read bytes from get_bus */
			if (*buflen > 0) {
				
				/* save temporarily tmp in msg buffer */
				memcpy(msg, tmp, tmplen);
				msglen = tmplen;

				/* copy input data into tmp buffer */
				memset(tmp, '\0', sizeof(tmp));
				memcpy(tmp, buf, *buflen);
				tmplen = *buflen;

				/* set input data buffer len to 0 */
				*buflen = 0;

				/* copy saved bus data back to tmp buffer */
				memcpy(&tmp[tmplen], msg, msglen);
				tmplen += msglen;

				/* reset msg buffer */
				memset(msg, '\0', sizeof(msg));
				msglen = 0;							
			}
			
			
			i = 0;
			while (i < tmplen) {

				msg[msglen] = tmp[i];
				msglen++;
						
				/* SYN */
				if (msg[0] == EBUS_SYN) {
					found = -2;
					break;
				}

				/* get end of message */
				if (msglen > 1) {

					if (msg[msglen - 1] == EBUS_SYN_ESC_A9)
						esc++;

					if (msglen == (2 + msg[0] + esc)) {
						found = 0;
						break;
					}
				}

				/*
				 * something went wrong - plz tell me
				 * we got some SYN during answer or
				 * msglen is out of spec. 1 
				 */
				if (msg[msglen] == EBUS_SYN || msg[0] > 16) {

					found = -3;
					break;
				}

				i++;
			}
		}
	} while (found == 99);

	*buflen = msglen;

	memset(buf, '\0', sizeof(buf));
	for (i = 0; i < msglen; i++)
		buf[i] = msg[i];
		
	return found;
}



int
eb_get_ack(unsigned char *buf, int *buflen)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, ret, i, j, found;
	
	j = 0;
	found = 99;

	/* do until found necessary string */
	do {
		memset(tmp, '\0', sizeof(tmp));
		tmplen = sizeof(tmp);

		ret = eb_serial_recv(tmp, &tmplen);
		if (ret < 0)
			return -1;	
		
		if (tmplen > 0) {		
			i = 0;
			while (i < tmplen) {
			
				/* compare recv with sent  - is this possible */
				if (tmp[i] != buf[j] && j < *buflen)
					return -2;
				
				/* compare only slaves answer */
				if (j > (*buflen - 1)) {									

					/* ACK */
					if (tmp[i] == EBUS_ACK)
						found = 0;
					
					/* NAK */
					else if (tmp[i] == EBUS_NAK)
						found = 0;
					
					/* SYN */
					else if (tmp[i] == EBUS_SYN)
						found = -3;
					
					/* ??? */
					else 
						found = -4;
					i++;
					break;
					
				}
				i++;
				j++;
			}
		}
	} while (found == 99);

	*buflen = tmplen - i;

	memset(buf, '\0', sizeof(buf));
	for (j = 0; i < tmplen; i++, j++)
		buf[j] = tmp[i];

	return found;
}



int
eb_wait_bus_syn(int *skip)
{
	unsigned char buf[SERIAL_BUFSIZE];
	int buflen, ret, i, found;
	
	found = 99;

	/* do until SYN read*/
	do {
		memset(buf, '\0', sizeof(buf));
		buflen = sizeof(buf);

		ret = eb_serial_recv(buf, &buflen);
		if (ret < 0)
			return -1;		

		if (buflen > 0) {
			
			i = 0;
			while (i < buflen) {
				/* break if byte = SYN and it is last byte */
				if (buf[i] == EBUS_SYN && (i + 1) == buflen) {
					found = 1;
					break;
				}
				i++;
			}

			if (*skip > 0)
				*skip -= 1;
		}
	} while (found == 99);
	
	return 0;
}

int
eb_wait_bus(void)
{
	unsigned char buf[SERIAL_BUFSIZE];
	int buflen, ret, skip, retry;
	struct timeval tact, tlast, tdiff;

	skip = 0;
	retry = 0;
	
	do {
		ret = eb_wait_bus_syn(&skip);
		if (ret < 0)
			return -1;	

		/* remember start time */
		gettimeofday(&tlast, NULL);

		/* send QQ */
		ret = eb_serial_send(&qq, 1);
		if (ret < 0)
			return -1;

		gettimeofday(&tact, NULL);
		eb_diff_time(&tact, &tlast, &tdiff);

		/* wait ~4200 usec */
		usleep(max_wait - tdiff.tv_usec);

		gettimeofday(&tact, NULL);
		eb_diff_time(&tact, &tlast, &tdiff);

		/* receive 1 byte - must be QQ */
		memset(buf, '\0', sizeof(buf));
		buflen = sizeof(buf);
		ret = eb_serial_recv(buf, &buflen);
		if (ret < 0)
			return -1;		

		/* is sent and read qq byte is equal */
		if (buf[0] == qq && buflen == 1)
			return 0;

		retry++;
		skip = skip_ack + retry;
	
	} while (retry < get_retry);

	/* reached max retry */
	return 1;
}

int
eb_free_bus(void)
{
	int ret, skip;
	ret = 0;
	skip = 0;

	ret = eb_serial_send(&syn, 1);
	if (ret < 0)
		return -1;
		
	ret = eb_wait_bus_syn(&skip);
	if (ret < 0)
		return -1;			

	return 0;
}




void
eb_send_data_prepare(const unsigned char *buf, int buflen)
{
	unsigned char crc[2], tmp[SERIAL_BUFSIZE];
	int tmplen, crclen;

	/* reset struct */
	memset(&send_data, '\0', sizeof(send_data));	

	/* set send_data.msg */
	memcpy(&send_data.msg[0], buf, buflen);
	send_data.len = buflen;


	/* set send_data.msg_esc + send_data.crc */
	memset(tmp, '\0', sizeof(tmp));
	memcpy(tmp, buf, buflen);
	tmplen = buflen;	
	
	eb_esc(tmp, &tmplen);

	memcpy(&send_data.msg_esc[0], &qq, 1);
	memcpy(&send_data.msg_esc[1], tmp, tmplen);
	tmplen++;

	memset(crc, '\0', sizeof(crc));
	send_data.crc = eb_calc_crc(&send_data.msg_esc[0], tmplen);
	crc[0] = send_data.crc;
	crclen = 1;

	if (crc[0] == EBUS_SYN || crc[0] == EBUS_SYN_ESC_A9) {
		/* esc crc */
		eb_esc(crc, &crclen);
		send_data.msg_esc[tmplen] = crc[0];
		tmplen++;
		send_data.msg_esc[tmplen] = crc[1];
		tmplen++;
	} else {
		send_data.msg_esc[tmplen] = crc[0];
		tmplen++;
	}

	send_data.len_esc = tmplen;

}

int
eb_send_data(const unsigned char *buf, int buflen, int type)
{
	unsigned char tmp[SERIAL_BUFSIZE];
	int tmplen, ret, val, i;
	ret = 0;
	i = 0;

	eb_send_data_prepare(buf, buflen);
	
	/* fetch AA and send QQ */
	ret = eb_wait_bus();
	if (ret != 0)
		return -1;
	
	/* send message to slave */
	ret = eb_serial_send(&send_data.msg_esc[1], send_data.len_esc - 1);
	if (ret < 0)
		return -1;

	if (type == EBUS_MSG_BROADCAST) {
		/* free bus */
		ret = eb_free_bus();
		if (ret < 0)
			return -1;		
		
		return 0;
	}

	/* get data from bus (we got our sent message too) */
	memset(tmp, '\0', sizeof(tmp));
	memcpy(tmp, &send_data.msg_esc[1], send_data.len_esc - 1);
	tmplen = send_data.len_esc - 1;

	ret = eb_get_ack(tmp, &tmplen);

	if (ret < 0 || ret > 1) {
		/* free bus */
		ret = eb_free_bus();
		if (ret < 0)
			return -1;		
	
		return -1;
	}

	/* first answer from slave is NAK - send message again (inkl. QQ) */
	if (ret == 1) {
			
		/* send message to slave */
		ret = eb_serial_send(&send_data.msg_esc[0], send_data.len_esc);
		if (ret < 0)
			return -1;

		/* get ack from bus (we got our sent message too) */
		memset(tmp, '\0', sizeof(tmp));
		memcpy(tmp, &send_data.msg_esc[0], send_data.len_esc);
		tmplen = send_data.len_esc;

		ret = eb_get_ack(tmp, &tmplen);		
	
		if (ret == 1) {
			/* free bus */
			ret = eb_free_bus();
			if (ret < 0)
				return -1;
	
			return -1;
		}
		
	}

	if (type == EBUS_MSG_MASTER_MASTER) {
		val = ret;
	
		/* free bus */
		ret = eb_free_bus();
		if (ret < 0)
			return -1;		

		return val;
	}

	/* get data - dont reset buffer */	
	ret = eb_recv_data(tmp, &tmplen);
	if (ret < 0)
		return -1;

	eb_recv_data_prepare(tmp, tmplen);

	/* check crc's from recv_data */
	if (recv_data.crc_calc != recv_data.crc_recv) {
		/* send message to slave */
		ret = eb_serial_send(&nak, 1);
		if (ret < 0)
			return -1;

		/* get data from bus (we got our sent message too) */
		memset(tmp, '\0', sizeof(tmp));
		tmplen = 0;

		ret = eb_get_ack(tmp, &tmplen);		

		/* we compare against nak ! */
		if (ret != 1) {
			/* free bus */
			ret = eb_free_bus();
			if (ret < 0)
				return -1;			
		
			return -1;
		}

		/* get data - don't reset buffer */
		ret = eb_recv_data(tmp, &tmplen);	
		if (ret < 0)
			return -1;

		eb_recv_data_prepare(tmp, tmplen);
		
	}

	if (recv_data.crc_calc != recv_data.crc_recv) {
		ret = eb_serial_send(&nak, 1);		
		if (ret < 0)
			return -1;
		
		val = 1;
	} else {
		ret = eb_serial_send(&ack, 1);
		if (ret < 0)
			return -1;
			
		val = 0;
	}

	/* free bus */
	ret = eb_free_bus();
	if (ret < 0)
		return -1;	
			
	return val;
}



void
eb_execute_prepare(int id, char *data, char *msg, int *msglen, int *msgtype, char *buf)
{
	unsigned char tmp[CMD_SIZE_S_MSG+1];
	char str[CMD_SIZE_S_ZZ + CMD_SIZE_S_CMD + 2 + CMD_SIZE_S_MSG+1];
	char byte;
	int ret, i, j, k;
	int in[SERIAL_BUFSIZE];	

	*msgtype = com[id].s_type;

	/* encode msg */
	memset(tmp, '\0', sizeof(tmp));
	if (strncasecmp(com[id].type, "set", 3) == 0)
		eb_cmd_encode(id, data, tmp, buf);

	memset(str, '\0', sizeof(str));
	sprintf(str, "%s%s%02X%s%s",
		com[id].s_zz, com[id].s_cmd, com[id].s_len, com[id].s_msg, tmp);

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
	
}

void
eb_execute(int id, char *data, char *buf, int *buflen)
{
	unsigned char msg[SERIAL_BUFSIZE];
	int msglen, msgtype, ret, retry, cycdata, i;

	memset(msg, '\0', sizeof(msg));
	msglen = sizeof(msg);
	msgtype = UNSET;
				
	cycdata = NO;
	ret = -1;
	retry = 0;
	
	if (strncasecmp(com[id].type, "cyc", 3) != 0) {
		/* prepare command - if prepare failed buflen > 0 */
		eb_execute_prepare(id, data, msg, &msglen, &msgtype, buf);
		eb_raw_print_hex(msg, msglen);
	} else {
		cycdata = YES;
	}

	if (cycdata == NO && strlen(buf) == 0) {
		/* send data to bus */
		while (ret < 0 && retry < send_retry) {
			if (retry > 0)
				log_print(L_NOT, "send retry: %d", retry);
					
			ret = eb_send_data(msg, msglen, msgtype);
			retry++;
		}
	}

	/* handle answer for sent messages */
	if (cycdata == NO && ret >= 0)  {
		
		if (msgtype == EBUS_MSG_BROADCAST)
			strcpy(buf, " broadcast done\n");

		if (msgtype == EBUS_MSG_MASTER_MASTER) {
			if (ret == 0) {
				strcpy(buf, " ACK\n");
			} else {
				strcpy(buf, " NAK\n");
			}
		}

		if (msgtype == EBUS_MSG_MASTER_SLAVE) {
			if (ret == 0) {
				memset(msg, '\0', sizeof(msg));
				msglen = sizeof(msg);
				eb_recv_data_get(msg, &msglen);
				eb_raw_print_hex(msg, msglen);

				/* decode */
				if (strncasecmp(com[id].type, "set", 3) == 0) {
					strcpy(buf, " ACK\n");
				} else {
					eb_cmd_decode(id, data, msg, buf);
					if (strlen(buf) > 0)
						strncat(buf, "\n", 1);
				}
				
			} else {
				strcpy(buf, " NAK\n");
			}
		}

	} else if (cycdata == YES) {
		for (i = 0; i < cyclen; i++)
			if (cyc[i].id == com[id].id)
				break;
			
		eb_cmd_decode(id, data, cyc[i].buf, buf);
		if (strlen(buf) > 0)
			strncat(buf, "\n", 1);
		else
			strcpy(buf, " error get cyc data\n");
			
	} else {
		strcpy(buf, " error send ebus msg\n");
	}
	
	*buflen = strlen(buf);
	
}



void
eb_cyc_data_process(const unsigned char *buf, int buflen)
{
	unsigned char msg[CMD_DATA_SIZE], hlp[CMD_DATA_SIZE];
	unsigned char crc_recv, crc_calc;
	char tmp[CMD_DATA_SIZE];
	int ret, crc, len, msglen, hlplen;

	memset(msg, '\0', sizeof(msg));
	memset(hlp, '\0', sizeof(hlp));
	memset(tmp, '\0', sizeof(tmp));

	/* decode and save data */
	ret = eb_search_cyc(&buf[1], buflen - 1);
	if (ret >= 0) {
		
		if (com[ret].s_type == EBUS_MSG_BROADCAST) {
			/* calculate CRC */
			if (buf[buflen - 2] == EBUS_SYN_ESC_A9) {
				crc_calc = eb_calc_crc(buf, buflen - 2);
				if (buf[buflen - 1] == EBUS_SYN_ESC_01)
					crc_recv = EBUS_SYN;
				else
					crc_recv = EBUS_SYN_ESC_A9;

				crc = 2;

			} else {
				crc_calc = eb_calc_crc(buf, buflen - 1);
				crc_recv = buf[buflen - 1];

				crc = 1;
			}

			if (crc_calc == crc_recv) {			
				/* unescape */
				memcpy(msg, buf, buflen - crc);
				msglen = buflen - crc;

				eb_unesc(msg, &msglen);

				/* save data */
				memcpy(cyc[ret].buf, &msg[4], msglen - 4);
					
				/* decode and save */
				eb_cmd_decode(ret, "-", &msg[4], tmp);	
			} else {
				strcpy(tmp, "error CRC");
			}

			
			
		} else if (com[ret].s_type == EBUS_MSG_MASTER_MASTER) {
			/* set NN to 0x01 for MM */
			msg[0] = 0x01;
			memcpy(&msg[1], &buf[buflen - 1], 1);
			
			/* decode and save */
			eb_cmd_decode(ret, "-", &msg[0], tmp);
								
		} else if (com[ret].s_type == EBUS_MSG_MASTER_SLAVE) {
			memcpy(msg, buf, buflen);
			msglen = buflen;
			
			/* unescape first, we need only the answer */
			eb_unesc(msg, &msglen);

			/* get only answer NN Dx CRC */
			len = 4 + msg[4] + 1 + 2;			
			memcpy(hlp, &msg[len], msglen - len - 1);
			hlplen = msglen - len - 1;

			/* escape copied hlp buffer again */
			eb_esc(hlp, &hlplen);
				
			/* calculate CRC */
			if (hlp[hlplen - 2] == EBUS_SYN_ESC_A9) {
				crc_calc = eb_calc_crc(hlp, hlplen - 2);
				if (hlp[hlplen - 1] == EBUS_SYN_ESC_01)
					crc_recv = EBUS_SYN;
				else
					crc_recv = EBUS_SYN_ESC_A9;

				crc = 2;

			} else {
				crc_calc = eb_calc_crc(hlp, hlplen - 1);
				crc_recv = hlp[hlplen - 1];

				crc = 1;
			}

			if (crc_calc == crc_recv) {			
				/* unescape */
				memset(msg, '\0', sizeof(msg));
				memcpy(msg, hlp, hlplen - crc);
				msglen = hlplen - crc;

				eb_unesc(msg, &msglen);

				/* save data */
				memcpy(cyc[ret].buf, msg, msglen);
					
				/* decode and save */
				eb_cmd_decode(ret, "-", msg, tmp);	
			} else {
				strcpy(tmp, "error CRC");
			}
					
		}

		if (strlen(tmp) > 0)
			log_print(L_EBS, "%s", tmp);
	}

	

}

int
eb_cyc_data_recv()
{
	static unsigned char msg[SERIAL_BUFSIZE];
	static int msglen = 0;
	
	unsigned char buf[SERIAL_BUFSIZE];
	int ret, i, buflen;

	memset(buf, '\0', sizeof(buf));
	buflen = sizeof(buf);

	if (msglen == 0)
		memset(msg, '\0', sizeof(msg));

	/* get new data */
	ret = eb_serial_recv(buf, &buflen);
	if (ret < 0)
		return -1;

	i = 0;
	while (i < buflen) {
		
		if (buf[i] != EBUS_SYN) {
			msg[msglen] = buf[i];
			msglen++;
		}

		/* ebus syn sign is reached - decode ebus message */
		if (buf[i] == EBUS_SYN && msglen > 0) {
			eb_raw_print_hex(msg, msglen);
			
			eb_cyc_data_process(msg, msglen);

			memset(msg, '\0', sizeof(msg));
			msglen = 0;
		}
		
		i++;
	}

	return msglen;
}

