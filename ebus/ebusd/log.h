/*
 * Author:  Jax Roland
 * Date:    04.12.2012
 * License: GPLv3
 */

#ifndef LOG_H_
#define LOG_H_

/*
 * INF = 0 - normal messages
 * WAR = 1 - warnings
 * ERR = 2 - errors
 * DBG = 3 - for debugging purpose
 */

#define INF 0
#define WAR 1
#define ERR 2
#define DBG 3

void log_set_file(FILE *fp);
void log_set_level(int loglevel);
int log_open(const char *logfile, int foreground);
void log_close();
void log_print_msg(int loglevel, const char *logtxt, ...);

#endif /* LOG_H_ */
