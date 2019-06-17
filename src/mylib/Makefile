
CFLAGS = -Wall

OBJS = getln.o putln.o putfmt.o putbuf.o \
 printi.o printu.o prints.o printsn.o printstm.o \
 scani.o scanu.o scanpat.o scandate.o scantime.o scanip4op.o

mylib: mylib.a
mylib.a: $(OBJS)
	rm -f $@
	ar cr $@ $+
	ranlib $@

getln.o: getln.c getln.h
putln.o: putln.c putln.h
putfmt.o: putfmt.c putln.h
putbuf.o: putbuf.c putln.h

scani.o: scani.c scan.h
scanu.o: scanu.c scan.h
scanpat.o: scanpat.c scan.h
scandate.o: scandate.c scan.h
scantime.o: scantime.c scan.h
scanip4op.o: scanip4op.c scan.h

printi.o: printi.c print.h
printu.o: printu.c print.h
prints.o: prints.c print.h
printsn.o: printsn.c print.h
printstm.o: printstm.c print.h

clean:
	rm -f *.o *.a

.PHONY: mylib clean
