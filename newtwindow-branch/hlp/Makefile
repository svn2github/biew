# Main makefile of the project

include ../config.mak

sep := $(if $(filter win32 os2 dos, $(HOST)),\\,/)
SUBDIRS = lzss beyehlp

DO_MAKE = @ for i in $(SUBDIRS); do $(MAKE) -C $$i $@ || exit; done
DO_IMAKE = @ for i in $(INSTALLABLE_SUBDIRS); do $(MAKE) -C $$i $@ || exit; done

all: $(SUBDIRS) help

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
ifeq ($(filter 3.81,$(firstword $(sort $(MAKE_VERSION) \ 3.81))),)
$(error 'make' utility is too old. Required version is 3.81)
@exit 1
endif
	$(MAKE) -C $@

install:
	$(INSTALL) -D -b -c -m 644 beye.hlp $(DESTDIR)$(DATADIR)/beye.hlp
	$(DO_IMAKE)

clean:
	$(DO_MAKE)
	rm -f beye.hlp

distclean:
	$(DO_MAKE)
	rm -f beye.hlp
	rm -f config.h config.mak

uninstall:
	rm -f $(DESTDIR)$(DATADIR)/*
	$(DO_IMAKE)

dep:
	$(DO_MAKE)

help:
	(.$(sep)beyehlp$(sep)beyehlp$(EXESUF) .$(sep)lzss$(sep)lzss$(EXESUF) beyehlp.prj)
