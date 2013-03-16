# for arm devices just use the *.deb files and skip this 
# first solve dependencies
apt-get install gettext libpango1.0-dev libxml2-dev checkinstall

# get sources
wget http://oss.oetiker.ch/rrdtool/pub/rrdtool-1.4.5.tar.gz
gunzip -c rrdtool-1.4.5.tar.gz | tar xf -

##### todo: make patch file so you have to replace one file
# in folder ./rrdtool-1.4.5/src/ replace rrd_tool.c

BUILD_DIR=/tmp/rrdbuild
INSTALL_DIR=/usr
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# be careful with make install
cd rrdtool-1.4.5
./configure --prefix=$INSTALL_DIR && make && make install

