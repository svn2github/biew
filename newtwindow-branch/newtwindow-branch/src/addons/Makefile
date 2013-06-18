include ../../config.mak

CXXFLAGS = -I.. $(HOST_CFLAGS)
LDXXFLAGS = $(HOST_LDFLAGS)

DO_MAKE = @ for i in $(SUBDIRS); do $(MAKE) -C $$i $@ || exit; done
DO_ALL  = @ for i in $(SUBDIRS); do $(MAKE) -C $$i all || exit; done

LIBNAME = $(LIBPREFIX)addons.a
SUBDIRS = sysinfo tools

SRCS=addendum.cpp sysinfo.cpp

OBJS=$(SRCS:.cpp=.o)

.SUFFIXES: .cpp .o

.PHONY: $(SUBDIRS)

all:    dep $(LIBNAME)

$(SUBDIRS):
	$(DO_ALL)

# .PHONY: all clean

.cpp.o:
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(LIBNAME): $(SUBDIRS) $(OBJS)
	$(AR) $(LIBNAME) $(OBJS) \
	$(wildcard sysinfo/*.o) \
	$(wildcard tools/*.o)

clean:
	$(DO_MAKE)
	rm -f $(OBJS) *.a *~

distclean: clean
	$(DO_MAKE)
	rm -f Makefile.bak .depend

dep:    .depend

.depend:
	$(DO_MAKE)
	$(CC) -MM $(CXXFLAGS) $(SRCS) 1>.depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif

