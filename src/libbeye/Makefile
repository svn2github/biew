include ../../config.mak

CXXFLAGS = -I.. $(HOST_CFLAGS)
LDXXFLAGS = $(HOST_LDFLAGS)

DO_MAKE = @ for i in $(SUBDIRS); do $(MAKE) -C $$i $@ || exit; done
DO_ALL  = @ for i in $(SUBDIRS); do $(MAKE) -C $$i all || exit; done
SUBDIRS = osdep

LIBNAME = $(LIBPREFIX)beye.a

SRCS= bbio.cpp \
      bstream.cpp \
      file_ini.cpp \
      libbeye.cpp \
      mmfile.cpp \
      mp_malloc.cpp \
      tw_class.cpp \
      twindow.cpp \
      sysdep/$(MACHINE)/cpu_info.cpp

OBJS=$(SRCS:.cpp=.o)

.SUFFIXES: .cpp .o

.PHONY: $(SUBDIRS)

all:    $(LIBNAME) $(SUBDIRS)

$(SUBDIRS):
	$(DO_ALL)

.cpp.o:
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(LIBNAME):     $(OBJS)
	$(AR) $(LIBNAME) $(OBJS)

clean:
	$(DO_MAKE)
	rm -f $(OBJS) *.a *~

distclean: clean
	rm -f Makefile.bak .depend
	$(DO_MAKE)

dep:    .depend

.depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif

