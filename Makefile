# %W% %G% %U% - (c) Copyright 1987, 1988 Chuck Simmons

#
#    Copyright (C) 1987, 1988 Chuck Simmons
#
# See the file COPYING, distributed with empire, for restriction
# and warranty information.

# Note: When the version changes, you also have to change
# the RPM spec file and the LSM.
VERS=1.2

# Change the line below for your system.  If you are on a Sun or Vax,
# you may want BSD.

#SYS = BSD
SYS = SYSV

# Use -g to compile the program for debugging.

DEBUG = -g -DDEBUG
#DEBUG = -O

# Use -p to profile the program.
#PROFILE = -p -DPROFILE
PROFILE =

# Define all necessary libraries.  'curses' is necessary.  'termcap'
# is needed on BSD systems.
LIBS = -lncurses
#LIBS = -lcurses -ltermcap

# You shouldn't have to modify anything below this line.

CFLAGS = $(DEBUG) $(PROFILE) -D$(SYS)

FILES = \
	attack.c \
	compmove.c \
	data.c \
	display.c \
	edit.c \
	empire.c \
	game.c \
	main.c \
	map.c \
	math.c \
	object.c \
	term.c \
	usermove.c \
	util.c

HEADERS = empire.h extern.h

OFILES = \
	attack.o \
	compmove.o \
	data.o \
	display.o \
	edit.o \
	empire.o \
	game.o \
	main.o \
	map.o \
	math.o \
	object.o \
	term.o \
	usermove.o \
	util.o

all: vms-empire

vms-empire: $(OFILES)
	$(CC) $(PROFILE) -o vms-empire $(OFILES) $(LIBS)

TAGS: $(HEADERS) $(FILES)
	etags $(HEADERS) $(FILES)

lint: $(FILES)
	lint -u -D$(SYS) $(FILES) -lcurses

clean:
	rm -f *.o TAGS

clobber: clean
	rm -f vms-empire vms-empire-*.tar*

SOURCES = READ.ME vms-empire.6 COPYING Makefile BUGS $(FILES) $(HEADERS) MANIFEST vms-empire.lsm vms-empire.spec

vms-empire-$(VERS).tar.gz: $(SOURCES)
	@ls $(SOURCES) | sed s:^:vms-empire-$(VERS)/: >MANIFEST
	@(cd ..; ln -s vms-empire vms-empire-$(VERS))
	(cd ..; tar -czvf vms-empire/vms-empire-$(VERS).tar.gz `cat vms-empire/MANIFEST`)
	@(cd ..; rm vms-empire-$(VERS))

vms-empire-$(VERS).shar:
	shar $(SOURCES) >vms-empire-$(VERS).shar

dist: vms-empire-$(VERS).tar.gz

RPMROOT=/usr/src/redhat
RPM = rpm
RPMFLAGS = -ba
rpm: dist
	cp vms-empire-$(VERS).tar.gz $(RPMROOT)/SOURCES;
	cp vms-empire.spec $(RPMROOT)/SPECS
	cd $(RPMROOT)/SPECS; $(RPM) $(RPMFLAGS) vms-empire.spec	
	cp $(RPMROOT)/RPMS/`arch|sed 's/i[4-9]86/i386/'`/vms-empire-$(VERS)*.rpm .
	cp $(RPMROOT)/SRPMS/vms-empire-$(VERS)*.src.rpm .



