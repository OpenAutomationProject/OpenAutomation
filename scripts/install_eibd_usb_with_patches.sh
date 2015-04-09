#!/bin/bash
###############################################################################
# Script to compile and install eibd on raspian based systems
# 
# Michael Albert info@michlstechblog.info
# 2014/02/19
# Changes:
# 20.05.2014 Michl enable usb backend support
#            Add User pi to eibd group	
#            udev rules for KNX USB Interfaces added
# 			 Special thanks to Bastian Herzog for testing and improving this script!
# 27.12.2014 Michael cEMI Patch adopted	
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
# export BUSSDK_SOURCES=http://netcologne.dl.sourceforge.net/project/bcusdk/bcusdk/bcusdk_0.0.5.tar.gz
# create folders
mkdir -p $PTHSEM_PATH
# mkdir -p $BUSSDK_PATH
apt-get -y install autoconf pkg-config libtool
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
# cd $BUSSDK_PATH
# wget http://netcologne.dl.sourceforge.net/project/bcusdk/bcusdk/bcusdk_0.0.5.tar.gz
# tar -xvzf bcusdk_0.0.5.tar.gz
# cd bcusdk-0.0.5
cd ${BUILD_PATH}
git clone git://bcusdk.git.sourceforge.net/gitroot/bcusdk/bcusdk
cd bcusdk
# http://sourceforge.net/p/bcusdk/patches/2/
wget http://sourceforge.net/p/bcusdk/patches/_discuss/thread/31eb300b/ecc9/attachment/bcusdk-usb-cemi.patch
patch -p1 < bcusdk-usb-cemi.patch
# http://sourceforge.net/p/bcusdk/mailman/message/30803080/
# http://sourceforge.net/mailarchive/attachment.php?list_name=bcusdk-list&message_id=518400B2.3040907%40nimkannon.de&counter=1
# http://sourceforge.net/p/bcusdk/mailman/attachment/518400B2.3040907@nimkannon.de/1/
wget http://sourceforge.net/p/bcusdk/mailman/attachment/518400B2.3040907@nimkannon.de/1/ -O read-within-callback.patch
patch -p3 < read-within-callback.patch
echo m4_pattern_allow\([AC_PROG_LIBTOOL]\) >> configure.in
autoreconf -fi
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
	--enable-usb \
    --enable-static=yes \
	--prefix=$INSTALL_PREFIX \
	CFLAGS="-static -static-libgcc -static-libstdc++" \
	LDFLAGS="-static -static-libgcc -static-libstdc++ -s" \
	CPPFLAGS="-static -static-libgcc -static-libstdc++" 
	# --with-pth=$INSTALL_PREFIX 

# For USB Debugging:
# 	CFLAGS="-static -static-libgcc -static-libstdc++ -DENABLE_LOGGING=1 -DENABLE_DEBUG_LOGGING=1" \
#	CPPFLAGS="-static -static-libgcc -static-libstdc++ -DENABLE_LOGGING=1 -DENABLE_DEBUG_LOGGING=1" 
	
make && make install
# clean up 
rm -r $BUILD_PATH
# New user and group eibd for running eibd, Group member of dailot for permissions on device /dev/ttyAMA0
useradd eibd -s /bin/false -U -M -G dialout
# Add pi user to eibd group for scanning usb devices
usermod -a -G eibd pi
# And eibd himself to group eibd too
usermod -a -G eibd eibd

