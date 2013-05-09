include ../../config.mak

CXXFLAGS = -I.. $(HOST_CFLAGS)
LDXXFLAGS = $(HOST_LDFLAGS)

LIBNAME = $(LIBPREFIX)beye.a

SRCS= bbio.cpp \
      bstream.cpp \
      file_ini.cpp \
      libbeye.cpp \
      mmfile.cpp \
      mp_malloc.cpp \
      tw_class.cpp \
      twindow.cpp \
      sysdep/$(MACHINE)/aclib.cpp \
      sysdep/$(MACHINE)/cpu_info.cpp \
      osdep/$(HOST)/keyboard.cpp \
      osdep/$(HOST)/mouse.cpp \
      osdep/$(HOST)/misc.cpp \
      osdep/$(HOST)/nls.cpp \
      osdep/$(HOST)/os_dep.cpp \
      osdep/$(HOST)/timer.cpp \
      osdep/$(HOST)/vio.cpp

ifeq ($(HOST),qnx)
ASMS = \
      osdep/$(HOST)/3rto3s.asm\
      osdep/$(HOST)/3sto3r.asm
endif

OBJS=$(SRCS:.cpp=.o)
AOBJS=$(ASMS:.asm=.o)

.SUFFIXES: .asm .c .o

# .PHONY: all clean

.cpp.o:
	$(CXX) -c $(CXXFLAGS) -o $@ $<
.asm.o:
	$(CC) -c -o $@ $<

$(LIBNAME):     $(OBJS) $(AOBJS)
	$(AR) $(LIBNAME) $(OBJS) $(AOBJS)

all:    $(LIBNAME)

clean:
	rm -f $(OBJS) *.a *~

distclean: clean
	rm -f Makefile.bak .depend

dep:    depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif

