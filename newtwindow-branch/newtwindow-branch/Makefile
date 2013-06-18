# Main makefile of the project

INSTALLABLE_SUBDIRS = bin_rc doc man src hlp
SUBDIRS = $(INSTALLABLE_SUBDIRS)

DO_MAKE = @ for i in $(SUBDIRS); do $(MAKE) -C $$i $@ || exit; done
DO_IMAKE = @ for i in $(INSTALLABLE_SUBDIRS); do $(MAKE) -C $$i $@ || exit; done

all: $(SUBDIRS)

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
ifeq ($(filter 3.81,$(firstword $(sort $(MAKE_VERSION) \ 3.81))),)
$(error 'make' utility is too old. Required version is 3.81)
@exit 1
endif
	$(MAKE) -C $@

install:
	$(DO_IMAKE)

clean:
	$(DO_MAKE)

distclean:
	$(DO_MAKE)
	rm -f config.h config.mak configure.log version.h .depend

uninstall:
	$(DO_IMAKE)

dep:
	$(DO_MAKE)
