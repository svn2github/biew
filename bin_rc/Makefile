# a BSD compatible 'install' program
INSTALL = install

include ../config.mak

all:
clean:
distclean:
	echo "nothing to do"
install:
	$(INSTALL) -D -b -c -m 644 beye.hlp $(DESTDIR)$(DATADIR)/beye.hlp
uninstall:
	rm -f $(DESTDIR)$(DATADIR)/*
	rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(DATADIR)
dep:
