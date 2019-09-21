Howto build:

- get Qt (tested with 4.x under Windows)
- run qmake & make
(or open qHSmon.pro in QT Creator und build it from there)

Under Linux (Debian/Ubuntu):
apt-get install qtcreator
(this implies all required Qt-libs)

There are statically linked binaries für Windows & OS/X in the downloads,
supporting thread http://knx-user-forum.de/knx-eib-forum/7449-hsmonitor-aktualisierung.html

Update 2019-09-21: 
Still works fine with Ubuntu 19.x and QT4, just do
sudo apt-get install libqt4-dev
qmake-qt4 
make
./qHSMon
