/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Michael Markstaller 2011 <devel@wiregate.de>
 * 
 * russconnectd is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * russconnectd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <pthread.h>
#include <eibclient.h>


#define DEBUG 1
#define DAEMON_NAME "russconnectd"
#define USAGESTRING	"\n\t-d\tRun as daemon/No debug output\n\t-p <pidfile>\tPID-filename\n\t-i <ip:port>\tIP-Address:Port to send UDP-packets to russound\n\t-l <UDP-port>\tUDP port to listen on\n\t-a <KNX address>\tKNX start-address (see README)\n\t-z <number>\tNumber of Zones to support\n\t-u <eib url>\tURL to conatct eibd like localo:/tmp/eib or ip:192.168.0.101\n"
#define NUM_THREADS 2
#define MAX_ZONES 31
#define RETRY_TIME 5
#define BUFLEN 1024
#define POLLING_INTERVAL 60
#define ZONES_PER_CONTROLLER 6
#define RUSS_KEYPAD_ID 0x70

pthread_mutex_t zonelock = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t initlock = PTHREAD_MUTEX_INITIALIZER; 

//FIXME:make struct zone dynamic
typedef struct Zone_t {
	char name[255];
	signed char balance;
	signed char bass;
	int dnd;
	int loudness;
	int partymode;
	signed char treble;
	int volume;
	int zonepower;
	int srcid;
	int onvolume;
	char srctxt[255];
	char srcinfo1[255];
	char srcinfo2[255];
	int inited;
} Zone_t;

Zone_t zones[MAX_ZONES];

char *russipaddr = "127.0.0.1";
int russport = 16011;
char *eibd_url = "local:/tmp/eib";
int listenport = 16012;
int knxstartaddress = 50000;
int numzones = ZONES_PER_CONTROLLER;
int pidFilehandle;
char *pidfilename = "/var/run/russconnectd.pid";

void daemonShutdown() {
	//FIXME: clean exit pthread_exit(NULL); pthread_cancel(..);
	syslog(LOG_INFO, "%s daemon exiting", DAEMON_NAME);
    fprintf(stderr, "%s daemon exiting", DAEMON_NAME);	
	close(pidFilehandle);
	unlink(pidfilename);
	exit(EXIT_SUCCESS);
}

void signal_handler(int sig) {
    switch(sig) {
        case SIGHUP:
            syslog(LOG_WARNING, "Received SIGHUP signal.");
            break;
        case SIGTERM:
            syslog(LOG_WARNING, "Received SIGTERM signal.");
			daemonShutdown();
			break;
        case SIGINT:
            syslog(LOG_WARNING, "Received SIGINT signal.");
			daemonShutdown();
			break;
        default:
            syslog(LOG_WARNING, "Unhandled signal (%d) %s", sig, strsignal(sig));
            break;
    }
}

char *russChecksum(char* buf, int len) {
	int i,j=0;
	for (i=0;i<len;i++)
		j+=buf[i];
	j+=len;
	j&= 0x007F;
	return (char *) j;
}

void *sendrussPolling(unsigned char zone) {
	syslog(LOG_DEBUG, "polling zone %d",zone);
	struct sockaddr_in si_other;
	int s, slen=sizeof(si_other);
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
		syslog(LOG_WARNING,"socket failed!");
	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(russport);
	if (inet_aton(russipaddr, &si_other.sin_addr)==0) {
		syslog(LOG_WARNING, "inet_aton() for %s failed", russipaddr);
		daemonShutdown();
	}

	char buf_onvol[25] = { 0xF0, 0, 0, 0x7F, 0, 0, RUSS_KEYPAD_ID, 0x01, 0x05, 0x02, 0, 0, 0, 0x04, 0, 0, 0, 0xF7 };
	buf_onvol[1] = zone/ZONES_PER_CONTROLLER;
	buf_onvol[11] = zone%ZONES_PER_CONTROLLER;
	buf_onvol[16] = (int) russChecksum (buf_onvol,18-2);
	if (sendto(s, buf_onvol, 18, 0, (struct sockaddr *) &si_other, slen)==-1)
		syslog(LOG_WARNING,"sendto failed!");
	usleep(20*1000); //throttle a little (20ms)

	char buf_getzone[25] = { 0xF0, 0, 0, 0x7F, 0, 0, RUSS_KEYPAD_ID, 0x01, 0x04, 0x02, 0, 0, 0x07, 0, 0, 0, 0xF7 };
	buf_getzone[1] = zone/ZONES_PER_CONTROLLER;
	buf_getzone[11] = zone%ZONES_PER_CONTROLLER;
	buf_getzone[15] = (int) russChecksum (buf_onvol,17-2);
	if (sendto(s, buf_getzone, 17, 0, (struct sockaddr *) &si_other, slen)==-1)
		syslog(LOG_WARNING,"sendto failed!");

	usleep(50*1000); //throttle a little (20ms)
	close(s);
	return 0;
}

