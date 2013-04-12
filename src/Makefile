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

SUBDIRS=libbeye

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
LIBS = -L./libbeye -lbeye $(EXTRALIBS)
SRCS = \
addendum.cpp\
bconsole.cpp\
beyehelp.cpp\
beyeutil.cpp\
bin_util.cpp\
bmfile.cpp\
codeguid.cpp\
colorset.cpp\
dialogs.cpp\
editors.cpp\
events.cpp\
fileutil.cpp\
info_win.cpp\
main.cpp\
mainloop.cpp\
refs.cpp\
search.cpp\
setup.cpp\
sysinfo.cpp\
tstrings.cpp\
addons/sys/ascii.cpp\
addons/sys/consinfo.cpp\
addons/sys/cpu_perf.cpp\
addons/sys/inview.cpp\
addons/tools/dig_conv.cpp\
addons/tools/eval.cpp\
plugins/bin/aout.cpp\
plugins/bin/arch.cpp\
plugins/bin/asf.cpp\
plugins/bin/avi.cpp\
plugins/bin/bin.cpp\
plugins/bin/bmp.cpp\
plugins/bin/coff386.cpp\
plugins/bin/dos_sys.cpp\
plugins/bin/elf386.cpp\
plugins/bin/jpeg.cpp\
plugins/bin/jvmclass.cpp\
plugins/bin/le.cpp\
plugins/bin/lmf.cpp\
plugins/bin/lx.cpp\
plugins/bin/mov.cpp\
plugins/bin/mp3.cpp\
plugins/bin/mpeg.cpp\
plugins/bin/mz.cpp\
plugins/bin/ne.cpp\
plugins/bin/nlm386.cpp\
plugins/bin/opharlap.cpp\
plugins/bin/pe.cpp\
plugins/bin/pharlap.cpp\
plugins/bin/rdoff.cpp\
plugins/bin/rdoff2.cpp\
plugins/bin/realmedia.cpp\
plugins/bin/sis.cpp\
plugins/bin/sisx.cpp\
plugins/bin/wav.cpp\
plugins/binmode.cpp\
plugins/disasm.cpp\
plugins/hexmode.cpp\
plugins/textmode.cpp\
plugins/nls/russian.cpp\
plugins/disasm/null_da.cpp\
plugins/disasm/arm/arm.cpp\
plugins/disasm/arm/arm16.cpp\
plugins/disasm/arm/arm32.cpp\
plugins/disasm/avr/avr.cpp\
plugins/disasm/ppc/ppc.cpp\
plugins/disasm/java/java.cpp\
plugins/disasm/ix86/ix86.cpp\
plugins/disasm/ix86/ix86_fpu.cpp\
plugins/disasm/ix86/ix86_fun.cpp

OBJS=$(SRCS:.cpp=.o)

HLP_SUBDIRS=tools/beyehlp tools/lzss
DO_HELP = @ for i in $(HLP_SUBDIRS); do $(MAKE) -C $$i $@ || exit; done

all:     $(SUBDIRS) $(TARGET)

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

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

$(TARGET): $(OBJS)
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
