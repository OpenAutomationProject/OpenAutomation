/*
 * log.h
 *
 * by eater (2010)
 * modfied by JNK 2012
 *
 * copied from stackoverflow.com/questions/2638654/redirect-c-stdclog-to-syslog-on-unix
 *
 * licensed under creative commons
 *
*/

#ifndef LOG_H
#define LOG_H

#include <syslog.h>
#include <iostream>
#include <string.h>
#include <fstream>

enum LogPriority {
    kLogEmerg   = LOG_EMERG,   // system is unusable
    kLogAlert   = LOG_ALERT,   // action must be taken immediately
    kLogCrit    = LOG_CRIT,    // critical conditions
    kLogErr     = LOG_ERR,     // error conditions
    kLogWarning = LOG_WARNING, // warning conditions
    kLogNotice  = LOG_NOTICE,  // normal, but significant, condition
    kLogInfo    = LOG_INFO,    // informational message
    kLogDebug   = LOG_DEBUG    // debug-level message
};

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority);

class Log : public std::basic_streambuf<char, std::char_traits<char> > {
public:
    explicit Log(std::string ident, int option, int facility);

protected:
    int sync();
    int overflow(int c);

private:
    friend std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority);
    std::string buffer_;
    int facility_;
    int priority_;
    int option_;
    char ident_[50];
};

#endif
