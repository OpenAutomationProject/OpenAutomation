#!/bin/bash
###############################################################################
# Script to compile and install eibd on a debian wheezy (7) based systems
# 
# Michael Albert info@michlstechblog.info
# 2014/02/19
# Changes:
# Version 1.1 Michael Albert Line 40 use $BUSSDK_SOURCES instead of the hardcoded URL
#                            Line 142 Modified on altered parameters in console.txt. Splited in two sperated sed's
#                            Thankx to thomas.dallmair@gmx.de
# Version 1.1.1 Michael Albert fixed missing - at line 144
# License: GPLv2
###############################################################################
if [ "$(id -u)" != "0" ]; then
   echo "     Attention!!!"
   echo "     Start script must run as root" 1>&2
   echo "     Start a root shell with"
   echo "     sudo su -"
   exit 1
fi
# define environment
export BUILD_PATH=~/eibdbuild
export PTHSEM_PATH=${BUILD_PATH}/pthsem
export BUSSDK_PATH=${BUILD_PATH}/bussdk
export INSTALL_PREFIX=/usr/local
# Sources
export PTHSEM_SOURCES=http://www.auto.tuwien.ac.at/~mkoegler/pth/pthsem_2.0.8.tar.gz
export BUSSDK_SOURCES=http://netcologne.dl.sourceforge.net/project/bcusdk/bcusdk/bcusdk_0.0.5.tar.gz
# create folders
mkdir -p $PTHSEM_PATH
mkdir -p $BUSSDK_PATH
cd $PTHSEM_PATH
# PTHSEM
wget $PTHSEM_SOURCES
tar -xvzf pthsem_2.0.8.tar.gz
cd pthsem-2.0.8
./configure --enable-static=yes --prefix=$INSTALL_PREFIX CFLAGS="-static -static-libgcc -static-libstdc++" LDFLAGS="-static -static-libgcc -static-libstdc++" 
make && make install
# Add pthsem library to libpath
export LD_LIBRARY_PATH=$INSTALL_PREFIX/lib:$LD_LIBRARY_PATH
# BUSSDK
cd $BUSSDK_PATH
wget $BUSSDK_SOURCES
tar -xvzf bcusdk_0.0.5.tar.gz
cd bcusdk-0.0.5
# Specify here which features eibd should support
# –enable-ft12                 enable FT1.2 backend
# –enable-pei16                enable BCU1 kernel driver backend
# –enable-tpuart               enable TPUART kernel driver backend (deprecated)
# –enable-pei16s               enable BCU1 user driver backend (very experimental)
# –enable-tpuarts              enable TPUART user driver backend
# –enable-eibnetip             enable EIBnet/IP routing backend
# –enable-eibnetiptunnel       enable EIBnet/IP tunneling backend
# –enable-usb                  enable USB backend
# –enable-eibnetipserver       enable EIBnet/IP server frontend
# –enable-groupcache           enable Group Cache (default: yes)
# –enable-java                 build java client library
# ./configure --enable-onlyeibd --enable-tpuarts --enable-tpuart --enable-ft12 --enable-eibnetip --enable-eibnetiptunnel --enable-eibnetipserver --enable-groupcache --enable-static=yes  --prefix=$INSTALL_PREFIX --with-pth=$INSTALL_PREFIX CFLAGS="-static -static-libgcc -static-libstdc++" LDFLAGS="-static -static-libgcc -static-libstdc++ -s" CPPFLAGS="-static -static-libgcc -static-libstdc++"
./configure \
    --enable-onlyeibd \
    --enable-tpuarts \
    --enable-tpuart \
    --enable-ft12 \
    --enable-eibnetip \
    --enable-eibnetiptunnel \
    --enable-eibnetipserver \
    --enable-groupcache \
    --enable-static=yes --prefix=$INSTALL_PREFIX --with-pth=$INSTALL_PREFIX CFLAGS="-static -static-libgcc -static-libstdc++" LDFLAGS="-static -static-libgcc -static-libstdc++ -s" CPPFLAGS="-static -static-libgcc -static-libstdc++"
make && make install
# clean up 
rm -r $BUILD_PATH
# New user for running eibd, Grop member of dailot for permissions on device /dev/ttyAMA0
useradd eibd -s /bin/false -U -M -G dialout
# init script
cat > /etc/init.d/eibd <<EOF
#! /bin/sh
### BEGIN INIT INFO
# Provides:             eibd
# Required-Start:       \$remote_fs \$syslog
# Required-Stop:        \$remote_fs \$syslog
# Default-Start:        2 3 4 5
# Default-Stop:         0 1 6
# Short-Description:    KNX/EIB eibd server
### END INIT INFO
set -e
export EIBD_BIN=/usr/local/bin/eibd
export EIBD_OPTIONS="-d -D -T -R -S -i -u --eibaddr=1.1.128 tpuarts:/dev/ttyAMA0"
#export EIBD_OPTIONS="-d -D -T -R -S -i -u --eibaddr=1.1.128 ipt:192.168.56.1"
export EIBD_USER=eibd
test -x \$EIBD_BIN || exit 0
umask 022
. /lib/lsb/init-functions
# Are we running from init?
run_by_init() {
    ([ "\$previous" ] && [ "\$runlevel" ]) || [ "\$runlevel" = S ]
}
export PATH="/usr/local/bin:\${PATH}"
case "\$1" in
  start)
        log_daemon_msg "Starting eibd daemon" "eibd" || true
        route add 224.0.23.12 dev eth0 > /dev/null 2>&1 || true
        if start-stop-daemon --start --quiet --oknodo -c \$EIBD_USER --exec \$EIBD_BIN -- \$EIBD_OPTIONS; then
            log_end_msg 0 || true
        else
            log_end_msg 1 || true
        fi
        ;;
  stop)
        log_daemon_msg "Stopping eibd daemon" "eibd" || true
        route delete 224.0.23.12 > /dev/null 2>&1 || true
        if start-stop-daemon --stop --quiet --oknodo --exec \$EIBD_BIN; then
            log_end_msg 0 || true
        else
            log_end_msg 1 || true
        fi
        ;;

  restart)
        log_daemon_msg "Restarting eibd daemon" "eibd" || true
        start-stop-daemon --stop --quiet --oknodo --retry 30 --exec \$EIBD_BIN
        if start-stop-daemon --start --quiet --oknodo -c \$EIBD_USER --exec \$EIBD_BIN -- \$EIBD_OPTIONS; then
            log_end_msg 0 || true
        else
            log_end_msg 1 || true
        fi
        ;;

  status)
        status_of_proc \$EIBD_BIN eibd && exit 0 || exit \$?
        ;;

  *)
        log_action_msg "Usage: /etc/init.d/eibd {start|stop|restart|status}" || true
        exit 1
esac
exit 0

EOF
# Set permissions on init script
chmod 755 /etc/init.d/eibd
sync
# Modify /boot/cmdline.txt to disable boot screen over serial interface
# sed -e's/ console=ttyAMA0,115200 kgdboc=ttyAMA0,115200//g' /boot/cmdline.txt --in-place=.bak
sed -e's/ console=ttyAMA0,115200//g' /boot/cmdline.txt -–in-place=.bak
sed -e's/ kgdboc=ttyAMA0,115200//g' /boot/cmdline.txt --in-place=.bak
# Disable serial console
sed -e 's/\(T0:23:respawn:\/sbin\/getty -L ttyAMA0 115200 vt100\)/# \1/g' /etc/inittab --in-place=.bak
# activate init script
# update-rc.d eibd defaults
echo Please reboot your device...



