Format: 1.0
Source: rrdtool
Binary: rrdtool, rrdcached, rrdtool-dbg, librrd4, librrd-dev, librrds-perl, librrdp-perl, rrdtool-tcl, python-rrdtool, librrd-ruby, librrd-ruby1.8, librrd-ruby1.9.1, liblua5.1-rrd0, liblua5.1-rrd-dev
Architecture: any all
Version: 1.4.7-2+rpi1+nmu1
Maintainer: Debian RRDtool Team <rrdtool@ml.snow-crash.org>
Uploaders: Sebastian Harl <tokkee@debian.org>, Alexander Wirt <formorer@debian.org>, Bernd Zeimetz <bzed@debian.org>
Homepage: http://oss.oetiker.ch/rrdtool/
Standards-Version: 3.9.2
Vcs-Browser: http://git.snow-crash.org/?p=pkg-rrdtool.git;a=summary
Vcs-Git: git://git.snow-crash.org/pkg-rrdtool.git/
Build-Depends: debhelper (>= 5.0.38), groff, autotools-dev, gettext, quilt, zlib1g-dev, libpng12-dev, libcairo2-dev, libpango1.0-dev, libfreetype6-dev, libdbi0-dev, libxml2-dev, tcl-dev (>= 8), tcl-dev (<= 9), perl (>= 5.8.0), python-all-dev (>= 2.6.6-3~), python-all-dbg (>= 2.6.6-3~), ruby1.8, ruby1.8-dev, ruby1.9.1, ruby1.9.1-dev, liblua5.1-0-dev, lua5.1, gcc-4.7
Build-Conflicts: lua50
Package-List: 
 liblua5.1-rrd-dev deb libdevel optional
 liblua5.1-rrd0 deb interpreters optional
 librrd-dev deb libdevel optional
 librrd-ruby deb ruby optional
 librrd-ruby1.8 deb ruby optional
 librrd-ruby1.9.1 deb ruby optional
 librrd4 deb libs optional
 librrdp-perl deb perl optional
 librrds-perl deb perl optional
 python-rrdtool deb python optional
 rrdcached deb utils optional
 rrdtool deb utils optional
 rrdtool-dbg deb debug extra
 rrdtool-tcl deb utils optional
Checksums-Sha1: 
 faab7df7696b69f85d6f89dd9708d7cf0c9a273b 1349040 rrdtool_1.4.7.orig.tar.gz
 247e19ee9460f5b657f34d96e0ee22222eb7d31c 28018 rrdtool_1.4.7-2+rpi1+nmu1.diff.gz
Checksums-Sha256: 
 956aaf431c955ba88dd7d98920ade3a8c4bad04adb1f9431377950a813a7af11 1349040 rrdtool_1.4.7.orig.tar.gz
 f4d163bfca8fa8f9829aa10aa628ebf26284d23a540e8e0e018fa27ca17235f2 28018 rrdtool_1.4.7-2+rpi1+nmu1.diff.gz
Files: 
 ffe369d8921b4dfdeaaf43812100c38f 1349040 rrdtool_1.4.7.orig.tar.gz
 2506db4c96f7505567cd3f6ce302e682 28018 rrdtool_1.4.7-2+rpi1+nmu1.diff.gz
