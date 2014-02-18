# -------------------------------------------------
# Project created by QtCreator 2010-01-14T21:01:53
# -------------------------------------------------
QT += network \
#    sql \
    xml
TARGET = qHSMon
TEMPLATE = app
SOURCES += main.cpp \
    window.cpp
HEADERS += window.h
RESOURCES += res.qrc

# manual rc-file for ico
RC_FILE = qHSMon.rc
OTHER_FILES += qHSMon.rc \
    LICENSE.GPL \
    README.txt
FORMS += 
win32:CONFIG += static
release:CONFIG:# doubled to suppress debug-output even if a console exists in release!
    DEFINES += QT_NO_DEBUG_OUTPUT

# remove mingw/libgcc-depends
win32:CONFIG -= exceptions
win32:QMAKE_LFLAGS+=-static-libgcc
