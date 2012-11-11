/*
 * Author:  Jax Roland
 * Date:    09.11.2012
 * License: GPLv3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#include "server.h"

const char g_cProgramName[] = "server";

static char g_arrcOptions[] = "dhi:o:s:vw";

static char g_arrcHelpMsg[] =
"\nUsage: sdump [dhiosvw]\n"
"\n"
"-d\t\tprint decoded ebus message\n"
"-h\t\tprint help\n"
"-i filename\tread from input file\n"
"-o filename\toutput file (needs -w | default: /tmp/sdump.bin)\n"
"-s device\tread from serial device (default: /dev/ttyUSB0)\n"
"-v\t\tbe verbose (-vv for very verbose)\n"
"-w\t\twrite into output file\n"
;

// options
int g_iVerbose = 0;                                    // show detailed information

int g_iFlagDecodeMsg = 0;                              // flag decode ebus message
static int g_iFlagInputSerial = 1;                     // flag read data from serial device
static int g_iFlagInputFile = 0;                       // flag read data from input file
static int g_iFlagOutputFile = 0;                      // flag write data into output file

const char *g_pcInputFile = NULL;                      // file for input data
const char *g_pcOutputFile = OUTPUT_DEFAULT_FILE;      // file for output data
const char *g_pcSerialDevice = SERIAL_DEFAULT_DEVICE;  // name of serial device


// get delta time between 2 messages
int get_delta_time(const struct timeval *ref_stop, const struct timeval *ref_start, time_t *res_delta_sec, suseconds_t *res_delta_usec)
{


	if (!ref_stop || !ref_start) {
		fprintf(stderr, "%s: Error: NULL timeval struct provided as argument\n", __func__);
		exit(1);
	}
	if (!res_delta_sec) {
		fprintf(stderr, "%s: Warning: NULL res_delta_sec pointer\n", __func__);
	}
	/* Note: we allow a NULL res_delta_usec... this simply means that the caller does not care about a µsec precision */
	if (ref_stop->tv_sec < ref_start->tv_sec) {
		fprintf(stderr, "%s: Error: time is going backwards ds<0!\n", __func__);
		return 1;
	}
	else {
		*res_delta_sec = ref_stop->tv_sec - ref_start->tv_sec;
		if (ref_stop->tv_usec < ref_start->tv_usec) {	/* µsec overflow, convert back one sec in µsec */
			if (*res_delta_sec>0) {
				(*res_delta_sec)--;
				if (res_delta_usec)	/* If the caller cares about usecs */
					*res_delta_usec = ref_stop->tv_usec + 1000000 - ref_start->tv_usec;	/* Add the second removed in the line above, and converts it as 10^6µs */
			}
			else {
				fprintf(stderr, "%s: Error: time is going backwards (ds=0, dus<0)!\n", __func__);
				return 1;
			}
		}
		else {
			if (res_delta_usec)	/* If the caller cares about µsecs */
				*res_delta_usec = ref_stop->tv_usec - ref_start->tv_usec;
		}
	}
	return 0;
}


// open serial device
int open_serial_device(const char *pcSerialDevice, int *piSerialFD, struct termios *pstOldTermios) {

	struct termios stNewTermios;

	// open the serial port
	if (!piSerialFD)
		return -1;
	else
		*piSerialFD = open(pcSerialDevice, O_RDWR | O_NOCTTY | O_NDELAY);

	if (*piSerialFD == -1)
		return -1;
	else
		fcntl(*piSerialFD, F_SETFL, 0);

	// get the current settings for the serial port
	if (pstOldTermios)
		tcgetattr(*piSerialFD, pstOldTermios);

	memset(&stNewTermios, '\0', sizeof(stNewTermios));

	// set raw input, 1 char minimum before read returns
	stNewTermios.c_cflag = SERIAL_BAUDRATE | CS8 | CLOCAL | CREAD;
	stNewTermios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	stNewTermios.c_iflag = IGNPAR;
	stNewTermios.c_oflag &= ~OPOST;
	stNewTermios.c_cc[VMIN]  = 1;
	stNewTermios.c_cc[VTIME] = 0;

	tcflush(*piSerialFD, TCIFLUSH);

	// reset the serial port options to what we have defined above
	tcsetattr(*piSerialFD, TCSANOW, &stNewTermios);

	return 0;
}


