## Process this file with automake to produce Makefile.in

SUBDIRS = src data ui pixmaps

gbilling_clientdocdir = ${prefix}/doc/gbilling-client
gbilling_clientdoc_DATA = \
        README \
        COPYING \
        AUTHORS \
        ChangeLog \
        INSTALL \
        NEWS \
        TODO

gbilling_clientmenudir = ${prefix}/share/applications
gbilling_clientmenu_DATA = gbilling-client.desktop

EXTRA_DIST = $(gbilling_clientdoc_DATA) $(gbilling_clientmenu_DATA)
