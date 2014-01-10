# a BSD compatible 'install' program
INSTALL = install

include ../config.mak

all:
clean:
distclean:
	echo "nothing to do"
install:
	mkdir --parents $(DESTDIR)$(DOCDIR)
	mkdir --parents $(DESTDIR)$(DOCDIR)/ru
	$(INSTALL) -D -c -m 644 beye.txt $(DESTDIR)$(DOCDIR)/beye.txt
	$(INSTALL) -D -c -m 644 ru/beye.txt $(DESTDIR)$(DOCDIR)/ru/beye.txt
	$(INSTALL) -D -c -m 644 unix.txt $(DESTDIR)$(DOCDIR)/unix.txt
	$(INSTALL) -D -c -m 644 release.txt $(DESTDIR)$(DOCDIR)/release.txt
uninstall:
	$(RM) $(DESTDIR)$(DOCDIR)/ru/*
	rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(DOCDIR)/ru
	$(RM) $(DESTDIR)$(DOCDIR)/*
	rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(DOCDIR)
dep:
