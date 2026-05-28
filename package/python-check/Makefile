include $(TOPDIR)/rules.mk

PKG_NAME:=python-check
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/python-check
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Python Version Checker
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) \
		-o $(PKG_BUILD_DIR)/python-check \
		./src/python-check.c
endef

define Package/python-check/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) \
		$(PKG_BUILD_DIR)/python-check  \
		$(1)/usr/bin/
endef

$(eval $(call BuildPackage,python-check))