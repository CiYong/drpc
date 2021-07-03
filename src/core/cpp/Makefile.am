
ACLOCAL_AMFLAGS= -I m4

LIBS = -lpthread -lzmq -luuid

AM_CPPFLAGS= \
	-I./include \
	-I./src \
        -std=gnu++11

lib_LTLIBRARIES=libdrpc.la

include_HEADERS = \
        include/drpc.hpp

libdrpc_la_SOURCES= \
	src/drpc.cpp \
	src/channel.cpp \
        src/handler.cpp

libdrpc_la_LDFLAGS= \
	-version-info 0:1:0