void *sendrussFunc(int controller, int zone, int func, int val) {
	struct sockaddr_in si_other;
	int s, slen=sizeof(si_other);
	syslog(LOG_DEBUG,"write ctrl %d zone %d func %d val 0x%02X",controller,zone,func,val);
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
		syslog(LOG_WARNING,"sendrussfunc: socket failed!");
	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(russport);
	if (inet_aton(russipaddr, &si_other.sin_addr)==0) {
		syslog(LOG_WARNING, "sendrussfunc: inet_aton() for %s failed", russipaddr);
		daemonShutdown();
	}

	//TODO: send on-telegram to actuator
	char buf_msg1[25] = { 0, 0, 0, 0x7F, 0, 0, RUSS_KEYPAD_ID, 0x05, 0x02, 0x02, 0, 0, 0xF1, 0x23, 0, 0, 0, 0, 0, 0x01, 0, 0xF7 };
	buf_msg1[1] = controller;
	buf_msg1[17] = zone;
	char buf_msg2[25] = { 0, 0, 0, 0x7F, 0, 0, RUSS_KEYPAD_ID, 0, 0x05, 0x02, 0, 0, 0, 0, 0, 0, 0, 0x01, 0, 0x01, 0, 0, 0, 0xF7 };
	buf_msg2[1] = controller;
	buf_msg2[11] = zone;

	switch (func) {
		case -9: //all zones
			buf_msg1[0] = 0xF0;
			buf_msg1[1] = 0x7E;
			buf_msg1[13] = 0x22;
			buf_msg1[16] = val;
			break;
		case 0: //power
			buf_msg1[0] = 0xF0;
			buf_msg1[15] = val;
			break;
		case 1: //src
			buf_msg1[0] = 0xF0;
			buf_msg1[5] = zone;
			buf_msg1[9] = 0;
			buf_msg1[17] = val;
			buf_msg1[13] = 0x3E;
			break;
		case 2: //volume
			buf_msg1[0] = 0xF0;
			buf_msg1[13] = 0x21;
			buf_msg1[15] = val*100/255/2;
			break;
		case 3: //bass
			buf_msg2[0] = 0xF0;
			buf_msg2[21] = val;
			break;
		case 4: //treb
			buf_msg2[0] = 0xF0;
			buf_msg2[13] = 0x01;
			buf_msg2[21] = val;
			break;
		case 5: //loud
			buf_msg2[0] = 0xF0;
			buf_msg2[13] = 0x02;
			buf_msg2[21] = val;
			break;
		case 6: //bal
			buf_msg2[0] = 0xF0;
			buf_msg2[13] = 0x03;
			buf_msg2[21] = val;
			break;
		case 7: //party
			buf_msg2[0] = 0xF0;
			buf_msg2[13] = 0x07;
			buf_msg2[21] = val;
			break;
		case 8: //dnd
			buf_msg2[0] = 0xF0;
			buf_msg2[13] = 0x06;
			buf_msg2[21] = val;
			break;
		case 9: //turnonvol
			buf_msg2[0] = 0xF0;
			buf_msg2[13] = 0x04;
			buf_msg2[21] = val*100/255/2;
			break;
		//TODO: 10 src cmd and 11 keypadcmd
		case 12: //volume relative up/down
			buf_msg1[0] = 0xF0;
			buf_msg1[5] = zone;
			if (val) {
				buf_msg1[12] = 0x7F;
				buf_msg1[13] = 0;
				buf_msg1[17] = 0;
				buf_msg1[18] = 0x01;
				buf_msg1[20] = 0xF7;
				buf_msg1[21] = 0; //clear
			} else {
				buf_msg1[12] = 0xF1;
				buf_msg1[13] = 0x7F; //clear
			}
			buf_msg1[15] = val;
		break;
	}			

	if (buf_msg1[0] && buf_msg1[21])
		buf_msg1[20] = (int) russChecksum (buf_msg1,22-2);
	else if (buf_msg1[0] && buf_msg1[20]) //keypadcmd without F1
		buf_msg1[19] = (int) russChecksum (buf_msg1,21-2);
	if (buf_msg2[0])
		buf_msg2[22] = (int) russChecksum (buf_msg2,24-2);

	if (buf_msg1[0])
		if (sendto(s, buf_msg1, 22, 0, (struct sockaddr *) &si_other, slen)==-1)
			syslog(LOG_WARNING,"sendrussfunc sendto failed!");
	if (buf_msg2[0])
		if (sendto(s, buf_msg2, 24, 0, (struct sockaddr *) &si_other, slen)==-1)
			syslog(LOG_WARNING,"sendrussfunc sendto failed!");
	usleep(20*1000); //throttle a little (20ms)
	if (buf_msg1[0] || buf_msg2[0])
		sendrussPolling (zone+(controller*ZONES_PER_CONTROLLER)); //fire update of states
	close(s);
	return 0;
}

