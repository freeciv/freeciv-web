## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libtoolsshared.la

AM_CPPFLAGS =                                                              \
        -I$(top_srcdir)/utility                                            \
        -I$(top_srcdir)/common                                             \
        -I$(top_srcdir)/common/networking                                  \
        -I$(top_srcdir)/server                                             \
        -I$(top_srcdir)/dependencies/tinycthread

libtoolsshared_la_SOURCES =                                                \
                tools_fc_interface.c                                       \
                tools_fc_interface.h
