# Makefile for pracc project

# The release version of pracc. Do not modify!
VERSION = 1.2.2

# ** Start Configuration:

# Use these variables to adjust pracc to your system.
# They will be used to generate the pracc.h header,
# which in turn is included by all pracc programs.

# Where the backend goes: Ubuntu/Gentoo/etc differ
BACKDIR = /usr/lib/cups/backend
#BACKDIR = /usr/libexec/cups/backend

# Owner for pracc files (root is fine):
PRACCOWNER = root
# Group for pracc files (dedicated group):
PRACCGROUP = lpadmin
# Name of the default account:
PRACCDEFLT = default

# The config file:
PRACCCONFIG = /etc/pracc.conf
# The accounting log file:
PRACCLOG = /var/print/pracc.log
# The pagecount log file:
PRACCPCLOG = /var/print/pc.log

# Where pracc files are stored:
PRACCDIR = /var/print/pracc
# Where pracc tools are installed:
PRACCBIN = /var/print/bin
# Where pracc documentation lives:
PRACCDOC = /var/print/doc
# Where the pracc.cgi is installed:
PRACCCGI = /var/print/cgi

# Admin group (can make changes; used by web ui):
PRACCPOKE = pracc-admin
# Observer group (read-only; used by web ui):
PRACCPEEK = pracc-watch

# ** End Configuration: no changes after here! **

vpath %.h mylib

# Remove symtab and reloc info (smaller binary); fussy warnings
CFLAGS = -s -Wall -Imylib

TOOLS = pracc-init pracc-edit pracc-view pracc-kill pracc-sum \
  pracc-purge pracc-check pracc-log pracc-pclog pracc-scan netprint

TESTS = config_test strbuf_test

all: backend tools webgui
backend: cupspracc
tools: $(TOOLS)
test: tests
tests: $(TESTS)
webgui: pracc.cgi