void *sendKNXdgram(int type, int dpt, eibaddr_t dest, unsigned char val) {
	int len = 0;
	EIBConnection *con;
	unsigned char buf[255] = { 0, 0x80 };
	buf[1] = type; //0x40 response, 0x80 write
	syslog(LOG_DEBUG,"send knx dgram t %d d %d dest %d val %d",type,dpt,dest,val);
	con = EIBSocketURL (eibd_url);
	if (!con)
		syslog(LOG_WARNING,"sendknxdgram: Open failed");
	if (EIBOpenT_Group (con, dest, 1) == -1)
		syslog(LOG_WARNING,"sendknxdgram: Connect failed");
	switch (dpt)
	{
	case 0:
	  len=1;
	  //for(i = 0; i < strlen(val)/2; i++)
	  //raw value: nothing for now
	  break;
	case 1:
	  buf[1] |= val & 0x3f;
	  len=2;
	  break;
	case 3:
	  // EIS2/DPT3 4bit dim
	  buf[1] |= val & 0x3f;
	  len=2;
	  break;
	case 5:
	  buf[2] = val*255/100;
	  len=3;
	  break;
	case 51:
	case 5001:
	  buf[2] = val;
	  len=3;
	  break;
/*
	case 9:
	  fval=atof(data);
	  if (fval<0)
		sign = 0x8000;
	  mant = (int)(fval * 100.0);
	  while (abs(mant) > 2047) {
		  mant = mant >> 1;
		  exp++;
	  }
	  i = sign | (exp << 11) | (mant & 0x07ff);
	  buf[2] = i >> 8;
	  buf[3] = i & 0xff;
	  //return $data >> 8, $data & 0xff;
	  len=4;
	  break;
	case 16:
	  len=2;
	  for (i=0;i<strlen(data); i++)
	  {
		buf[i+2] = (int)(data[i]);
		len++;
	  }
*/	
	}

	len = EIBSendAPDU (con, len, buf);
	if (len == -1)
		syslog (LOG_WARNING,"sendknxdram: Request failed");
	EIBClose (con);
	usleep(50*1000); //throttle a little (50ms/max 20tps)
	return 0;
}


void *sendKNXresponse(eibaddr_t dest, int zone, int func) {
	syslog(LOG_DEBUG, "KNX response %d zone %d func %d", dest,zone,func);
	pthread_mutex_lock (&zonelock);
	switch (func) {
		case 0:
		case 20:
			sendKNXdgram (0x40,1,dest,zones[zone].zonepower);
			break;
		case 1:
		case 21:
			sendKNXdgram (0x40,51,dest,zones[zone].srcid);
			break;
		case 2:
		case 22:
			sendKNXdgram (0x40,5,dest,zones[zone].volume*2);
			break;
		case 3:
		case 23:
			sendKNXdgram (0x40,51,dest,zones[zone].bass);
			break;
		case 4:
		case 24:
			sendKNXdgram (0x40,51,dest,zones[zone].treble);
			break;
		case 5:
		case 25:
			sendKNXdgram (0x40,1,dest,zones[zone].loudness);
			break;
		case 6:
		case 26:
			sendKNXdgram (0x40,51,dest,zones[zone].balance);
			break;
		case 27:
			sendKNXdgram (0x40,1,dest,zones[zone].partymode);
			break;
		case 28:
			sendKNXdgram (0x40,1,dest,zones[zone].dnd);
			break;
		case 29:
			sendKNXdgram (0x40,5,dest,zones[zone].onvolume*2);
			break;
	}
	pthread_mutex_unlock (&zonelock);
	return 0;
}

void *handleKNXdgram(eibaddr_t dest, unsigned char* buf, int len){
	unsigned char val;
	int func,zone,controller;
	switch (buf[1] & 0xC0) {
	case 0x00:
		func = (dest - knxstartaddress)%256;
		zone = (func-10)/40;
		func = (func-10)%40;
		controller = (dest - knxstartaddress)/256;
		sendKNXresponse (dest,zone+(controller*ZONES_PER_CONTROLLER),func);
		break;
	case 0x40:
		//FIXME: response dunno
		break;
	case 0x80:
		if (buf[1] & 0xC0) {
			if (len == 2)
				val = buf[1] & 0x3F;
			else
				val = buf[2];
			func = (dest - knxstartaddress)%256;
			zone = (func-10)/40;
			func = (func-10)%40;
			controller = (dest - knxstartaddress)/256;
			sendrussFunc(controller,zone,func,val);
		}
		break;
	}
	return 0;
}

void *knxhandler()
{
	syslog(LOG_DEBUG, "KNX reader thread started");
	int len;
	EIBConnection *con;
	eibaddr_t dest;
	eibaddr_t src;
	unsigned char buf[255];
	
	while (1) //retry infinite
	{
		con = EIBSocketURL (eibd_url);
		if (!con) {
			syslog(LOG_WARNING, "eibd: Open failed");
			sleep(RETRY_TIME);
			continue;
		}

		if (EIBOpen_GroupSocket (con, 0) == -1) {
			syslog(LOG_WARNING, "eibd: Connect failed");
			sleep(RETRY_TIME);
			continue;
		}

		while (1)
		{
			len = EIBGetGroup_Src (con, sizeof (buf), buf, &src, &dest);
			if (len == -1) {
				syslog(LOG_WARNING, "eibd: Read failed");
				sleep(RETRY_TIME);
				break;
			}
			if (len < 2) {
				syslog(LOG_WARNING, "eibd: Invalid Packet");
				break;
			}
			if (buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0) {
				syslog(LOG_WARNING, "eibd: Unknown APDU from %d to %d",src,dest);
				break;
			} else {
				if ( dest<knxstartaddress || dest > (knxstartaddress+(numzones/ZONES_PER_CONTROLLER*256)-1)  ) //not for us
					continue;
				handleKNXdgram(dest,buf,len);
			}
		}
		syslog(LOG_WARNING,"eibd: closed connection"); //break in read-loop
		EIBClose (con);
	}
	pthread_exit(NULL);
}



void *updateZone(unsigned char num, unsigned char val, int func) {
	syslog(LOG_DEBUG, "update Zone %d val %d func %d",num,val,func);
	int controller = num/ZONES_PER_CONTROLLER;
	int zone = num%ZONES_PER_CONTROLLER;
	//TODO IMPORTANT!: lock mutex befor updating
	pthread_mutex_lock (&zonelock);
	switch (func) {
			case 1:
				zones[num].zonepower =val;
				if (zones[num].inited)
					sendKNXdgram (0x80,1,(knxstartaddress+30)+(zone*40)+(controller*256),val);
				break;
			case 2:
				zones[num].srcid =val;
				if (zones[num].inited)
					sendKNXdgram (0x80,51,(knxstartaddress+31)+(zone*40)+(controller*256),val);
				break;
			case 3:
				zones[num].volume =val;
				if (zones[num].inited)
					sendKNXdgram (0x80,5,(knxstartaddress+32)+(zone*40)+(controller*256),val*2);
				break;
			case 4:
				zones[num].bass =val;
				if (zones[num].inited)
					sendKNXdgram (0x80,51,(knxstartaddress+33)+(zone*40)+(controller*256),val);
				break;
			case 5:
				zones[num].treble =val;
				if (zones[num].inited)
					sendKNXdgram (0x80,51,(knxstartaddress+34)+(zone*40)+(controller*256),val);
				break;
			case 6:
				zones[num].loudness =val;
				if (zones[num].inited)
					sendKNXdgram (0x80,1,(knxstartaddress+35)+(zone*40)+(controller*256),val);
				break;
			case 7:
				zones[num].balance =val;
				if (zones[num].inited)
					sendKNXdgram (0x80,51,(knxstartaddress+36)+(zone*40)+(controller*256),val);
				break;
			case 8:
				zones[num].partymode =val;
				if (zones[num].inited)
					sendKNXdgram (0x80,1,(knxstartaddress+37)+(zone*40)+(controller*256),val);
				break;
			case 9:
				zones[num].dnd =val;
				if (zones[num].inited)
					sendKNXdgram (0x80,1,(knxstartaddress+38)+(zone*40)+(controller*256),val);
				break;
			case 10:
				zones[num].onvolume =val;
				if (zones[num].inited)
					sendKNXdgram (0x80,5,(knxstartaddress+39)+(zone*40)+(controller*256),val*2);
				break;
			default:
				break;
		}
	pthread_mutex_unlock (&zonelock);
	return 0;
}