// close serial device
int close_serial_device(int *piSerialFD, struct termios *pstOldTermios) {

	// Reset serial device to default settings
	tcsetattr(*piSerialFD, TCSANOW, pstOldTermios);

	// Close file descriptor from serial device
	close(*piSerialFD);

	return 0;
}


// get size of input file
int input_file_size(FILE *pstrInputFile) {
	long lReadDataSize = 0;

	fseek(pstrInputFile, 0L, SEEK_END);
	lReadDataSize = ftell(pstrInputFile);
	rewind(pstrInputFile);

	return (int) lReadDataSize;
}


// read input file
void input_file_copy(FILE *pstrInputFile, unsigned char *pucSerialBuffer) {
	int ii = 0;
	int jj = 0;

	// copy input data
	while ((ii = fgetc(pstrInputFile)) != EOF) {
		*(pucSerialBuffer + jj) = (unsigned char)ii;
		if (g_iVerbose > 1)
			fprintf(stdout, "pos: %05d hex: %02x\n", jj, *(pucSerialBuffer + jj));
		jj++;
	}

	if (g_iVerbose > 1)
	log_info("%d Bytes read from \"%s\"\n", jj, g_pcInputFile);
}


// main
int main(int argc, char **argv)
{

	// serial device
	struct termios stOldTermios;                          // save serial device settings
	int iSerialFD = 0;                                    // serial device file descriptor

	// input data handling
	unsigned char arrucInputBuffer[BUFFER_SIZE];   // input data buffer
	unsigned char *pucInputBuffer = NULL;                 // input data buffer pointer
	int iInputDataSize = 0;                               // input data buffer size


	// file handling
	FILE *pstrInputFile = NULL;                           // file descriptor for input file
	FILE *pstrOutputFile = NULL;                          // file descriptor for output file


	// time
	struct timeval stTimeNow;                             // current time
//	struct timeval tlastdata; /* Time last data byte was received on serial line */
//	struct timeval stTimeLastSYN; /* Time last syn byte was received on serial line */
//	struct timeval stTimeLastIN; /* Time last byte (syn or data) was received on serial line */

//	time_t delta_tv_sec;    /* Delta between two timestamps in secs */
//	suseconds_t delta_tv_usec;    /* Delta between two timestamps (additionnal µsec precision) */


	// misc
	int iTerminate = 0;
	int iCount = 0;
	int ii = 0;
//	int jj = 0;

	// data handling
	unsigned char arrucDataBuffer[BUFFER_SIZE];
//	unsigned char *pucEbusBuffer = arrucEbusBuffer;

	//	struct MSG_HEAD_t stMsgHead;
//	struct MSG_t stMsg;
//	unsigned char *pucMsgData;

	// parse command line arguments
	while ((iCount = getopt(argc, argv, g_arrcOptions)) != -1) {
		switch (iCount) {
			case 'd':
				g_iFlagDecodeMsg = 1;
				break;
			case 'h':
				fprintf(stderr, g_arrcHelpMsg, "");
				exit(1);
			case 'i':
				g_iFlagInputSerial = 0;
				g_iFlagInputFile = 1;
				g_pcInputFile = optarg;
				break;
			case 'o':
				if (strncmp(optarg, "-", 1) != 0)
					g_pcOutputFile = optarg;
				else {
					fprintf(stderr, "filname \"%s\" for output data seems to be wrong\n", optarg);
					fprintf(stderr, g_arrcHelpMsg, "");
					exit(1);
				}
				break;
			case 's':
				g_iFlagInputFile = 0;
				g_iFlagInputSerial = 1;
				g_pcSerialDevice = optarg;
				break;
			case 'v':
				g_iVerbose++;
				break;
			case 'w':
				g_iFlagOutputFile = 1;
				break;
			case '?':
				fprintf(stderr, g_arrcHelpMsg, "");
				exit(1);
			default:
				break;
		}
	}

	for (iCount = optind; iCount < argc; iCount++)
		log_info("Non-option argument %s\n", argv[iCount]);


	// open serial device
	if (g_iFlagInputSerial) {
		log_info("Using serial device \"%s\" for input data\n", g_pcSerialDevice);

		if (open_serial_device(g_pcSerialDevice, &iSerialFD, &stOldTermios) != 0) {
			log_error("Could not open serial device %s\n", g_pcSerialDevice);
			exit(1);
		}
	}

	// open input file
	if (g_iFlagInputFile) {
		log_info("Using file \"%s\" for input data\n", g_pcInputFile);

		pstrInputFile = fopen(g_pcInputFile, "r");

		if (!pstrInputFile) {
			log_error("Could not open file \"%s\"\n", g_pcInputFile);
			exit(1);
		}
	}

	// open output file
	if (g_iFlagOutputFile) {
		log_info("Using file \"%s\" for output data\n", g_pcOutputFile);

		pstrOutputFile = fopen(g_pcOutputFile, "w");

		if (!pstrOutputFile) {
			log_error("Could not open file \"%s\"\n", g_pcOutputFile);
			exit(1);
		}
	}


	// get current time for last ebus data
//	gettimeofday(&tlastsyn, NULL);
//	tlastin=tlastsyn;

//	tlastdata.tv_sec=(time_t)0; tlastdata.tv_usec=(suseconds_t)0; 	/* Time for last received data byte is never */
//	in_ebus_msg = 0;

	// reset data buffer
	memset(arrucDataBuffer, '\0', sizeof(arrucDataBuffer));

	// main loop
	do {

		// read serial device
		if (g_iFlagInputSerial) {
			iInputDataSize = read(iSerialFD, arrucInputBuffer, BUFFER_SIZE);
			pucInputBuffer = arrucInputBuffer;
		}

		// read input file
		if (g_iFlagInputFile) {
			iInputDataSize = input_file_size(pstrInputFile);

			pucInputBuffer = malloc(iInputDataSize * sizeof(unsigned char));
			if (!pucInputBuffer) {
				log_error("Error during malloc for input data\n");
				exit(1);
			} else {
				input_file_copy(pstrInputFile, pucInputBuffer);
			}
		}

		// new data received
		if (iInputDataSize > 0) {

			// get current timestamp
			gettimeofday(&stTimeNow, NULL);

			// write data into output file
			if (g_iFlagOutputFile) {
				for (iCount = 0; iCount < iInputDataSize; iCount++) {
					if (g_iVerbose > 1)
						fprintf(stdout, "pos: %05d hex: %02x\n", iCount, pucInputBuffer[iCount]);
					if(fputc(pucInputBuffer[iCount], pstrOutputFile) == EOF)
						log_error("Error during writing data\n");
				}
				if (g_iVerbose > 1)
					log_info("%d Bytes written into \"%s\"\n", iInputDataSize, g_pcOutputFile);
				if(fflush(pstrOutputFile) == EOF)
					log_error("Error during flushing data\n");
			}

			// be verbose
			if (g_iVerbose > 1 && g_iFlagInputSerial) {
				fprintf(stdout, "\nReceived %d bytes at t=%d.%03d: ", iInputDataSize, (int)stTimeNow.tv_sec, (int)(stTimeNow.tv_usec/1000));
				for (iCount = 0; iCount < iInputDataSize; iCount++)
					fprintf(stdout, "%02x ", pucInputBuffer[iCount]);
			}


			// TODO check correct time for grouping bytes


			// 1. walk through input data
			// 2. collect this into temporary buffer
			// 3. decode message if SYN byte arrive

			iCount = 0;
			while (iCount < iInputDataSize) {
				// skip 0xaa entries
				if (pucInputBuffer[iCount] == EBUS_SYN) {

					if (ii > 0) {

						// decode ebus messages
						decode_ebus_msg(arrucDataBuffer, ii);

						memset(arrucDataBuffer, '\0', sizeof(arrucDataBuffer));
						ii = 0;
					}
				}
				// copy input data into buffer
				else {
					arrucDataBuffer[ii] = pucInputBuffer[iCount];
					ii++;
				}
				iCount++;
			}
		}

	} while (!iTerminate && !g_iFlagInputFile);

	fprintf(stdout, "\n\n");

	// close read file
	if (g_iFlagInputFile) {
		fclose(pstrInputFile);
		// free memory
		free(pucInputBuffer);
	}

	// close dump file
	if (g_iFlagOutputFile)
		fclose(pstrOutputFile);

	// close serial device
	if (g_iFlagInputSerial)
		close_serial_device(&iSerialFD, &stOldTermios);

	// close program
	exit(1);
}

