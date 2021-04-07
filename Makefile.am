
ACLOCAL_AMFLAGS= -I m4

AM_CPPFLAGS= \
	-I$(top_builddir)/include \
	-I$(top_srcdir)/src \
        -std=c++11

lib_LTLIBRARIES=libdrpc.la

libdrpc_la_SOURCES= \
	src/drpc.cpp \
	src/channel.cpp


libdrpc_la_LDFLAGS= \
	-version-info 0:1:0
