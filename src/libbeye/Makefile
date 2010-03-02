include ../../config.mak

CFLAGS = -I.. $(HOST_CFLAGS)
LDFLAGS = $(OSLDEF) $(HOST_LDFLAGS)

LIBNAME = $(LIBPREFIX)beye.$(LIBEXT)

SRCS= bbio.c \
      file_ini.c \
      libbeye.c \
      pmalloc.c \
      tw_class.c \
      twin.c \
      sysdep/$(MACHINE)/aclib.c \
      sysdep/$(MACHINE)/cpu_info.c \
      osdep/$(HOST)/fileio.c \
      osdep/$(HOST)/keyboard.c \
      osdep/$(HOST)/mmfio.c \
      osdep/$(HOST)/mouse.c \
      osdep/$(HOST)/misc.c \
      osdep/$(HOST)/nls.c \
      osdep/$(HOST)/os_dep.c \
      osdep/$(HOST)/timer.c \
      osdep/$(HOST)/vio.c


ifeq ($(HOST),qnx)
ASMS = \
      osdep/$(HOST)/3rto3s.asm\
      osdep/$(HOST)/3sto3r.asm
endif

OBJS=$(SRCS:.c=.o)
AOBJS=$(ASMS:.asm=.o)

.SUFFIXES: .asm .c .o

# .PHONY: all clean

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<
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

