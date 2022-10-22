# Makefile for UNIX
BINDIR = /usr/local/bin
MANDIR = /usr/local/man
MANJP = ja_JP.ujis
CONFIGS =

CC = gcc
CFLAGS = -O2 -Wall -DUNIX $(CONFIGS)	# for gcc
#CFLAGS = -g -Wall -DUNIX $(CONFIGS)	# for debugging
#CC = cc
#CFLAGS = -O -DUNIX $(CONFIGS)		# for non-gcc

EXE = mml2mid
LIBS =
OBJS = file.o charproc.o note.o mmlproc.o mml2mid.o

all : $(EXE)

install : all
	strip $(EXE)
	cp $(EXE) $(BINDIR)
	mkdir $(MANDIR)/$(MANJP) 2>/dev/null || :
	mkdir $(MANDIR)/$(MANJP)/man1 2>/dev/null || :
	cp mml2mid.1 $(MANDIR)/$(MANJP)/man1

clean :
	rm -f $(EXE) $(OBJS) core

$(EXE) : $(OBJS)
	$(CC) -o $(EXE) $(CFLAGS) $(OBJS) $(LIBS)

file.o : file.c file.h win.h
	$(CC) $(CFLAGS) -c file.c

charproc.o : charproc.c charproc.h win.h file.h
	$(CC) $(CFLAGS) -c charproc.c

note.o : note.c win.h file.h charproc.h mmlproc.h
	$(CC) $(CFLAGS) -c note.c

mmlproc.o : mmlproc.c mmlproc.h charproc.h win.h file.h
	$(CC) $(CFLAGS) -c mmlproc.c

mml2mid.o : mml2mid.c mml2mid.h mmlproc.h charproc.h win.h file.h
	$(CC) $(CFLAGS) -c mml2mid.c
