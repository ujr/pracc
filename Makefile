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

# Admin group (can make changes):
PRACCPOKE = pracc-admin
# Observer group (read-only):
PRACCPEEK = pracc-watch

# ** End Configuration: no changes after here! **

# Remove symtab and reloc info (smaller binary)
CFLAGS = -s

TOOLS = pracc-init pracc-edit pracc-view pracc-kill pracc-sum \
 pracc-purge pracc-check pracc-log pracc-pclog pracc-scan netprint

TESTS = config_test strbuf_test

all: backend tools gui
backend: cupspracc
tools: $(TOOLS)
tests: $(TESTS)
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
	install -o root -g root -m 755 pracc-pclog $(PRACCBIN)
	install -o root -g root -m 755 pracc-scan $(PRACCBIN)
	install -o root -g root -m 755 netprint $(PRACCBIN)
	install -o root -g $(PRACCGROUP) -m 700 cupspracc $(BACKDIR)/pracc
	install -o root -g root -m 755 pracc.cgi $(PRACCCGI)/pracc.cgi
	cp -a doc/* $(PRACCDOC)
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

# The CUPS backend:

cupspracc: cupspracc.o pjl.o ps.o printsn.o pracclib.a writen.o
	gcc -lcups -o $@ $+
cupspracc.o: cupspracc.c pracc.h pjl.h ps.h print.h tai.h writen.h

pstest:	ps.o
	gcc -D TESTING -o pstest ps.c scani.c scanu.c
ps.o: ps.c ps.h

pjltest: pjl.o
	gcc -D TESTING -o pjltest pjl.c scani.c scanu.c
pjl.o: pjl.c pjl.h

# Pracc tools:

netprint: netprint.o delay.o pjl.o ps.o scanip4op.c scani.o scanu.o \
	praccIdentify.c writen.o
netprint.o: netprint.c delay.h pjl.h ps.h scan.h writen.h

pracc-scan: pracc-scan.o joblex.o pcl5.o pclxl.o papersize.o common.a \
	praccIdentify.o config.o strbuf.o scani.o scanu.o
pracc-scan.o: pracc-scan.c joblex.h pcl5.h pclxl.h papersize.h pracc.h common.h config.h

pracc-init: pracc-init.o common.a pracclib.a
pracc-init.o: pracc-init.c common.h pracc.h print.h scan.h

pracc-edit: pracc-edit.o common.a pracclib.a
pracc-edit.o: pracc-edit.c common.h pracc.h print.h scan.h

pracc-view: pracc-view.o printstm.o tailocal.o common.a pracclib.a
pracc-view.o: pracc-view.c common.h pracc.h scan.h

pracc-kill: pracc-kill.o common.a pracclib.a
pracc-kill.o: pracc-kill.c common.h pracc.h print.h

pracc-sum: pracc-sum.o tailocal.o common.a pracclib.a
pracc-sum.o: pracc-sum.c common.h pracc.h print.h scan.h tai.h

pracc-purge: pracc-purge.o common.a pracclib.a
pracc-purge.o: pracc-purge.c common.h pracc.h print.h scan.h tai.h

pracc-check: pracc-check.o printsn.o common.a pracclib.a
pracc-check.o: pracc-check.c common.h pracc.h print.h

pracc-log: pracc-log.o printstm.o tailocal.o common.a pracclib.a
pracc-log.o: pracc-log.c common.h pracc.h scan.h tai.h

pracc-pclog: pracc-pclog.o pclog.o symtab.o common.a pracclib.a
pracc-pclog.o: pracc-pclog.c common.h pclog.h pracc.h

# Pracc Web GUI

pracc.cgi.o: pracc.cgi.c pracc.h symtab.h cgi.h daterange.h \
        ui_acct.h ui_accts.h ui_pclog.h ui_pracclog.h ui_report.h
pracc.cgi: pracc.cgi.o subst.o symtab.o cgi.o daterange.c common.a \
        ui_acct.o ui_accts.o ui_pclog.o ui_pracclog.o ui_report.o \
        pclog.o tailocal.o pracclib.a

accts: ui_accts.c pracc.h tai.h
	cc -D TESTING -o $@ ui_accts.c pracclib.a common.a

ui_acct.o: ui_acct.c ui_acct.h pracc.h tai.h
ui_accts.o: ui_accts.c ui_accts.h pracc.h tai.h
ui_pclog.o: ui_pclog.c ui_pclog.h getln.h pracc.h symtab.h tai.h
ui_pracclog.o: ui_pracclog.c ui_pracclog.h pracc.h tai.h
ui_report.o: ui_report.c ui_report.h pracc.h symtab.h

daterange.o: daterange.c daterange.h scan.h
cgi.o: cgi.c cgi.h pracc.h
pclog.o: pclog.c getln.h pclog.h pracc.h symtab.h tai.h
subst.o: subst.c pracc.h scan.h symtab.h
symtab.o: symtab.c symtab.h

# Pracc API:

pracclib: pracclib.a
pracclib.a: praccAppend.o praccAssemble.o praccCheckName.o praccCreate.o \
	praccFormat.o praccGrant.o praccLogRead.o \
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
praccFormat.o: praccFormat.c pracc.h
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

# joblex:

joblex:	joblex.l pcl5.o pclxl.o papersize.o
	lex  -t joblex.l > joblex.c
	cc   -c -o joblex.o joblex.c
#	cc   joblex.o pcl5.o pclxl.o -lm -o joblex

# Unit tests:

config_test: config_test.c config.c config.h strbuf.h scan.h
	cc -g -o config_test config_test.c config.c strbuf.c scani.c scanu.c
	@./config_test || echo "Unit test for config FAILED"

strbuf_test: strbuf_test.c strbuf.h
	cc -g -o strbuf_test strbuf_test.c strbuf.c
	@./strbuf_test || echo "Unit test for strbuf FAILED"

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
writen.o: writen.c writen.h

pcl5.o: pcl5.c pcl5.h
pclxl.o: pclxl.c pclxl.h
papersize.o: papersize.c papersize.h

prints.o: prints.c print.h
printu.o: printu.c print.h
printi.o: printi.c print.h
printsn.o: printsn.c print.h
printstm.o: printstm.c print.h

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
	rm -f cupspracc $(TOOLS) $(TESTS) pracc.cgi *.o *.a

tgz: clean
	(cd ..; tar chzvf pracc-`date +%Y%m%d`.tgz --exclude test pracc)

dist: clean
	(cd ..; tar chzvf pracc-$(VERSION).tgz \
	 --exclude .git --exclude RCS --exclude legacy --exclude test pracc)
#	(V=`head -1 VERSION`; cd ..; tar chzvf pracc-v$$V.tgz \
#	 --exclude RCS --exclude legacy --exclude test pracc)
