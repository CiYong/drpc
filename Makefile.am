
ACLOCAL_AMFLAGS= -I m4

LIBS = -lzmq -luuid

AM_CPPFLAGS= \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/src

lib_LTLIBRARIES=libdrpc.la

include_HEADERS = \
        include/drpc.hpp

libdrpc_la_SOURCES= \
	src/drpc.cpp \
	src/channel.cpp

libdrpc_la_LDFLAGS= \
	-version-info 0:1:0