void *russhandler()
{
	//FIXME: also handle serial-port directly?
	struct sockaddr_in si_me, si_other;
	int s, i;
	socklen_t slen=sizeof(si_other);
	unsigned char buf[BUFLEN];
	syslog(LOG_DEBUG, "Russound reader thread started");
	while (1) {
		if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
			syslog(LOG_WARNING, "russ: Socket failed");
			sleep(RETRY_TIME);
			continue;
		}
		memset(&si_me, 0, sizeof(si_me));
		si_me.sin_family = AF_INET;
		si_me.sin_port = htons(listenport);
		si_me.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(s, (struct sockaddr *) &si_me, sizeof(si_me))==-1) {
			syslog(LOG_WARNING, "russ: Bind failed");
			sleep(RETRY_TIME);
			continue;
		}
		while (1) {
			int len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen);
			//TODO: Checksum calculation / check of russound-message
			if (len==-1) {
				syslog(LOG_WARNING, "russ: recvfrom failed");
				sleep(RETRY_TIME);
				break;
			} else if ((len==34) && (buf[0]==0xF0) && (buf[9]==0x04)) { //zone-status
//				syslog(LOG_DEBUG,"russ Controller:%d Zone:%d Status:%d src:%d vol:%d bass:%d treb:%d loud:%d bal:%d sys:%d shrsrc:%d party:%d,DnD:%d\n",
//				       buf[4],buf[12],buf[20],buf[21],buf[22],buf[23],buf[24],buf[25],buf[26],buf[27],buf[28],buf[29],buf[30]);
				buf[12] = (buf[4]*ZONES_PER_CONTROLLER)+buf[12]; //controller + zonenumber
				if (buf[20] != zones[buf[12]].zonepower)
					updateZone(buf[12],buf[20],1);
				if (buf[21] != zones[buf[12]].srcid)
					updateZone(buf[12],buf[21],2);
				if (buf[22] != zones[buf[12]].volume)
					updateZone(buf[12],buf[22],3);
				if (buf[23] != zones[buf[12]].bass)
					updateZone(buf[12],buf[23],4);
				if (buf[24] != zones[buf[12]].treble)
					updateZone(buf[12],buf[24],5);
				if (buf[25] != zones[buf[12]].loudness)
					updateZone(buf[12],buf[25],6);
				if (buf[26] != zones[buf[12]].balance)
					updateZone(buf[12],buf[26],7);
				if (buf[29] != zones[buf[12]].partymode)
					updateZone(buf[12],buf[29],8);
				if (buf[30] != zones[buf[12]].dnd)
					updateZone(buf[12],buf[30],9);
				zones[buf[12]].inited = 1;
			} else if ((len==24) && (buf[0]==0xF0) && (buf[9]==0x05) && (buf[13]==0x00)) { //zone turn-on volume
				//FIXME: this *might* be wrong andf trigger also on other msgs, as it's written otherwise in the docs, the checked bytes are just a guess!
//				syslog(LOG_DEBUG,"russ Controller:%d Zone:%d TurnOnVolume:%d",
//				       buf[4],buf[12],buf[21]);
				buf[12] = (buf[4]*ZONES_PER_CONTROLLER)+buf[12]; //controller + zonenumber
				if (buf[21] != zones[buf[12]].onvolume)
					updateZone(buf[12],buf[21],10);
			} else {
				//FIXME: just for debugging
				for (i=0; i<len; i++)
					printf("%d:0x%02X ",i,buf[i]);
				printf(" unknown len: %d\n",len);
				for (i=0; i<len; i++)
					printf("0x%02X ",buf[i]);
				printf("\n");
			}
		}
		syslog(LOG_WARNING,"russ: closed socket"); //break in read-loop
		close(s);		
	}
	pthread_exit(NULL);
}

