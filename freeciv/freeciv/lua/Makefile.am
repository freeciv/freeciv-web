## Process this file with automake to produce Makefile.in

pkgsysconfdir = $(sysconfdir)/$(PACKAGE)

if SERVER
SERVER_CONF = database.lua
else
SERVER_FILES =
SERVER_CONF =
endif

pkgsysconf_DATA = $(SERVER_CONF)

EXTRA_DIST = \
	database.lua
