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
# All rights reserved. This software is redistributable under the licence    #
# This software is redistributable under the licence given in the file       #
# "Licence.en" ("Licence.ru" in russian translation) distributed in the BEYE #
##############################################################################

##############################################################################
#  Main configure section of this makefile                                   #
##############################################################################
include ../../config.mak

CXXFLAGS = $(HOST_CFLAGS)
LDXXFLAGS = $(HOST_LDFLAGS)

###########################################################################
# TARGET: put name of executable image here                               #
###########################################################################
TARGET = lzss

##########################################################################
#                Please not modify contents below                        #
##########################################################################

INCS = -I. -I../../src
LIBS = -L../../src/libbeye -lbeye $(EXTRALIBS)
OBJS = lzss.o
all: $(TARGET)
help: $(TARGET)

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)$(EXESUF)

distclean: clean

cleansys:
	$(RM) lib/sysdep/$(MACHINE)/$(HOST)/*.o

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(INCS) -c $< -o $@

lzss.o:                        lzss.cpp
