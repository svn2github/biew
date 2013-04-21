include ../../../config.mak

CXXSRCS =\
    aout.cpp\
    arch.cpp\
    asf.cpp\
    avi.cpp\
    bin.cpp\
    bmp.cpp\
    coff386.cpp\
    dos_sys.cpp\
    elf386.cpp\
    jpeg.cpp\
    jvmclass.cpp\
    le.cpp\
    lmf.cpp\
    lx.cpp\
    mov.cpp\
    mp3.cpp\
    mpeg.cpp\
    mz.cpp\
    ne.cpp\
    nlm386.cpp\
    opharlap.cpp\
    pe.cpp\
    pharlap.cpp\
    rdoff.cpp\
    rdoff2.cpp\
    realmedia.cpp\
    sis.cpp\
    sisx.cpp\
    wav.cpp

SRCS    =
OBJS	= $(SRCS:.c=.o)
CXXOBJS	= $(CXXSRCS:.cpp=.o)

CXXFLAGS = -I.. -I../.. $(HOST_CFLAGS)
LDXXFLAGS = $(HOST_LDFLAGS)

.SUFFIXES: .c .o

# .PHONY: all clean

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<
.cpp.o:
	$(CC) -c $(CXXFLAGS) -o $@ $<

all:	dep $(OBJS) $(CXXOBJS)

clean:
	rm -f *.o *.a *~

distclean:
	rm -f test *.o $(LIBNAME) *~ .depend

dep:    .depend

.depend: $(SRCS) $(CXXSRCS)
	$(CC) -MM $(CXXFLAGS) $(SRCS) $(CXXSRCS) 1>.depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
