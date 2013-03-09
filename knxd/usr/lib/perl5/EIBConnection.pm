#
#  EIBD client library
#  Copyright (C) 2005-2010 Martin Koegler <mkoegler@auto.tuwien.ac.at>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  In addition to the permissions in the GNU General Public License, 
#  you may link the compiled version of this file into combinations
#  with other programs, and distribute those combinations without any 
#  restriction coming from the use of this file. (The General Public 
#  License restrictions do apply in other respects; for example, they 
#  cover modification of the file, and distribution when not linked into 
#  a combine executable.)
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
package EIBConnection;
#sub TCP_NODELAY () { 1 }
use Errno;
use Socket ('IPPROTO_TCP', 'TCP_NODELAY');
use Socket;
use Fcntl ('F_GETFL', 'F_SETFL', 'O_NONBLOCK');
use constant ('IMG_UNKNOWN_ERROR', 0);
use constant ('IMG_UNRECOG_FORMAT', 1);
use constant ('IMG_INVALID_FORMAT', 2);
use constant ('IMG_NO_BCUTYPE', 3);
use constant ('IMG_UNKNOWN_BCUTYPE', 4);
use constant ('IMG_NO_CODE', 5);
use constant ('IMG_NO_SIZE', 6);
use constant ('IMG_LODATA_OVERFLOW', 7);
use constant ('IMG_HIDATA_OVERFLOW', 8);
use constant ('IMG_TEXT_OVERFLOW', 9);
use constant ('IMG_NO_ADDRESS', 10);
use constant ('IMG_WRONG_SIZE', 11);
use constant ('IMG_IMAGE_LOADABLE', 12);
use constant ('IMG_NO_DEVICE_CONNECTION', 13);
use constant ('IMG_MASK_READ_FAILED', 14);
use constant ('IMG_WRONG_MASK_VERSION', 15);
use constant ('IMG_CLEAR_ERROR', 16);
use constant ('IMG_RESET_ADDR_TAB', 17);
use constant ('IMG_LOAD_HEADER', 18);
use constant ('IMG_LOAD_MAIN', 19);
use constant ('IMG_ZERO_RAM', 20);
use constant ('IMG_FINALIZE_ADDR_TAB', 21);
use constant ('IMG_PREPARE_RUN', 22);
use constant ('IMG_RESTART', 23);
use constant ('IMG_LOADED', 24);
use constant ('IMG_NO_START', 25);
use constant ('IMG_WRONG_ADDRTAB', 26);
use constant ('IMG_ADDRTAB_OVERFLOW', 27);
use constant ('IMG_OVERLAP_ASSOCTAB', 28);
use constant ('IMG_OVERLAP_TEXT', 29);
use constant ('IMG_NEGATIV_TEXT_SIZE', 30);
use constant ('IMG_OVERLAP_PARAM', 31);
use constant ('IMG_OVERLAP_EEPROM', 32);
use constant ('IMG_OBJTAB_OVERFLOW', 33);
use constant ('IMG_WRONG_LOADCTL', 34);
use constant ('IMG_UNLOAD_ADDR', 35);
use constant ('IMG_UNLOAD_ASSOC', 36);
use constant ('IMG_UNLOAD_PROG', 37);
use constant ('IMG_LOAD_ADDR', 38);
use constant ('IMG_WRITE_ADDR', 39);
use constant ('IMG_SET_ADDR', 40);
use constant ('IMG_FINISH_ADDR', 41);
use constant ('IMG_LOAD_ASSOC', 42);
use constant ('IMG_WRITE_ASSOC', 43);
use constant ('IMG_SET_ASSOC', 44);
use constant ('IMG_FINISH_ASSOC', 45);
use constant ('IMG_LOAD_PROG', 46);
use constant ('IMG_ALLOC_LORAM', 47);
use constant ('IMG_ALLOC_HIRAM', 48);
use constant ('IMG_ALLOC_INIT', 49);
use constant ('IMG_ALLOC_RO', 50);
use constant ('IMG_ALLOC_EEPROM', 51);
use constant ('IMG_ALLOC_PARAM', 52);
use constant ('IMG_SET_PROG', 53);
use constant ('IMG_SET_TASK_PTR', 54);
use constant ('IMG_SET_OBJ', 55);
use constant ('IMG_SET_TASK2', 56);
use constant ('IMG_FINISH_PROC', 57);
use constant ('IMG_WRONG_CHECKLIM', 58);
use constant ('IMG_INVALID_KEY', 59);
use constant ('IMG_AUTHORIZATION_FAILED', 60);
use constant ('IMG_KEY_WRITE', 61);
sub EIBGetAPDU_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 37 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
        sub bytes::length (_) ;
        ;
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 2);
    return &bytes::length(${$$self{'buf'};});
}
sub EIBGetAPDU_async {
    use bytes;
    my($self, $buf) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $$self{'buf'} = $buf;
    $$self{'complete'} = 'EIBGetAPDU_complete';
    return 0;
}
sub EIBGetAPDU {
    use bytes;
    my($self, $buf) = @_;
    return undef unless $self->EIBGetAPDU_async($buf) == 0;
    return $self->EIBComplete;
}
sub EIBGetAPDU_Src_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 37 or &bytes::length($$self{'data'}) < 4) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    if (defined $$self{'ptr5'}) {
        ${$$self{'ptr5'};} = vec($$self{'data'}, 2, 8) << 8 | vec($$self{'data'}, 3, 8);
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 4);
    return &bytes::length(${$$self{'buf'};});
}
sub EIBGetAPDU_Src_async {
    use bytes;
    my($self, $buf, $src) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $$self{'buf'} = $buf;
    $$self{'ptr5'} = $src;
    $$self{'complete'} = 'EIBGetAPDU_Src_complete';
    return 0;
}
sub EIBGetAPDU_Src {
    use bytes;
    my($self, $buf, $src) = @_;
    return undef unless $self->EIBGetAPDU_Src_async($buf, $src) == 0;
    return $self->EIBComplete;
}
sub EIBGetBusmonitorPacket_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 20 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 2);
    return &bytes::length(${$$self{'buf'};});
}
sub EIBGetBusmonitorPacket_async {
    use bytes;
    my($self, $buf) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $$self{'buf'} = $buf;
    $$self{'complete'} = 'EIBGetBusmonitorPacket_complete';
    return 0;
}
sub EIBGetBusmonitorPacket {
    use bytes;
    my($self, $buf) = @_;
    return undef unless $self->EIBGetBusmonitorPacket_async($buf) == 0;
    return $self->EIBComplete;
}
sub EIBGetGroup_Src_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 39 or &bytes::length($$self{'data'}) < 6) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    if (defined $$self{'ptr5'}) {
        ${$$self{'ptr5'};} = vec($$self{'data'}, 2, 8) << 8 | vec($$self{'data'}, 3, 8);
    }
    if (defined $$self{'ptr6'}) {
        ${$$self{'ptr6'};} = vec($$self{'data'}, 4, 8) << 8 | vec($$self{'data'}, 5, 8);
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 6);
    return &bytes::length(${$$self{'buf'};});
}
sub EIBGetGroup_Src_async {
    use bytes;
    my($self, $buf, $src, $dest) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $$self{'buf'} = $buf;
    $$self{'ptr5'} = $src;
    $$self{'ptr6'} = $dest;
    $$self{'complete'} = 'EIBGetGroup_Src_complete';
    return 0;
}
sub EIBGetGroup_Src {
    use bytes;
    my($self, $buf, $src, $dest) = @_;
    return undef unless $self->EIBGetGroup_Src_async($buf, $src, $dest) == 0;
    return $self->EIBComplete;
}
sub EIBGetTPDU_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 37 or &bytes::length($$self{'data'}) < 4) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    if (defined $$self{'ptr5'}) {
        ${$$self{'ptr5'};} = vec($$self{'data'}, 2, 8) << 8 | vec($$self{'data'}, 3, 8);
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 4);
    return &bytes::length(${$$self{'buf'};});
}
sub EIBGetTPDU_async {
    use bytes;
    my($self, $buf, $src) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $$self{'buf'} = $buf;
    $$self{'ptr5'} = $src;
    $$self{'complete'} = 'EIBGetTPDU_complete';
    return 0;
}
sub EIBGetTPDU {
    use bytes;
    my($self, $buf, $src) = @_;
    return undef unless $self->EIBGetTPDU_async($buf, $src) == 0;
    return $self->EIBComplete;
}
sub EIB_Cache_Clear_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 114 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_Cache_Clear_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 114;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_Cache_Clear_complete';
    return 0;
}
sub EIB_Cache_Clear {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIB_Cache_Clear_async == 0;
    return $self->EIBComplete;
}
sub EIB_Cache_Disable_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 113 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_Cache_Disable_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 113;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_Cache_Disable_complete';
    return 0;
}
sub EIB_Cache_Disable {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIB_Cache_Disable_async == 0;
    return $self->EIBComplete;
}
sub EIB_Cache_Enable_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) == 1) {
        $! = &Errno::EBUSY;
        return undef;
    }
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 112 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_Cache_Enable_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 112;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_Cache_Enable_complete';
    return 0;
}
sub EIB_Cache_Enable {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIB_Cache_Enable_async == 0;
    return $self->EIBComplete;
}
sub EIB_Cache_Read_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 117 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    if ((vec($$self{'data'}, 4, 8) << 8 | vec($$self{'data'}, 5, 8)) == 0) {
        $! = &Errno::ENODEV;
        return undef;
    }
    if (&bytes::length($$self{'data'}) <= 6) {
        $! = &Errno::ENOENT;
        return undef;
    }
    if (defined $$self{'ptr5'}) {
        ${$$self{'ptr5'};} = vec($$self{'data'}, 2, 8) << 8 | vec($$self{'data'}, 3, 8);
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 6);
    return &bytes::length(${$$self{'buf'};});
}
sub EIB_Cache_Read_async {
    use bytes;
    my($self, $dst, $src, $buf) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 4; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 4;
    $$self{'buf'} = $buf;
    $$self{'ptr5'} = $src;
    $ibuf[2] = $dst >> 8 & 255;
    $ibuf[3] = $dst & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 117;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_Cache_Read_complete';
    return 0;
}
sub EIB_Cache_Read {
    use bytes;
    my($self, $dst, $src, $buf) = @_;
    return undef unless $self->EIB_Cache_Read_async($dst, $src, $buf) == 0;
    return $self->EIBComplete;
}
sub EIB_Cache_Read_Sync_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 116 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    if ((vec($$self{'data'}, 4, 8) << 8 | vec($$self{'data'}, 5, 8)) == 0) {
        $! = &Errno::ENODEV;
        return undef;
    }
    if (&bytes::length($$self{'data'}) <= 6) {
        $! = &Errno::ENOENT;
        return undef;
    }
    if (defined $$self{'ptr5'}) {
        ${$$self{'ptr5'};} = vec($$self{'data'}, 2, 8) << 8 | vec($$self{'data'}, 3, 8);
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 6);
    return &bytes::length(${$$self{'buf'};});
}
sub EIB_Cache_Read_Sync_async {
    use bytes;
    my($self, $dst, $src, $buf, $age) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 6; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 6;
    $$self{'buf'} = $buf;
    $$self{'ptr5'} = $src;
    $ibuf[2] = $dst >> 8 & 255;
    $ibuf[3] = $dst & 255;
    $ibuf[4] = $age >> 8 & 255;
    $ibuf[5] = $age & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 116;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_Cache_Read_Sync_complete';
    return 0;
}
sub EIB_Cache_Read_Sync {
    use bytes;
    my($self, $dst, $src, $buf, $age) = @_;
    return undef unless $self->EIB_Cache_Read_Sync_async($dst, $src, $buf, $age) == 0;
    return $self->EIBComplete;
}
sub EIB_Cache_Remove_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 115 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_Cache_Remove_async {
    use bytes;
    my($self, $dest) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 4; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 4;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 115;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_Cache_Remove_complete';
    return 0;
}
sub EIB_Cache_Remove {
    use bytes;
    my($self, $dest) = @_;
    return undef unless $self->EIB_Cache_Remove_async($dest) == 0;
    return $self->EIBComplete;
}
sub EIB_Cache_LastUpdates_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 118 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    if (defined $$self{'ptr4'}) {
        ${$$self{'ptr4'};} = vec($$self{'data'}, 2, 8) << 8 | vec($$self{'data'}, 3, 8);
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 4);
    return &bytes::length(${$$self{'buf'};});
}
sub EIB_Cache_LastUpdates_async {
    use bytes;
    my($self, $start, $timeout, $buf, $ende) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 5; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 5;
    $$self{'buf'} = $buf;
    $$self{'ptr4'} = $ende;
    $ibuf[2] = $start >> 8 & 255;
    $ibuf[3] = $start & 255;
    $ibuf[4] = $timeout & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 118;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_Cache_LastUpdates_complete';
    return 0;
}
sub EIB_Cache_LastUpdates {
    use bytes;
    my($self, $start, $timeout, $buf, $ende) = @_;
    return undef unless $self->EIB_Cache_LastUpdates_async($start, $timeout, $buf, $ende) == 0;
    return $self->EIBComplete;
}
sub EIB_LoadImage_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 99 or &bytes::length($$self{'data'}) < 4) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return vec($$self{'data'}, 2, 8) << 8 | vec($$self{'data'}, 3, 8);
}
sub EIB_LoadImage_async {
    use bytes;
    my($self, $image) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    if (&bytes::length($image) < 0) {
        $! = &Errno::EINVAL;
        return undef;
    }
    $$self{'sendlen'} = &bytes::length($image);
    for ($i = 0; $i < &bytes::length($image); ++$i) {
        $ibuf[$headlen + $i] = vec($image, $i, 8);
    }
    $ibuf[0] = 0;
    $ibuf[1] = 99;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_LoadImage_complete';
    return 0;
}
sub EIB_LoadImage {
    use bytes;
    my($self, $image) = @_;
    return undef unless $self->EIB_LoadImage_async($image) == 0;
    return $self->EIBComplete;
}
sub EIB_MC_Authorize_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 87 or &bytes::length($$self{'data'}) < 3) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return $$self{'data'}[2];
}
sub EIB_MC_Authorize_async {
    use bytes;
    my($self, $key) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 6; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 6;
    if (@key != 4) {
        $! = &Errno::EINVAL;
        return undef;
    }
    for ($i = 0; $i < 4; ++$i) {
        $ibuf[2 + $i] = vec($key, $i, 8);
    }
    $ibuf[0] = 0;
    $ibuf[1] = 87;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_Authorize_complete';
    return 0;
}
sub EIB_MC_Authorize {
    use bytes;
    my($self, $key) = @_;
    return undef unless $self->EIB_MC_Authorize_async($key) == 0;
    return $self->EIBComplete;
}
sub EIB_MC_Connect_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 80 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_MC_Connect_async {
    use bytes;
    my($self, $dest) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 4; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 4;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 80;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_Connect_complete';
    return 0;
}
sub EIB_MC_Connect {
    use bytes;
    my($self, $dest) = @_;
    return undef unless $self->EIB_MC_Connect_async($dest) == 0;
    return $self->EIBComplete;
}
sub EIB_MC_Individual_Open_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 73 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_MC_Individual_Open_async {
    use bytes;
    my($self, $dest) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 4; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 4;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 73;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_Individual_Open_complete';
    return 0;
}
sub EIB_MC_Individual_Open {
    use bytes;
    my($self, $dest) = @_;
    return undef unless $self->EIB_MC_Individual_Open_async($dest) == 0;
    return $self->EIBComplete;
}
sub EIB_MC_GetMaskVersion_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 89 or &bytes::length($$self{'data'}) < 4) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return vec($$self{'data'}, 2, 8) << 8 | vec($$self{'data'}, 3, 8);
}
sub EIB_MC_GetMaskVersion_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 89;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_GetMaskVersion_complete';
    return 0;
}
sub EIB_MC_GetMaskVersion {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIB_MC_GetMaskVersion_async == 0;
    return $self->EIBComplete;
}
sub EIB_MC_GetPEIType_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 85 or &bytes::length($$self{'data'}) < 4) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return vec($$self{'data'}, 2, 8) << 8 | vec($$self{'data'}, 3, 8);
}
sub EIB_MC_GetPEIType_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 85;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_GetPEIType_complete';
    return 0;
}
sub EIB_MC_GetPEIType {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIB_MC_GetPEIType_async == 0;
    return $self->EIBComplete;
}
sub EIB_MC_Progmode_Off_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 96 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_MC_Progmode_Off_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 3; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 3;
    $ibuf[2] = 0;
    $ibuf[0] = 0;
    $ibuf[1] = 96;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_Progmode_Off_complete';
    return 0;
}
sub EIB_MC_Progmode_Off {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIB_MC_Progmode_Off_async == 0;
    return $self->EIBComplete;
}
sub EIB_MC_Progmode_On_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 96 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_MC_Progmode_On_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 3; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 3;
    $ibuf[2] = 1;
    $ibuf[0] = 0;
    $ibuf[1] = 96;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_Progmode_On_complete';
    return 0;
}
sub EIB_MC_Progmode_On {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIB_MC_Progmode_On_async == 0;
    return $self->EIBComplete;
}
sub EIB_MC_Progmode_Status_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 96 or &bytes::length($$self{'data'}) < 3) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return $$self{'data'}[2];
}
sub EIB_MC_Progmode_Status_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 3; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 3;
    $ibuf[2] = 3;
    $ibuf[0] = 0;
    $ibuf[1] = 96;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_Progmode_Status_complete';
    return 0;
}
sub EIB_MC_Progmode_Status {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIB_MC_Progmode_Status_async == 0;
    return $self->EIBComplete;
}
sub EIB_MC_Progmode_Toggle_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 96 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_MC_Progmode_Toggle_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 3; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 3;
    $ibuf[2] = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 96;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_Progmode_Toggle_complete';
    return 0;
}
sub EIB_MC_Progmode_Toggle {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIB_MC_Progmode_Toggle_async == 0;
    return $self->EIBComplete;
}
sub EIB_MC_PropertyDesc_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 97 or &bytes::length($$self{'data'}) < 6) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    if (defined $$self{'ptr2'}) {
        ${$$self{'ptr2'};} = $$self{'data'}[2];
    }
    if (defined $$self{'ptr4'}) {
        ${$$self{'ptr4'};} = vec($$self{'data'}, 3, 8) << 8 | vec($$self{'data'}, 4, 8);
    }
    if (defined $$self{'ptr3'}) {
        ${$$self{'ptr3'};} = $$self{'data'}[5];
    }
    return 0;
}
sub EIB_MC_PropertyDesc_async {
    use bytes;
    my($self, $obj, $propertyno, $proptype, $max_nr_of_elem, $access) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 4; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 4;
    $$self{'ptr2'} = $proptype;
    $$self{'ptr4'} = $max_nr_of_elem;
    $$self{'ptr3'} = $access;
    $ibuf[2] = $obj & 255;
    $ibuf[3] = $propertyno & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 97;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_PropertyDesc_complete';
    return 0;
}
sub EIB_MC_PropertyDesc {
    use bytes;
    my($self, $obj, $propertyno, $proptype, $max_nr_of_elem, $access) = @_;
    return undef unless $self->EIB_MC_PropertyDesc_async($obj, $propertyno, $proptype, $max_nr_of_elem, $access) == 0;
    return $self->EIBComplete;
}
sub EIB_MC_PropertyRead_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 83 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 2);
    return &bytes::length(${$$self{'buf'};});
}
sub EIB_MC_PropertyRead_async {
    use bytes;
    my($self, $obj, $propertyno, $start, $nr_of_elem, $buf) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 7; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 7;
    $$self{'buf'} = $buf;
    $ibuf[2] = $obj & 255;
    $ibuf[3] = $propertyno & 255;
    $ibuf[4] = $start >> 8 & 255;
    $ibuf[5] = $start & 255;
    $ibuf[6] = $nr_of_elem & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 83;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_PropertyRead_complete';
    return 0;
}
sub EIB_MC_PropertyRead {
    use bytes;
    my($self, $obj, $propertyno, $start, $nr_of_elem, $buf) = @_;
    return undef unless $self->EIB_MC_PropertyRead_async($obj, $propertyno, $start, $nr_of_elem, $buf) == 0;
    return $self->EIBComplete;
}
sub EIB_MC_PropertyScan_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 98 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 2);
    return &bytes::length(${$$self{'buf'};});
}
sub EIB_MC_PropertyScan_async {
    use bytes;
    my($self, $buf) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $$self{'buf'} = $buf;
    $ibuf[0] = 0;
    $ibuf[1] = 98;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_PropertyScan_complete';
    return 0;
}
sub EIB_MC_PropertyScan {
    use bytes;
    my($self, $buf) = @_;
    return undef unless $self->EIB_MC_PropertyScan_async($buf) == 0;
    return $self->EIBComplete;
}
sub EIB_MC_PropertyWrite_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 84 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 2);
    return &bytes::length(${$$self{'buf'};});
}
sub EIB_MC_PropertyWrite_async {
    use bytes;
    my($self, $obj, $propertyno, $start, $nr_of_elem, $buf, $res) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 7; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 7;
    $ibuf[2] = $obj & 255;
    $ibuf[3] = $propertyno & 255;
    $ibuf[4] = $start >> 8 & 255;
    $ibuf[5] = $start & 255;
    $ibuf[6] = $nr_of_elem & 255;
    if (&bytes::length($buf) < 0) {
        $! = &Errno::EINVAL;
        return undef;
    }
    $$self{'sendlen'} = &bytes::length($buf);
    for ($i = 0; $i < &bytes::length($buf); ++$i) {
        $ibuf[$headlen + $i] = vec($buf, $i, 8);
    }
    $$self{'buf'} = $res;
    $ibuf[0] = 0;
    $ibuf[1] = 84;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_PropertyWrite_complete';
    return 0;
}
sub EIB_MC_PropertyWrite {
    use bytes;
    my($self, $obj, $propertyno, $start, $nr_of_elem, $buf, $res) = @_;
    return undef unless $self->EIB_MC_PropertyWrite_async($obj, $propertyno, $start, $nr_of_elem, $buf, $res) == 0;
    return $self->EIBComplete;
}
sub EIB_MC_ReadADC_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 86 or &bytes::length($$self{'data'}) < 4) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    if (defined $$self{'ptr1'}) {
        ${$$self{'ptr1'};} = vec($$self{'data'}, 2, 8) << 8 | vec($$self{'data'}, 3, 8);
    }
    return 0;
}
sub EIB_MC_ReadADC_async {
    use bytes;
    my($self, $channel, $count, $val) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 4; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 4;
    $$self{'ptr1'} = $val;
    $ibuf[2] = $channel & 255;
    $ibuf[3] = $count & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 86;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_ReadADC_complete';
    return 0;
}
sub EIB_MC_ReadADC {
    use bytes;
    my($self, $channel, $count, $val) = @_;
    return undef unless $self->EIB_MC_ReadADC_async($channel, $count, $val) == 0;
    return $self->EIBComplete;
}
sub EIB_MC_Read_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 81 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 2);
    return &bytes::length(${$$self{'buf'};});
}
sub EIB_MC_Read_async {
    use bytes;
    my($self, $addr, $buf_len, $buf) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 6; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 6;
    $$self{'buf'} = $buf;
    $ibuf[2] = $addr >> 8 & 255;
    $ibuf[3] = $addr & 255;
    $ibuf[4] = $buf_len >> 8 & 255;
    $ibuf[5] = $buf_len & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 81;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_Read_complete';
    return 0;
}
sub EIB_MC_Read {
    use bytes;
    my($self, $addr, $buf_len, $buf) = @_;
    return undef unless $self->EIB_MC_Read_async($addr, $buf_len, $name) == 0;
    return $self->EIBComplete;
}
sub EIB_MC_Restart_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 90 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_MC_Restart_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 90;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_Restart_complete';
    return 0;
}
sub EIB_MC_Restart {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIB_MC_Restart_async == 0;
    return $self->EIBComplete;
}
sub EIB_MC_SetKey_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) == 2) {
        $! = &Errno::EPERM;
        return undef;
    }
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 88 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_MC_SetKey_async {
    use bytes;
    my($self, $key, $level) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 7; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 7;
    if (@key != 4) {
        $! = &Errno::EINVAL;
        return undef;
    }
    for ($i = 0; $i < 4; ++$i) {
        $ibuf[2 + $i] = vec($key, $i, 8);
    }
    $ibuf[6] = $level & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 88;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_SetKey_complete';
    return 0;
}
sub EIB_MC_SetKey {
    use bytes;
    my($self, $key, $level) = @_;
    return undef unless $self->EIB_MC_SetKey_async($key, $level) == 0;
    return $self->EIBComplete;
}
sub EIB_MC_Write_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) == 68) {
        $! = &Errno::EIO;
        return undef;
    }
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 82 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return $$self{'sendlen'};
}
sub EIB_MC_Write_async {
    use bytes;
    my($self, $addr, $buf) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 6; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 6;
    $ibuf[2] = $addr >> 8 & 255;
    $ibuf[3] = $addr & 255;
    $ibuf[4] = $buf >> 8 & 255;
    $ibuf[5] = $buf & 255;
    if (&bytes::length($buf) < 0) {
        $! = &Errno::EINVAL;
        return undef;
    }
    $$self{'sendlen'} = &bytes::length($buf);
    for ($i = 0; $i < &bytes::length($buf); ++$i) {
        $ibuf[$headlen + $i] = vec($buf, $i, 8);
    }
    $ibuf[0] = 0;
    $ibuf[1] = 82;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_Write_complete';
    return 0;
}
sub EIB_MC_Write {
    use bytes;
    my($self, $addr, $buf) = @_;
    return undef unless $self->EIB_MC_Write_async($addr, $buf) == 0;
    return $self->EIBComplete;
}
sub EIB_MC_Write_Plain_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 91 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return $$self{'sendlen'};
}
sub EIB_MC_Write_Plain_async {
    use bytes;
    my($self, $addr, $buf) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 6; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 6;
    $ibuf[2] = $addr >> 8 & 255;
    $ibuf[3] = $addr & 255;
    $ibuf[4] = $buf >> 8 & 255;
    $ibuf[5] = $buf & 255;
    if (&bytes::length($buf) < 0) {
        $! = &Errno::EINVAL;
        return undef;
    }
    $$self{'sendlen'} = &bytes::length($buf);
    for ($i = 0; $i < &bytes::length($buf); ++$i) {
        $ibuf[$headlen + $i] = vec($buf, $i, 8);
    }
    $ibuf[0] = 0;
    $ibuf[1] = 91;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_MC_Write_Plain_complete';
    return 0;
}
sub EIB_MC_Write_Plain {
    use bytes;
    my($self, $addr, $buf) = @_;
    return undef unless $self->EIB_MC_Write_Plain_async($addr, $buf) == 0;
    return $self->EIBComplete;
}
sub EIB_M_GetMaskVersion_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 49 or &bytes::length($$self{'data'}) < 4) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return vec($$self{'data'}, 2, 8) << 8 | vec($$self{'data'}, 3, 8);
}
sub EIB_M_GetMaskVersion_async {
    use bytes;
    my($self, $dest) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 4; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 4;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 49;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_M_GetMaskVersion_complete';
    return 0;
}
sub EIB_M_GetMaskVersion {
    use bytes;
    my($self, $dest) = @_;
    return undef unless $self->EIB_M_GetMaskVersion_async($dest) == 0;
    return $self->EIBComplete;
}
sub EIB_M_Progmode_Off_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 48 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_M_Progmode_Off_async {
    use bytes;
    my($self, $dest) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 5; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 5;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    $ibuf[4] = 0;
    $ibuf[0] = 0;
    $ibuf[1] = 48;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_M_Progmode_Off_complete';
    return 0;
}
sub EIB_M_Progmode_Off {
    use bytes;
    my($self, $dest) = @_;
    return undef unless $self->EIB_M_Progmode_Off_async($dest) == 0;
    return $self->EIBComplete;
}
sub EIB_M_Progmode_On_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 48 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_M_Progmode_On_async {
    use bytes;
    my($self, $dest) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 5; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 5;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    $ibuf[4] = 1;
    $ibuf[0] = 0;
    $ibuf[1] = 48;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_M_Progmode_On_complete';
    return 0;
}
sub EIB_M_Progmode_On {
    use bytes;
    my($self, $dest) = @_;
    return undef unless $self->EIB_M_Progmode_On_async($dest) == 0;
    return $self->EIBComplete;
}
sub EIB_M_Progmode_Status_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 48 or &bytes::length($$self{'data'}) < 3) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return $$self{'data'}[2];
}
sub EIB_M_Progmode_Status_async {
    use bytes;
    my($self, $dest) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 5; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 5;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    $ibuf[4] = 3;
    $ibuf[0] = 0;
    $ibuf[1] = 48;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_M_Progmode_Status_complete';
    return 0;
}
sub EIB_M_Progmode_Status {
    use bytes;
    my($self, $dest) = @_;
    return undef unless $self->EIB_M_Progmode_Status_async($dest) == 0;
    return $self->EIBComplete;
}
sub EIB_M_Progmode_Toggle_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 48 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_M_Progmode_Toggle_async {
    use bytes;
    my($self, $dest) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 5; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 5;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    $ibuf[4] = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 48;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_M_Progmode_Toggle_complete';
    return 0;
}
sub EIB_M_Progmode_Toggle {
    use bytes;
    my($self, $dest) = @_;
    return undef unless $self->EIB_M_Progmode_Toggle_async($dest) == 0;
    return $self->EIBComplete;
}
sub EIB_M_ReadIndividualAddresses_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 50 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    ${$$self{'buf'};} = substr($$self{'data'}, 2);
    return &bytes::length(${$$self{'buf'};});
}
sub EIB_M_ReadIndividualAddresses_async {
    use bytes;
    my($self, $buf) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $$self{'buf'} = $buf;
    $ibuf[0] = 0;
    $ibuf[1] = 50;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_M_ReadIndividualAddresses_complete';
    return 0;
}
sub EIB_M_ReadIndividualAddresses {
    use bytes;
    my($self, $buf) = @_;
    return undef unless $self->EIB_M_ReadIndividualAddresses_async($buf) == 0;
    return $self->EIBComplete;
}
sub EIB_M_WriteIndividualAddress_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) == 65) {
        $! = &Errno::EADDRINUSE;
        return undef;
    }
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) == 67) {
        $! = &Errno::ETIMEDOUT;
        return undef;
    }
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) == 66) {
        $! = &Errno::EADDRNOTAVAIL;
        return undef;
    }
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 64 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIB_M_WriteIndividualAddress_async {
    use bytes;
    my($self, $dest) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 4; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 4;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 64;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIB_M_WriteIndividualAddress_complete';
    return 0;
}
sub EIB_M_WriteIndividualAddress {
    use bytes;
    my($self, $dest) = @_;
    return undef unless $self->EIB_M_WriteIndividualAddress_async($dest) == 0;
    return $self->EIBComplete;
}
sub EIBOpenBusmonitor_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) == 1) {
        $! = &Errno::EBUSY;
        return undef;
    }
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 16 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIBOpenBusmonitor_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 16;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIBOpenBusmonitor_complete';
    return 0;
}
sub EIBOpenBusmonitor {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIBOpenBusmonitor_async == 0;
    return $self->EIBComplete;
}
sub EIBOpenBusmonitorText_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) == 1) {
        $! = &Errno::EBUSY;
        return undef;
    }
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 17 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIBOpenBusmonitorText_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 17;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIBOpenBusmonitorText_complete';
    return 0;
}
sub EIBOpenBusmonitorText {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIBOpenBusmonitorText_async == 0;
    return $self->EIBComplete;
}
sub EIBOpen_GroupSocket_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 38 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIBOpen_GroupSocket_async {
    use bytes;
    my($self, $write_only) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 5; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 5;
    $ibuf[4] = $write_only ? 255 : 0;
    $ibuf[0] = 0;
    $ibuf[1] = 38;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIBOpen_GroupSocket_complete';
    return 0;
}
sub EIBOpen_GroupSocket {
    use bytes;
    my($self, $write_only) = @_;
    return undef unless $self->EIBOpen_GroupSocket_async($write_only) == 0;
    return $self->EIBComplete;
}
sub EIBOpenT_Broadcast_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 35 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIBOpenT_Broadcast_async {
    use bytes;
    my($self, $write_only) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 5; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 5;
    $ibuf[4] = $write_only ? 255 : 0;
    $ibuf[0] = 0;
    $ibuf[1] = 35;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIBOpenT_Broadcast_complete';
    return 0;
}
sub EIBOpenT_Broadcast {
    use bytes;
    my($self, $write_only) = @_;
    return undef unless $self->EIBOpenT_Broadcast_async($write_only) == 0;
    return $self->EIBComplete;
}
sub EIBOpenT_Connection_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 32 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIBOpenT_Connection_async {
    use bytes;
    my($self, $dest) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 5; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 5;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 32;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIBOpenT_Connection_complete';
    return 0;
}
sub EIBOpenT_Connection {
    use bytes;
    my($self, $dest) = @_;
    return undef unless $self->EIBOpenT_Connection_async($dest) == 0;
    return $self->EIBComplete;
}
sub EIBOpenT_Group_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 34 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIBOpenT_Group_async {
    use bytes;
    my($self, $dest, $write_only) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 5; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 5;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    $ibuf[4] = $write_only ? 255 : 0;
    $ibuf[0] = 0;
    $ibuf[1] = 34;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIBOpenT_Group_complete';
    return 0;
}
sub EIBOpenT_Group {
    use bytes;
    my($self, $dest, $write_only) = @_;
    return undef unless $self->EIBOpenT_Group_async($dest, $write_only) == 0;
    return $self->EIBComplete;
}
sub EIBOpenT_Individual_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 33 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIBOpenT_Individual_async {
    use bytes;
    my($self, $dest, $write_only) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 5; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 5;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    $ibuf[4] = $write_only ? 255 : 0;
    $ibuf[0] = 0;
    $ibuf[1] = 33;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIBOpenT_Individual_complete';
    return 0;
}
sub EIBOpenT_Individual {
    use bytes;
    my($self, $dest, $write_only) = @_;
    return undef unless $self->EIBOpenT_Individual_async($dest, $write_only) == 0;
    return $self->EIBComplete;
}
sub EIBOpenT_TPDU_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 36 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIBOpenT_TPDU_async {
    use bytes;
    my($self, $src) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 5; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 5;
    $ibuf[2] = $src >> 8 & 255;
    $ibuf[3] = $src & 255;
    $ibuf[0] = 0;
    $ibuf[1] = 36;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIBOpenT_TPDU_complete';
    return 0;
}
sub EIBOpenT_TPDU {
    use bytes;
    my($self, $src) = @_;
    return undef unless $self->EIBOpenT_TPDU_async($src) == 0;
    return $self->EIBComplete;
}
sub EIBOpenVBusmonitor_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) == 1) {
        $! = &Errno::EBUSY;
        return undef;
    }
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 18 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIBOpenVBusmonitor_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 18;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIBOpenVBusmonitor_complete';
    return 0;
}
sub EIBOpenVBusmonitor {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIBOpenVBusmonitor_async == 0;
    return $self->EIBComplete;
}
sub EIBOpenVBusmonitorText_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) == 1) {
        $! = &Errno::EBUSY;
        return undef;
    }
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 19 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIBOpenVBusmonitorText_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 19;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIBOpenVBusmonitorText_complete';
    return 0;
}
sub EIBOpenVBusmonitorText {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIBOpenVBusmonitorText_async == 0;
    return $self->EIBComplete;
}
sub EIBReset_complete {
    use bytes;
    my $self = shift @_;
    $$self{'complete'} = undef;
    my $i;
    return undef unless $self->_EIB_Get_Request == 0;
    if ((vec($$self{'data'}, 0, 8) << 8 | vec($$self{'data'}, 1, 8)) != 4 or &bytes::length($$self{'data'}) < 2) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    return 0;
}
sub EIBReset_async {
    use bytes;
    my($self) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    $ibuf[0] = 0;
    $ibuf[1] = 4;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    $$self{'complete'} = 'EIBReset_complete';
    return 0;
}
sub EIBReset {
    use bytes;
    my($self) = @_;
    return undef unless $self->EIBReset_async == 0;
    return $self->EIBComplete;
}
sub EIBSendAPDU {
    use bytes;
    my($self, $data) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 2; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 2;
    if (&bytes::length($data) < 2) {
        $! = &Errno::EINVAL;
        return undef;
    }
    $$self{'sendlen'} = &bytes::length($data);
    for ($i = 0; $i < &bytes::length($data); ++$i) {
        $ibuf[$headlen + $i] = vec($data, $i, 8);
    }
    $ibuf[0] = 0;
    $ibuf[1] = 37;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    return $$self{'sendlen'};
}
sub EIBSendGroup {
    use bytes;
    my($self, $dest, $data) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 4; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 4;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    if (&bytes::length($data) < 2) {
        $! = &Errno::EINVAL;
        return undef;
    }
    $$self{'sendlen'} = &bytes::length($data);
    for ($i = 0; $i < &bytes::length($data); ++$i) {
        $ibuf[$headlen + $i] = vec($data, $i, 8);
    }
    $ibuf[0] = 0;
    $ibuf[1] = 39;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    return $$self{'sendlen'};
}
sub EIBSendTPDU {
    use bytes;
    my($self, $dest, $data) = @_;
    my $i;
    my(@ibuf) = {};
    for ($i = 0; $i < 4; ++$i) {
        $ibuf[$i] = 0;
    }
    my $headlen = 4;
    $ibuf[2] = $dest >> 8 & 255;
    $ibuf[3] = $dest & 255;
    if (&bytes::length($data) < 2) {
        $! = &Errno::EINVAL;
        return undef;
    }
    $$self{'sendlen'} = &bytes::length($data);
    for ($i = 0; $i < &bytes::length($data); ++$i) {
        $ibuf[$headlen + $i] = vec($data, $i, 8);
    }
    $ibuf[0] = 0;
    $ibuf[1] = 37;
    return undef unless $self->_EIB_SendRequest(pack('C' . @ibuf, @ibuf)) == 0;
    return $$self{'sendlen'};
}
sub EIBSocketLocal {
    use bytes;
    my $class = shift @_;
    my $self = {};
    bless $self, $class;
    my($path) = @_;
    socket $$self{'fd'}, 1, 1, 0;
    return undef unless connect $$self{'fd'}, sockaddr_un($path);
    $$self{'data'} = {};
    $$self{'readlen'} = 0;
    return $self;
}
sub EIBSocketRemote {
    use bytes;
    my $class = shift @_;
    my $self = {};
    bless $self, $class;
    my($ip, $port) = @_;
    return undef unless my $host = gethostbyname $ip;
    $port = $port || 6720;
    socket $$self{'fd'}, 2, 1, getprotobyname 'tcp';
    return undef unless connect $$self{'fd'}, sockaddr_in($port, $host);
    setsockopt $$self{'fd'}, 6, &TCP_NODELAY, 1;
    $$self{'data'} = {};
    $$self{'readlen'} = 0;
    return $self;
}
sub EIBSocketURL {
    use bytes;
    my $class = shift @_;
    my($url) = @_;
    if ($url =~ /^local:(.*)$/) {
        return $class->EIBSocketLocal($1);
    }
    if ($url =~ /^ip:([^:]+)(:[0-9]+)?$/) {
        my $port = defined $2 ? substr($2, 1) : undef;
        return $class->EIBSocketRemote($1, $port);
    }
    $! = &Errno::EINVAL;
    return undef;
}
sub EIBSocketClose_sync {
    use bytes;
    my $self = shift @_;
    $self->EIBReset;
    return $self->EIBClose;
}
sub EIBClose {
    use bytes;
    my $self = shift @_;
    return undef unless close $$self{'fd'};
    $$self{'fd'} = undef;
    return 0;
}
sub _EIB_SendRequest {
    use bytes;
    my $self = shift @_;
    my($data) = @_;
    my $result;
    if (not defined $$self{'fd'}) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    if (&bytes::length($data) >= 65535 or &bytes::length($data) < 2) {
        $! = &Errno::EINVAL;
        return undef;
    }
    $result = pack('n', &bytes::length($data)) . $data;
    return undef unless defined send($$self{'fd'}, $result, 0);
    return 0;
}
sub _EIB_CheckRequest {
    use bytes;
    my $self = shift @_;
    my $buffer;
    my $i;
    my($block) = @_;
    if (not defined $$self{'fd'}) {
        $! = &Errno::ECONNRESET;
        return undef;
    }
    if ($$self{'readlen'} == 0) {
        $$self{'data'} = '';
        $$self{'head'} = '';
    }
    if ($$self{'readlen'} < 2) {
        fcntl $$self{'fd'}, 4, fcntl($$self{'fd'}, 3, 0) & 4294965247 | ($block ? 0 : 2048);
        if (not defined recv($$self{'fd'}, $buffer, 2 - $$self{'readlen'}, 0)) {
            $! = &Errno::ECONNRESET;
            return -1;
        }
        $$self{'head'} .= $buffer;
        $$self{'readlen'} += &bytes::length($buffer);
    }
    if ($$self{'readlen'} < 2) {
        return 0;
    }
    $$self{'datalen'} = unpack('n', $$self{'head'});
    if ($$self{'readlen'} < $$self{'datalen'} + 2) {
        fcntl $$self{'fd'}, 4, fcntl($$self{'fd'}, 3, 0) & 4294965247 | ($block ? 0 : 2048);
        if (not defined recv($$self{'fd'}, $buffer, $$self{'datalen'} + 2 - $$self{'readlen'}, 0)) {
            $! = &Errno::ECONNRESET;
            return undef;
        }
        $$self{'data'} .= $buffer;
        $$self{'readlen'} += &bytes::length($buffer);
    }
    return 0;
}
sub _EIB_Get_Request {
    use bytes;
    my $self = shift @_;
    do {
        return undef unless defined $self->_EIB_CheckRequest('true')
    } while $$self{'readlen'} < 2 or $$self{'readlen'} >= 2 and $$self{'readlen'} < $$self{'datalen'} + 2;
    $$self{'readlen'} = 0;
    return 0;
}
sub EIB_Poll_Complete {
    use bytes;
    my $self = shift @_;
    return undef unless defined $self->_EIB_CheckRequest('false');
    if ($$self{'readlen'} < 2 or $$self{'readlen'} >= 2 and $$self{'readlen'} < $$self{'datalen'} + 2) {
        return 0;
    }
    return 1;
}
sub EIB_Poll_FD {
    use bytes;
    my $self = shift @_;
    if (not defined $$self{'fd'}) {
        $! = &Errno::EINVAL;
        return undef;
    }
    return $$self{'fd'};
}
sub EIBComplete {
    use bytes;
    my $self = shift @_;
    my $func = $$self{'complete'};
    if (not defined $$self{'fd'} or not defined $func) {
        $! = &Errno::EINVAL;
        return undef;
    }
    $self->$func;
}
sub EIBBuffer {
    use bytes;
    my $buffer = '';
    return \$buffer;
}
sub EIBAddr {
    use bytes;
    my $addr = undef;
    return \$addr;
}
sub EIBInt8 {
    use bytes;
    my $data = undef;
    return \$data;
}
sub EIBInt16 {
    use bytes;
    my $data = undef;
    return \$data;
}
use bytes;
'???';
