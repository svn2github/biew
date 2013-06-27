include ../../../config.mak

CXXFLAGS = -I.. -I../.. $(HOST_CFLAGS)
LDXXFLAGS = $(HOST_LDFLAGS)

LIBNAME = $(LIBPREFIX)osdep.a

SRCS= system.cpp \
      tconsole.cpp \
      $(HOST)/nls.cpp \
      $(HOST)/os_dep.cpp \
      $(HOST)/timer.cpp

ifeq ($(HOST),dos)
SRCS+=input_dos.cpp
SRCS+=vio_dos.cpp
endif
ifeq ($(HOST),dos4gw)
SRCS+=input_dos.cpp
SRCS+=vio_dos4gw.cpp
endif
ifeq ($(HOST),linux)
SRCS+=input_linux.cpp
SRCS+=vio_linux.cpp
endif
ifeq ($(HOST),os2)
SRCS+=input_os2.cpp
SRCS+=vio_os2.cpp
endif
ifeq ($(HOST),qnx)
SRCS+=input_qnx.cpp
SRCS+=vio_qnx.cpp
endif
ifeq ($(HOST),qnxto)
SRCS+=input_qnxto.cpp
SRCS+=vio_qnxto.cpp
endif
ifeq ($(HOST),win32)
SRCS+=input_win32.cpp
SRCS+=vio_win32.cpp
endif
SRCS+=input_unix.cpp
SRCS+=input_null.cpp
SRCS+=vio_vt100.cpp
SRCS+=vio_null.cpp

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

