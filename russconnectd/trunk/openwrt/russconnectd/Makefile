include $(TOPDIR)/rules.mk

PKG_NAME:=russconnectd
PKG_VERSION:=0.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/openautomation
PKG_CAT:=zcat
PKG_MD5SUM:=571001b17b4bb75a97e34c4b3fa798cf

PKG_INSTALL:=1

PKG_BUILD_DEPENDS:=bcusdk

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/russconnectd
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Russsound RNET(tm) EIB/KNX connector
  URL:=http://sourceforge.net//openautomation
  DEPENDS:=libeibclient
endef

# FIXME: depend on uclibc++??

define Package/russconnectd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/$(PKG_NAME) $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/init $(1)/etc/init.d/$(PKG_NAME)
endef

$(eval $(call BuildPackage,russconnectd))
