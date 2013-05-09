include ../../../config.mak

CXXFLAGS = -I.. -I../.. $(HOST_CFLAGS)
LDXXFLAGS = $(HOST_LDFLAGS)

LIBNAME = $(LIBPREFIX)osdep.a

SRCS= system.cpp \
      tconsole.cpp \
      $(HOST)/keyboard.cpp \
      $(HOST)/mouse.cpp \
      $(HOST)/misc.cpp \
      $(HOST)/nls.cpp \
      $(HOST)/os_dep.cpp \
      $(HOST)/timer.cpp \
      $(HOST)/vio.cpp

ifeq ($(HOST),qnx)
ASMS = \
      $(HOST)/3rto3s.asm\
      $(HOST)/3sto3r.asm
endif

OBJS=$(SRCS:.cpp=.o)
AOBJS=$(ASMS:.asm=.o)

.SUFFIXES: .asm .c .o

# .PHONY: all clean

all:    $(LIBNAME)

.cpp.o:
	$(CXX) -c $(CXXFLAGS) -o $@ $<
.asm.o:
	$(CC) -c -o $@ $<

$(LIBNAME):     $(OBJS) $(AOBJS)
	$(AR) $(LIBNAME) $(OBJS) $(AOBJS)


clean:
	rm -f $(OBJS) *.a *~

distclean: clean
	rm -f Makefile.bak .depend

dep:    depend

depend:
	$(CXX) -MM $(CXXFLAGS) $(SRCS) 1>.depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif

