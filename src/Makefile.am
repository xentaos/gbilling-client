## Process this file with automake to produce Makefile.in

INCLUDES = $(PACKAGE_CFLAGS)

AM_CFLAGS = -Wall -Wpointer-arith

# -DGTK_DISABLE_DEPRECATED=1 -DG_DISABLE_DEPRECATED=1

bin_PROGRAMS = gbilling-client

gbilling_client_SOURCES = \
	callback.c \
	gbilling.c \
	gui.c \
	sockc.c \
	setting.c \
	client.c \
	main.c

gbilling_client_LDFLAGS = -export-dynamic

gbilling_client_LDADD = $(PACKAGE_LIBS)
