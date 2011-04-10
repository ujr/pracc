# Makefile for pracc project
# $Id: Makefile,v 1.6 2008/04/14 19:16:38 ujr Exp ujr $

# The release version of pracc. Do not modify!
VERSION = 1.2.0

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

# Admin group (can make changes):
PRACCPOKE = pracc-admin
# Observer group (read-only):
PRACCPEEK = pracc-watch

# ** End Configuration: no changes after here! **

# Remove symtab and reloc info (smaller binary)
CFLAGS = -s

TOOLS = pracc-init pracc-edit pracc-view pracc-kill \
 pracc-sum pracc-purge pracc-check pracc-log

all: backend tools gui
backend: cupspracc
tools: $(TOOLS)
gui: pracc.cgi

install: cupspracc pracc.cgi tools
	install -d $(PRACCBIN) $(PRACCDOC) $(PRACCCGI)
	install -o root -g root -m 755 pracc-init $(PRACCBIN)
	install -o root -g root -m 755 pracc-edit $(PRACCBIN)
	install -o root -g root -m 755 pracc-view $(PRACCBIN)
	install -o root -g root -m 755 pracc-kill $(PRACCBIN)
	install -o root -g root -m 755 pracc-sum $(PRACCBIN)
	install -o root -g root -m 755 pracc-check $(PRACCBIN)
	install -o root -g root -m 755 pracc-log $(PRACCBIN)
	install -o root -g $(PRACCGROUP) -m 700 cupspracc $(BACKDIR)/pracc
	install -o root -g root -m 755 pracc.cgi $(PRACCCGI)/pracc.cgi
	cp -a doc/* $(PRACCDOC)
	chown -R root:root $(PRACCDOC)

# Generate the pracc.h header:

pracc.h: Makefile subst
	./subst "VERSION=$(VERSION)" \
	 "PRACCOWNER=$(PRACCOWNER)" \
	 "PRACCGROUP=$(PRACCGROUP)" \
	 "PRACCDEFLT=$(PRACCDEFLT)" \
	 "PRACCLOG=$(PRACCLOG)" \
	 "PRACCPCLOG=$(PRACCPCLOG)" \
	 "PRACCDIR=$(PRACCDIR)" \
	 "PRACCBIN=$(PRACCBIN)" \
	 "PRACCDOC=$(PRACCDOC)" \
	 "PRACCCGI=$(PRACCCGI)" \
	 "PRACCPEEK=$(PRACCPEEK)" \
	 "PRACCPOKE=$(PRACCPOKE)" \
	 pracc.t > pracc.h

# The CUPS backend:

cupspracc: cupspracc.o pjl.o ps.o scans.o printsn.o pracclib.a
	gcc -lcups -o $@ $+
cupspracc.o: cupspracc.c pracc.h pjl.h ps.h print.h tai.h

pstest:	ps.o
	gcc -D TESTING -o pstest ps.c scani.c scanu.c
ps.o: ps.c ps.h

pjltest: pjl.o
	gcc -D TESTING -o pjltest pjl.c scani.c scanu.c
pjl.o: pjl.c pjl.h

# Pracc tools:

pracc-print: pracc-print.o delay.o pjl.o ps.o scanip4op.c scani.c scanu.c
pracc-print.o: pracc-print.c delay.h pjl.h ps.h scan.h

pracc-init: pracc-init.o common.a pracclib.a
pracc-init.o: pracc-init.c common.h pracc.h print.h scan.h streq.h

pracc-edit: pracc-edit.o common.a pracclib.a
pracc-edit.o: pracc-edit.c common.h pracc.h print.h scan.h streq.h

pracc-view: pracc-view.o printstm.o tailocal.o common.a pracclib.a
pracc-view.o: pracc-view.c common.h pracc.h scan.h streq.h

pracc-kill: pracc-kill.o common.a pracclib.a
pracc-kill.o: pracc-kill.c common.h pracc.h print.h

pracc-sum: pracc-sum.o tailocal.o common.a pracclib.a
pracc-sum.o: pracc-sum.c common.h pracc.h print.h scan.h tai.h

pracc-purge: pracc-purge.o common.a pracclib.a
pracc-purge.o: pracc-purge.c common.h pracc.h print.h scan.h tai.h

pracc-check: pracc-check.o printsn.o common.a pracclib.a
pracc-check.o: pracc-check.c common.h pracc.h print.h streq.h

pracc-log: pracc-log.o printstm.o tailocal.o common.a pracclib.a
pracc-log.o: pracc-log.c common.h pracc.h scan.h streq.h tai.h

# Pracc Web GUI

pracc.cgi.o: pracc.cgi.c pracc.h symtab.h cgi.h datetools.h \
        ui_acct.h ui_accts.h ui_pclog.h ui_pracclog.h ui_report.h
pracc.cgi: pracc.cgi.o subst.o symtab.o cgi.o datetools.c common.a \
        ui_acct.o ui_accts.o ui_pclog.o ui_pracclog.o ui_report.o \
        tailocal.o pracclib.a

# Subst MUST NOT depend on pracc.h, not even transitively!
subst: subst.c symtab.h scan.h
	cc -D STANDALONE -D VERSION='"$(VERSION)"' -o $@ \
	subst.c symtab.c progname.c scani.c scanu.c

pclog: ui_pclog.c symtab.h symtab.c pracc.h common.h getln.h tai.h
	cc -D TESTING -o $@ ui_pclog.c symtab.c getln.c scani.c scanu.c \
	scandate.c taiscan.c taifmt.c taistore.c tailocal.c common.a

accts: ui_accts.c pracc.h tai.h
	cc -D TESTING -o $@ ui_accts.c pracclib.a common.a

ui_acct.o: ui_acct.c ui_acct.h pracc.h tai.h
ui_accts.o: ui_accts.c ui_accts.h pracc.h tai.h
ui_pclog.o: ui_pclog.c ui_pclog.h getln.h pracc.h symtab.h tai.h
ui_pracclog.o: ui_pracclog.c ui_pracclog.h pracc.h tai.h
ui_report.o: ui_report.c ui_report.h pracc.h symtab.h

datetools.o: datetools.c datetools.h scan.h
cgi.o: cgi.c cgi.h pracc.h
subst.o: subst.c pracc.h scan.h symtab.h
symtab.o: symtab.c symtab.h

# Pracc API:

pracclib: pracclib.a
pracclib.a: praccAppend.o praccAssemble.o praccCheckName.o praccCreate.o \
	praccFormatInfo.o praccFormatName.o praccGrant.o praccLogRead.o \
	praccLogup.o praccIdentify.o \
	praccPath.o praccPurge.o praccRead.o praccSum.o \
	praccTypeString.o praccAccountInfo.o praccDelete.o \
	hasgroup.o getln.o putln.o \
	taifmt.o tainow.o taiscan.o taistore.o \
	scandate.o scani.o scanpat.o scanu.o \
	prints.o printu.o printi.o
	rm -f $@
	ar cr $@ $+
	ranlib $@

praccAppend.o: praccAppend.c pracc.h
praccCheckName.o: praccCheckName.c pracc.h
praccCreate.o: praccCreate.c pracc.h print.h
praccDelete.o: praccDelete.c pracc.h
praccAssemble.o: praccAssemble.c pracc.h print.h tai.h
praccFormatInfo.o: praccFormatInfo.c pracc.h
praccFormatName.o: praccFormatName.c pracc.h
praccGrant.o: praccGrant.c pracc.h hasgroup.h
praccLogRead.o: praccLogRead.c pracc.h getln.h scan.h tai.h
praccLogup.o: praccLogup.c pracc.h print.h
praccIdentify.o: praccIdentify.c pracc.h
praccPath.o: praccPath.c pracc.h
praccPurge.o: praccPurge.c pracc.h print.h
praccRead.o: praccRead.c pracc.h getln.h tai.h
praccSum.o: praccSum.c pracc.h
praccTypeString.o: praccTypeString.c pracc.h
praccAccountInfo.o: praccAccountInfo.c pracc.h

# Utilities:

common.a: putln.o putfmt.o putbuf.o die.o progname.o
	rm -f $@
	ar cr $@ $+
	ranlib $@

progname.o: progname.c
die.o: die.c
getln.o: getln.c getln.h
putln.o: putln.c
putfmt.o: putfmt.c
putbuf.o: putbuf.c

delay.o: delay.c delay.h
hasgroup.o: hasgroup.c hasgroup.h

prints.o: prints.c print.h
printu.o: printu.c print.h
printi.o: printi.c print.h
printsn.o: printsn.c print.h
printstm.o: printstm.c print.h

scans.o: scans.c scan.h
scanu.o: scanu.c scan.h
scani.o: scani.c scan.h
scanpat.o: scanpat.c scan.h
scandate.o: scandate.c scan.h

taifmt.o: taifmt.c tai.h
taiload.o: taiload.c tai.h
tailocal.o: tailocal.c tai.h
tainow.o: tainow.c tai.h
taiscan.o: taiscan.c tai.h
taistore.o: taistore.c tai.h

# Administration

clean:
	rm -f cupspracc $(TOOLS) pracc.cgi subst *.o common.a pracclib.a

tgz: clean
	(cd ..; tar chzvf pracc-`date +%Y%m%d`.tgz pracc)

dist: clean
	(cd ..; tar chzvf pracc-$(VERSION).tgz \
	 --exclude .git --exclude RCS --exclude legacy --exclude test pracc)
#	(V=`head -1 VERSION`; cd ..; tar chzvf pracc-v$$V.tgz \
#	 --exclude RCS --exclude legacy --exclude test pracc)

## Conversion from old pracc files to the current (2008) format
#
#conv:	conv.c pracc.h getln.h tai.h
#	cc -o conv conv.c oldscan.c getln.c scani.c scanu.c scans.c \
#	scanpat.c scantime.c scandate.c taiscan.c taifmt.c taistore.c \
#	praccAssemble.c praccFormatName.c praccFormatInfo.c printu.c printi.c