# http://knx-user-forum.de/342820-post9.html
cat > /etc/udev/rules.d/90-knxusb-devices.rules <<EOF
# Siemens KNX
SUBSYSTEM=="usb", ATTR{idVendor}=="0e77", ATTR{idProduct}=="0111", ACTION=="add", GROUP="eibd", MODE="0664"
SUBSYSTEM=="usb", ATTR{idVendor}=="0e77", ATTR{idProduct}=="0112", ACTION=="add", GROUP="eibd", MODE="0664"
SUBSYSTEM=="usb", ATTR{idVendor}=="0681", ATTR{idProduct}=="0014", ACTION=="add", GROUP="eibd", MODE="0664"
# Merlin Gerin KNX-USB Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="0e77", ATTR{idProduct}=="0141", ACTION=="add", GROUP="eibd", MODE="0664"
# Hensel KNX-USB Interface 
SUBSYSTEM=="usb", ATTR{idVendor}=="0e77", ATTR{idProduct}=="0121", ACTION=="add", GROUP="eibd", MODE="0664"
# Busch-Jaeger KNX-USB Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="145c", ATTR{idProduct}=="1330", ACTION=="add", GROUP="eibd", MODE="0664"
SUBSYSTEM=="usb", ATTR{idVendor}=="145c", ATTR{idProduct}=="1490", ACTION=="add", GROUP="eibd", MODE="0664"
# ABB STOTZ-KONTAKT KNX-USB Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="147b", ATTR{idProduct}=="5120", ACTION=="add", GROUP="eibd", MODE="0664"
# Feller KNX-USB Data Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="135e", ATTR{idProduct}=="0026", ACTION=="add", GROUP="eibd", MODE="0664"
# JUNG KNX-USB Data Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="135e", ATTR{idProduct}=="0023", ACTION=="add", GROUP="eibd", MODE="0664"
# Gira KNX-USB Data Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="135e", ATTR{idProduct}=="0022", ACTION=="add", GROUP="eibd", MODE="0664"
# Berker KNX-USB Data Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="135e", ATTR{idProduct}=="0021", ACTION=="add", GROUP="eibd", MODE="0664"
# Insta KNX-USB Data Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="135e", ATTR{idProduct}=="0020", ACTION=="add", GROUP="eibd", MODE="0664"
# Weinzierl KNX-USB Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="0e77", ATTR{idProduct}=="0104", ACTION=="add", GROUP="eibd", MODE="0664"
# Weinzierl KNX-USB Interface (RS232)
SUBSYSTEM=="usb", ATTR{idVendor}=="0e77", ATTR{idProduct}=="0103", ACTION=="add", GROUP="eibd", MODE="0664"
# Weinzierl KNX-USB Interface (Flush mounted)
SUBSYSTEM=="usb", ATTR{idVendor}=="0e77", ATTR{idProduct}=="0102", ACTION=="add", GROUP="eibd", MODE="0664"
# Tapko USB Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="16d0", ATTR{idProduct}=="0490", ACTION=="add", GROUP="eibd", MODE="0664"
# Hager KNX-USB Data Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="135e", ATTR{idProduct}=="0025", ACTION=="add", GROUP="eibd", MODE="0664"
# preussen automation USB2KNX
SUBSYSTEM=="usb", ATTR{idVendor}=="16d0", ATTR{idProduct}=="0492", ACTION=="add", GROUP="eibd", MODE="0664"
# Merten KNX-USB Data Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="135e", ATTR{idProduct}=="0024", ACTION=="add", GROUP="eibd", MODE="0664"
# b+b EIBWeiche USB
SUBSYSTEM=="usb", ATTR{idVendor}=="04cc", ATTR{idProduct}=="0301", ACTION=="add", GROUP="eibd", MODE="0664"
# MDT KNX_USB_Interface
SUBSYSTEM=="usb", ATTR{idVendor}=="16d0", ATTR{idProduct}=="0491", ACTION=="add", GROUP="eibd", MODE="0664"
EOF
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
# To enable usb backend support uncomment the following 2 lines by removing
export USBID=\$($INSTALL_PREFIX/bin/findknxusb | grep device: | cut -d' ' -f2)
export EIBD_OPTIONS="-d -D -T -R -S -i -u --eibaddr=1.1.128 usb:\$USBID"
# If you want to use the usb backend comment the follwing next line with a #
# export EIBD_OPTIONS="-d -D -T -R -S -i -u --eibaddr=1.1.128 tpuarts:/dev/ttyAMA0"
# export EIBD_OPTIONS="-d -D -T -R -S -i -u --eibaddr=1.1.128 ipt:192.168.56.1"
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

#grep usbfs /etc/fstab
#if [ $? -ne 0 ]; then echo 'none /proc/bus/usb usbfs defaults,devmode=0666 0 0' >> /etc/fstab; fi

sync
# Modify /boot/cmdline.txt to disable boot screen over serial interface
sed -e's/ console=ttyAMA0,115200 kgdboc=ttyAMA0,115200//g' /boot/cmdline.txt --in-place=.bak
# Disable serial console
sed -e 's/\(T0:23:respawn:\/sbin\/getty -L ttyAMA0 115200 vt100\)/# \1/g' /etc/inittab --in-place=.bak
# activate init script
# update-rc.d eibd defaults
echo Please reboot your device...



