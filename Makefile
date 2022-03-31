# -*- Makefile -*-

#-O3 --std=c99 -lcurses -DNO_MOUSE for NetBSD curses
#adding --std=c99 makes warnings in GNU, and the blame is upon glibc feature test macros. my code is correct.

GAMESDIR?=$(PREFIX)/usr/games
SCORESDIR?=$(PREFIX)/var/games
MANDIR?=$(PREFIX)/usr/share/man/man6
CFLAGS=-Wno-unused-result -DSCORES_DIR=\"$(PREFIX)$(SCORESDIR)\"
LIBS=$(shell pkg-config --libs --cflags ncurses) -lm


ALL= nbsdgames jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare darrt snakeduel tugow
SCORE_FILES= pipes_scores jewels_scores miketron_scores muncher_scores fisher_scores darrt_scores tugow_scores

all: $(ALL)

scorefiles:
	for sf in $(SCORE_FILES); do touch $(DESTDIR)$(SCORESDIR)/$$sf ; chmod 664 $(DESTDIR)$(SCORESDIR)/$$sf; chown :games $(DESTDIR)$(SCORESDIR)/$$sf ; done;
	for game in $(ALL); do chown :games $(DESTDIR)$(GAMESDIR)/$$game; chmod g $(DESTDIR)$(GAMESDIR)/$$game ; done;

manpages:
	cp man/* $(DESTDIR)$(MANDIR)
jewels: jewels.c config.h common.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
sudoku: sudoku.c config.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
mines: mines.c config.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
reversi: reversi.c config.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
checkers: checkers.c config.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
battleship: battleship.c config.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
rabbithole: rabbithole.c config.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
sos: sos.c config.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
pipes: pipes.c config.h common.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
fifteen: fifteen.c config.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
memoblocks: memoblocks.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
fisher: fisher.c config.h common.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
muncher: muncher.c config.h common.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
miketron: miketron.c config.h common.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
redsquare: redsquare.c config.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
darrt: darrt.c config.h common.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
nbsdgames: nbsdgames.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
snakeduel: snakeduel.c config.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
tugow: tugow.c common.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
menu:
	cp nbsdgames.desktop $(DESTIDR)$(PREFIX)/usr/share/applications
	cp nbsdgames.svg $(DESTDIR)$(PREFIX)/usr/share/pixmaps
clean:
	for game in $(ALL); do rm $$game; done;
uninstall:
	for game in $(ALL); do rm $(GAMESDIR)/$$game; rm $(MANDIR)/$$game.6.gz ;done;
install: $(ALL)
	cp $(ALL) $(DESTDIR)/$(GAMESDIR)
test:
	for game in $(ALL); do ./$$game ;done;

#######for namespacing #######
nb:
	CFLAGS="$$CFLAGS -D NB=\\\"nb\\\"" $(MAKE)
	for game in $(ALL); do cp $$game nb$$game ;done;
	for manpage in $(ls man); do cp man/$$manpage man/nb$$manpage ;done;
nbinstall: nb 
	cp nb* $(DESTDIR)/$(GAMESDIR)
nbmanpages: nb
	cp man/nb* $(DESTDIR)/$(MANDIR)
nbclean:
	for game in $(ALL); do rm nb$$game; done;