eibaddr_t readgaddr (const char *addr) {
	int a, b, c;
	if (sscanf (addr, "%d/%d/%d", &a, &b, &c) == 3)
		return ((a & 0x01f) << 11) | ((b & 0x07) << 8) | ((c & 0xff));
	if (sscanf (addr, "%d/%d", &a, &b) == 2)
	    return ((a & 0x01f) << 11) | ((b & 0x7FF));
	if (sscanf (addr, "%x", &a) == 1)
		return a & 0xffff;
	syslog(LOG_WARNING,"invalid group address format %s", addr);
	daemonShutdown();
	return 0;
}

int main(int argc, char **argv) {
	int daemonize = 0;
    int c;
	char *p;
	char pidstr[255];
	
	while ((c = getopt (argc, argv, "dp:i:l:a:z:u:")) != -1)
		switch (c) {
			case 'd':
				daemonize = 1;
				break;
			case 'p':
				pidfilename = optarg;
				break;
			case 'i':
				p = strtok(optarg,":");
				russipaddr = p;
				p = strtok (NULL, ":");
				russport = atoi(p);
				break;
			case 'l':
				listenport = atoi(optarg);
				break;
			case 'a':
				knxstartaddress = readgaddr(optarg);
				break;
			case 'z':
				numzones = atoi(optarg);
				break;
			case 'u':
				eibd_url = optarg;
				break;
			case '?':
				//FIXME: check arguments better, print_usage
				fprintf (stderr, "Unknown option `-%c'.\nUsage: %s %s", optopt, argv[0], USAGESTRING);
				return 1;
			default:
			 abort ();
		}

	//FIXME: clean shutdown in sub-thread with signals?
	signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
 
    if (!daemonize) {
	    setlogmask(LOG_UPTO(LOG_DEBUG));
		openlog(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);
        syslog(LOG_DEBUG, "startup with debug; Russ-IP: %s:%d, listenport %d, pidfile: %s, start address: %d, number of zones: %d, eibd: %s", 
               russipaddr, russport, listenport, pidfilename, knxstartaddress, numzones, eibd_url);
	} else {
	    setlogmask(LOG_UPTO(LOG_INFO));
		openlog(DAEMON_NAME, LOG_CONS, LOG_USER);
	}

	pid_t pid, sid;
 
    if (daemonize) {
        syslog(LOG_INFO, "starting daemon");
 
        pid = fork();
        if (pid < 0) {
            exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }
        umask(0);
        sid = setsid();
        if (sid < 0) {
            exit(EXIT_FAILURE);
        }
        if ((chdir("/")) < 0) {
            exit(EXIT_FAILURE);
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }
	//FIXME: output errors to stderr, change order
	pidFilehandle = open(pidfilename, O_RDWR|O_CREAT, 0600);
	if (pidFilehandle == -1 )
	{
		syslog(LOG_INFO, "Could not open pidfile %s, exiting", pidfilename);
	    fprintf(stderr, "Could not open pidfile %s, exiting", pidfilename);	
		exit(EXIT_FAILURE);
	}
	if (lockf(pidFilehandle,F_TLOCK,0) == -1)
	{
		syslog(LOG_INFO, "Could not lock pidfile %s, exiting", pidfilename);
	    fprintf(stderr, "Could not lock pidfile %s, exiting", pidfilename);	
		exit(EXIT_FAILURE);
	}
	sprintf(pidstr,"%d\n",getpid());
	c = write(pidFilehandle, pidstr, strlen(pidstr));

	int knxthread,russthread;
	pthread_t threads[NUM_THREADS];
	// PTHREAD_CREATE_DETACHED?
	knxthread = pthread_create(&threads[1], NULL, knxhandler, NULL); //id, thread attributes, subroutine, arguments
	russthread = pthread_create(&threads[2], NULL, russhandler, NULL); //id, thread attributes, subroutine, arguments
	//TODO: Maybe another console/TCP-server/Logging thread?
	while (1) {
		syslog(LOG_DEBUG, "%s daemon running", DAEMON_NAME);
		for (c=0;c<numzones;c++)
			sendrussPolling(c);
		//TODO: send off-telegram to actuator
		sleep(POLLING_INTERVAL);
	}
	syslog(LOG_INFO, "%s daemon exiting", DAEMON_NAME);
     // TODO: Free any allocated resources before exiting - we never get here though -> signal handler
    exit(EXIT_SUCCESS);
}
