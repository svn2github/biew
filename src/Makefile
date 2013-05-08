##############################################################################
#   MAKEFILE - this file is part of Binary EYE project (BEYE)                #
##############################################################################
#   Copyrights:           1998, 2000 Nickols_K                               #
#   License:              See below                                          #
#   Author and developer: Nickols_K                                          #
#   Requirement:          GNU make                                           #
#   Original file name:   makefile                                           #
####################### [ D e s c r i p t i o n ] ############################
#  This file is script for make utility of GNU development system.           #
########################### [ L i c e n c e ] ################################
# The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.                     #
# All rights reserved.                                                       #
# This software is redistributable under the licence given in the file       #
# "Licence" distributed in the BEYE archive.                                 #
##############################################################################
include ../config.mak

SUBDIRS=addons plugins libbeye

DO_MAKE = @ for i in $(SUBDIRS); do $(MAKE) -C $$i $@ || exit; done
DO_ALL  = @ for i in $(SUBDIRS); do $(MAKE) -C $$i all || exit; done

CXXFLAGS = -I../hlp $(HOST_CFLAGS)
LDXXFLAGS = $(HOST_LDFLAGS)

###########################################################################
# TARGET: put name of executable image here                               #
###########################################################################
TARGET = $(PROGNAME)
BEYELIB = ./libbeye/$(LIBPREFIX)beye.$(LIBEXT)
##########################################################################
#                Please not modify contents below                        #
##########################################################################

INCS = -I.
LIBS = addons/libaddons.a \
       plugins/libplugins.a \
       plugins/disasm/libdisasm.a \
       libbeye/libbeye.a
LIBS += $(EXTRALIBS)
SRCS = \
bconsole.cpp\
beyehelp.cpp\
beyeutil.cpp\
bin_util.cpp\
codeguid.cpp\
colorset.cpp\
dialogs.cpp\
editors.cpp\
events.cpp\
fileutil.cpp\
info_win.cpp\
main.cpp\
mainloop.cpp\
search.cpp\
setup.cpp\
tstrings.cpp

OBJS=$(SRCS:.cpp=.o)

HLP_SUBDIRS=tools/beyehlp tools/lzss
DO_HELP = @ for i in $(HLP_SUBDIRS); do $(MAKE) -C $$i $@ || exit; done

all:     $(LIBS) $(OBJS) $(TARGET)

$(LIBS): $(SUBDIRS)

.PHONY: all $(SUBDIRS)

$(SUBDIRS):
ifeq ($(filter 3.81,$(firstword $(sort $(MAKE_VERSION) \ 3.81))),)
$(error 'make' utility is too old. Required version is 3.81)
@exit 1
endif
	$(MAKE) -C $@

clean:
	$(DO_MAKE)
	$(RM) $(OBJS)
	$(RM) $(TARGET)$(EXESUF)

distclean: clean
	$(DO_MAKE)

$(TARGET): $(OBJS) $(LIBS)
	$(CXX) $(LDXXFLAGS) -o $@ $(OBJS) $(LIBS)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@

install:
ifeq ($(INSTALL),)
	@echo "*** 'install' utility was not found and you can't run automatic"
	@echo "*** installation. Please download 'fileutils' from ftp://ftp.gnu.org and"
	@echo "*** install them to have possibility perform autiomatic installation"
	@echo "*** of this project"
	@exit 1
endif
	$(INSTALL) -D -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
