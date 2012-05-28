/*
 * log.cc
 *
 * by eater (2010)
 * modfied by JNK 2012
 *
 * copied from stackoverflow.com/questions/2638654/redirect-c-stdclog-to-syslog-on-unix
 *
 * licensed under creative commons
 *
*/

#include "log.h"

Log::Log(std::string ident, int option, int facility) {
    facility_ = facility;
    option_ = option;
    priority_ = LOG_DEBUG;
    strncpy(ident_, ident.c_str(), sizeof(ident_));
    ident_[sizeof(ident_)-1] = '\0';

    openlog(ident_, option_, facility_);
}

int Log::sync() {
    if (buffer_.length()) {
        syslog(priority_, "%s", (char *)buffer_.c_str());
        buffer_.erase();
        priority_ = LOG_DEBUG; // default to debug for each message
    }
    return 0;
}

int Log::overflow(int c) {
    if (c != EOF) {
        buffer_ += static_cast<char>(c);
    } else {
        sync();
    }
    return c;
}

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority) {
    static_cast<Log *>(os.rdbuf())->priority_ = (int)log_priority;
    return os;
}

