# -*- Makefile -*-

#-O3 --std=c99 -lcurses -DNO_MOUSE for NetBSD curses
#adding --std=c99 makes warnings in GNU, and the blame is upon glibc feature test macros. my code is correct.
PREFIX?=/
GAMESDIR?=$(PREFIX)/usr/bin
SCORESDIR?=$(PREFIX)/var/games
MANDIR?=$(PREFIX)/usr/share/man/man6
CFLAGS+= -Wno-unused-result -DSCORES_DIR=\"$(SCORESDIR)\"
LIBS_PKG_CONFIG!=pkg-config --libs --cflags ncurses
LIBS=$(LIBS_PKG_CONFIG) -lm


ALL= nbsdgames jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare darrt snakeduel tugow
SCORE_FILES= pipes_scores jewels_scores miketron_scores muncher_scores fisher_scores darrt_scores tugow_scores

all: $(ALL)

scorefiles:
	for sf in $(SCORE_FILES); do touch $(DESTDIR)$(SCORESDIR)/$$sf ; chmod 664 $(DESTDIR)$(SCORESDIR)/$$sf; chown :games $(DESTDIR)$(SCORESDIR)/$$sf ; done;
	for game in $(ALL); do chown :games $(DESTDIR)$(GAMESDIR)/$$game; chmod g $(DESTDIR)$(GAMESDIR)/$$game ; done;

manpages:
	cp man/* $(DESTDIR)$(MANDIR)

# Games which only need config.h
sudoku mines reversi checkers battleship rabbithole sos fifteen redsquare snakeduel: config.h
	$(CC) $(CFLAGS) $@.c $< $(LDFLAGS) $(LIBS) -o $@

# Games which need config.h and common.h
jewels pipes fisher muncher miketron darrt: config.h common.h
	$(CC) $(CFLAGS) $@.c $< $(LDFLAGS) $(LIBS) -o $@

# Games which only need common.h
tugow: common.h
	$(CC) $(CFLAGS) $@.c $< $(LDFLAGS) $(LIBS) -o $@

# Games which only need themselves
memoblocks nbsdgames:
	$(CC) $(CFLAGS) $@.c $< $(LDFLAGS) $(LIBS) -o $@

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