install: cupspracc pracc.cgi tools
	install -d -o $(PRACCOWNER) -g $(PRACCGROUP) $(PRACCDIR)
	install -d $(PRACCBIN)
	install -o root -g root -m 755 pracc-init $(PRACCBIN)
	install -o root -g root -m 755 pracc-edit $(PRACCBIN)
	install -o root -g root -m 755 pracc-view $(PRACCBIN)
	install -o root -g root -m 755 pracc-kill $(PRACCBIN)
	install -o root -g root -m 755 pracc-sum $(PRACCBIN)
	install -o root -g root -m 755 pracc-check $(PRACCBIN)
	install -o root -g root -m 755 pracc-log $(PRACCBIN)
	install -o root -g root -m 755 pracc-pclog $(PRACCBIN)
	install -o root -g root -m 755 pracc-scan $(PRACCBIN)
	install -o root -g root -m 755 netprint $(PRACCBIN)
	install -o root -g $(PRACCGROUP) -m 700 cupspracc $(BACKDIR)/pracc
	install -d $(PRACCCGI)
	install -o root -g root -m 755 pracc.cgi $(PRACCCGI)/pracc.cgi
	install -d $(PRACCDOC)
	cp -a ../doc/* $(PRACCDOC)
	cp -a tmpl $(PRACCDOC)
	chown -R root:root $(PRACCDOC)


# Generate the pracc.h header:

pracc.h: Makefile pracc.t
	sed -e 's:{VERSION}:$(VERSION):g' \
	    -e 's:{PRACCOWNER}:$(PRACCOWNER):g' \
	    -e 's:{PRACCGROUP}:$(PRACCGROUP):g' \
	    -e 's:{PRACCDEFLT}:$(PRACCDEFLT):g' \
	    -e 's:{PRACCCONFIG}:$(PRACCCONFIG):g' \
	    -e 's:{PRACCLOG}:$(PRACCLOG):g' \
	    -e 's:{PRACCPCLOG}:$(PRACCPCLOG):g' \
	    -e 's:{PRACCDIR}:$(PRACCDIR):g' \
	    -e 's:{PRACCBIN}:$(PRACCBIN):g' \
	    -e 's:{PRACCDOC}:$(PRACCDOC):g' \
	    -e 's:{PRACCCGI}:$(PRACCCGI):g' \
	    -e 's:{PRACCPEEK}:$(PRACCPEEK):g' \
	    -e 's:{PRACCPOKE}:$(PRACCPOKE):g' \
	    pracc.t > pracc.h


# In general, foo.o is built from foo.c as follows:

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)


# The CUPS backend:

cupspracc: cupspracc.o pjl.o ps.o writen.o pracclib.a mylib.a
	$(CC) -o $@ $+ -lcups
cupspracc.o: cupspracc.c pracc.h pjl.h ps.h print.h tai.h

pstest:	ps.c mylib.a
	$(CC) -D TESTING -o $@ $^ $(CFLAGS)

pjltest: pjl.c mylib.a
	$(CC) -D TESTING -o $@ $^ $(CFLAGS)


# Pracc tools:

netprint: netprint.o delay.o pjl.o ps.o praccIdentify.c writen.o mylib.a
netprint.o: netprint.c pracc.h pjl.h ps.h scan.h

pracc-scan: pracc-scan.o joblex.o pcl5.o pclxl.o printer.o papersize.o \
  praccIdentify.o config.o strbuf.o pracclib.a mylib.a
pracc-scan.o: pracc-scan.c pracc.h joblex.h printer.h papersize.h config.h

pracc-init: pracc-init.o pracclib.a mylib.a
pracc-init.o: pracc-init.c pracc.h print.h putln.h scan.h

pracc-edit: pracc-edit.o pracclib.a mylib.a
pracc-edit.o: pracc-edit.c pracc.h print.h putln.h scan.h

pracc-view: pracc-view.o pracclib.a mylib.a
pracc-view.o: pracc-view.c pracc.h print.h putln.h scan.h

pracc-kill: pracc-kill.o pracclib.a mylib.a
pracc-kill.o: pracc-kill.c pracc.h print.h putln.h

pracc-sum: pracc-sum.o pracclib.a mylib.a
pracc-sum.o: pracc-sum.c pracc.h print.h putln.h scan.h

pracc-purge: pracc-purge.o pracclib.a mylib.a
pracc-purge.o: pracc-purge.c pracc.h print.h putln.h scan.h

pracc-check: pracc-check.o pracclib.a mylib.a
pracc-check.o: pracc-check.c pracc.h print.h putln.h

pracc-log: pracc-log.o pracclib.a mylib.a
pracc-log.o: pracc-log.c pracc.h print.h putln.h scan.h

pracc-pclog: pracc-pclog.o pclog.o symtab.o pracclib.a mylib.a
pracc-pclog.o: pracc-pclog.c pclog.h pracc.h putln.h scan.h


# Pracc Web GUI

WEBDEPS = webgui.h pracc.h symtab.h cgi.h daterange.h
WEBLIBS = pracclib.a mylib.a
WEBOBJS = pracc.cgi.o subst.o symtab.o cgi.o daterange.o pclog.o \
  ui_acct.o ui_accts.o ui_pclog.o ui_pracclog.o ui_report.o

pracc.cgi: $(WEBOBJS) $(WEBLIBS)
pracc.cgi.o: pracc.cgi.c $(WEBDEPS)

accts: ui_accts.c pracc.h tai.h
	$(CC) -D TESTING -o $@ ui_accts.c pracclib.a mylib.a $(CFLAGS)

subst: subst.c symtab.c progname.c mylib/scani.c mylib/scanu.c
	$(CC) -D STANDALONE -o $@ $^ $(CFLAGS)

ui_acct.o: ui_acct.c webgui.h pracc.h
ui_accts.o: ui_accts.c webgui.h pracc.h tai.h
ui_pclog.o: ui_pclog.c webgui.h pracc.h getln.h pclog.h symtab.h tai.h
ui_pracclog.o: ui_pracclog.c webgui.h pracc.h
ui_report.o: ui_report.c webgui.h pracc.h symtab.h

daterange.o: daterange.c daterange.h scan.h
cgi.o: cgi.c cgi.h pracc.h
pclog.o: pclog.c getln.h pclog.h pracc.h scan.h symtab.h tai.h
subst.o: subst.c scan.h symtab.h
symtab.o: symtab.c symtab.h


# Pracc API:

pracclib.a: praccAppend.o praccAssemble.o praccCheckName.o praccCreate.o \
	praccFormat.o praccGrant.o praccLogRead.o \
	praccLogup.o praccIdentify.o \
	praccPath.o praccPurge.o praccRead.o praccSum.o \
	praccTypeString.o praccAccountInfo.o praccDelete.o \
	hasgroup.o progname.o die.o \
	mylib/taifmt.o mylib/tainow.o mylib/taiscan.o mylib/taistore.o
	rm -f $@
	ar cr $@ $+
	-ranlib $@

praccAppend.o: praccAppend.c pracc.h
praccCheckName.o: praccCheckName.c pracc.h
praccCreate.o: praccCreate.c pracc.h print.h
praccDelete.o: praccDelete.c pracc.h
praccAssemble.o: praccAssemble.c pracc.h print.h tai.h
praccFormat.o: praccFormat.c pracc.h
praccGrant.o: praccGrant.c pracc.h
praccLogRead.o: praccLogRead.c pracc.h getln.h scan.h tai.h
praccLogup.o: praccLogup.c pracc.h print.h
praccIdentify.o: praccIdentify.c pracc.h
praccPath.o: praccPath.c pracc.h
praccPurge.o: praccPurge.c pracc.h print.h
praccRead.o: praccRead.c pracc.h getln.h tai.h
praccSum.o: praccSum.c pracc.h
praccTypeString.o: praccTypeString.c pracc.h
praccAccountInfo.o: praccAccountInfo.c pracc.h


# joblex:

joblex.c: joblex.l
	lex -t joblex.l > joblex.c
	
joblex.o: joblex.c

joblex: joblex.o pcl5.o pclxl.o printer.o papersize.o
	$(CC) -o $@ $^ -lm


# Unit tests:

config_test: config_test.c config.o strbuf.o mylib.a
	$(CC) -g -o $@ $^ $(CFLAGS)
	@./config_test || echo "Unit test for config FAILED"

strbuf_test: strbuf_test.c strbuf.o
	$(CC) -g -o $@ $^ $(CFLAGS)
	@./strbuf_test || echo "Unit test for strbuf FAILED"


# Utilities:

progname.o: progname.c
die.o: die.c
strbuf.o: strbuf.c
config.o: config.c

delay.o: delay.c
hasgroup.o: hasgroup.c
writen.o: writen.c

pcl5.o: pcl5.c pcl5.h printer.h
pclxl.o: pclxl.c pclxl.h printer.h
printer.o: printer.c printer.h papersize.h
papersize.o: papersize.c papersize.h


# Library

LIBDIR = mylib
LIBDEPS = getln.h print.h putln.h scan.h tai.h
LIBDEPS := $(patsubst %,$(LIBDIR)/%,$(LIBDEPS))
LIBOBJS = getln.o putln.o putfmt.o putbuf.o \
  scani.o scanu.o scanpat.o scandate.o scantime.o scanip4op.o \
  printi.o printu.o prints.o printsn.o printstm.o
LIBOBJS := $(patsubst %,$(LIBDIR)/%,$(LIBOBJS))

mylib.a: $(LIBOBJS)
	rm -f $@
	ar cr $@ $+
	-ranlib $@

$(LIBDIR)/%.o: $(LIBDIR)/%.c $(LIBDEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


# Administration

clean:
	rm -f cupspracc $(TOOLS) $(TESTS) pracc.cgi *.o *.a mylib/*.o

tgz: clean
	(cd ..; tar chzvf pracc-`date +%Y%m%d`.tgz --exclude test pracc)

dist: clean
	(cd ../..; tar chzvf pracc-$(VERSION).tgz \
	 --exclude .git --exclude RCS --exclude legacy --exclude test pracc)
#	(V=`head -1 VERSION`; cd ..; tar chzvf pracc-v$$V.tgz \
#	 --exclude RCS --exclude legacy --exclude test pracc)

.PHONY: all backend tools webgui test tests install clean tgz dist
