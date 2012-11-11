/*
 * Author:  Jax Roland
 * Date:    09.11.2012
 * License: GPLv3
 */

#ifndef SERVER_H_
#define SERVER_H_


// includes
#include "ebus.h"
#include "vaillant.h"


// serial device definitions
#define SERIAL_DEFAULT_DEVICE "/dev/ttyUSB0"
#define SERIAL_BAUDRATE       B2400


// ouput file definitions
#define OUTPUT_DEFAULT_FILE "/tmp/sdump.dat"


// Logging Macros
#define log_info(M, ...) fprintf(stderr, "[I] " M, ##__VA_ARGS__)
#define log_error(M, ...) fprintf(stderr, "[E] " M, ##__VA_ARGS__)
#define log_debug(M, ...) fprintf(stderr, "[D] " M, ##__VA_ARGS__)
#define log_verbose(M, ...) fprintf(stderr, "[V] " M, ##__VA_ARGS__)


#endif /* SERVER_H_ */
