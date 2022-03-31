# -*- Makefile -*-

#-O3 --std=c99 -lcurses -DNO_MOUSE for NetBSD curses
#adding --std=c99 makes warnings in GNU, and the blame is upon glibc feature test macros. my code is correct.

GAMES_DIR?=/usr/games
SCORES_DIR?=/var/games
MAN_DIR?=/usr/share/man/man6
CFLAGS+=  -Wno-unused-result -D SCORES_DIR=\"$(SCORES_DIR)\"
PKG-CONFIG?=pkg-config
LIBS=$(shell pkg-config --libs --cflags ncurses) -lm


ALL= nbsdgames jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare darrt snakeduel tugow
SCORE_FILES= pipes_scores jewels_scores miketron_scores muncher_scores fisher_scores darrt_scores tugow_scores

all: $(ALL)

scorefiles:
	for sf in $(SCORE_FILES); do touch $(SCORES_DIR)/$$sf ; chmod 664 $(SCORES_DIR)/$$sf; chown :games $(SCORES_DIR)/$$sf ; done;
	for game in $(ALL); do chown :games $(GAMES_DIR)/$$game; chmod g $(GAMES_DIR)/$$game ; done;

manpages:
	cp man/* $(MAN_DIR)
jewels: jewels.c config.h common.h
	$(CC) $(CFLAGS) jewels.c $(LDFLAGS) $(LIBS) -o ./jewels
sudoku: sudoku.c config.h
	$(CC) $(CFLAGS) sudoku.c $(LDFLAGS) $(LIBS) -o ./sudoku
mines: mines.c config.h
	$(CC) $(CFLAGS) mines.c $(LDFLAGS) $(LIBS) -o ./mines
reversi: reversi.c config.h
	$(CC) $(CFLAGS) reversi.c $(LDFLAGS) $(LIBS) -o ./reversi
checkers: checkers.c config.h
	$(CC) $(CFLAGS) checkers.c $(LDFLAGS) $(LIBS) -o ./checkers
battleship: battleship.c config.h
	$(CC) $(CFLAGS) battleship.c $(LDFLAGS) $(LIBS) -o ./battleship
rabbithole: rabbithole.c config.h
	$(CC) $(CFLAGS) rabbithole.c $(LDFLAGS) $(LIBS) -o ./rabbithole
sos: sos.c config.h
	$(CC) $(CFLAGS) sos.c $(LDFLAGS) $(LIBS) -o ./sos
pipes: pipes.c config.h common.h
	$(CC) $(CFLAGS) pipes.c $(LDFLAGS) $(LIBS) -o ./pipes
fifteen: fifteen.c config.h
	$(CC) $(CFLAGS) fifteen.c $(LDFLAGS) $(LIBS) -o ./fifteen
memoblocks: memoblocks.c
	$(CC) $(CFLAGS) memoblocks.c $(LDFLAGS) $(LIBS) -o ./memoblocks
fisher: fisher.c config.h common.h
	$(CC) $(CFLAGS) fisher.c $(LDFLAGS) $(LIBS) -o ./fisher
muncher: muncher.c config.h common.h
	$(CC) $(CFLAGS) muncher.c $(LDFLAGS) $(LIBS) -o ./muncher
miketron: miketron.c config.h common.h
	$(CC) $(CFLAGS) miketron.c $(LDFLAGS) $(LIBS) -o ./miketron
redsquare: redsquare.c config.h
	$(CC) $(CFLAGS) redsquare.c $(LDFLAGS) $(LIBS) -o ./redsquare
darrt: darrt.c config.h common.h
	$(CC) $(CFLAGS) darrt.c $(LDFLAGS) $(LIBS) -o ./darrt
nbsdgames: nbsdgames.c
	$(CC) $(CFLAGS) nbsdgames.c $(LDFLAGS) $(LIBS) -o ./nbsdgames
snakeduel: snakeduel.c config.h
	$(CC) $(CFLAGS) snakeduel.c $(LDFLAGS) $(LIBS) -o ./snakeduel
tugow: tugow.c common.h
	$(CC) $(CFLAGS) tugow.c $(LDFLAGS) $(LIBS) -o ./tugow
menu:
	cp nbsdgames.desktop /usr/share/applications
	cp nbsdgames.svg /usr/share/pixmaps
clean:
	for game in $(ALL); do rm $$game; done;
uninstall:
	for game in $(ALL); do rm $(GAMES_DIR)/$$game; rm $(MAN_DIR)/$$game.6.gz ;done;
install: $(ALL)
	cp $(ALL)  $(GAMES_DIR)
test:
	for game in $(ALL); do ./$$game ;done;

#######for namespacing #######
nb:
	CFLAGS="$$CFLAGS -D NB=\\\"nb\\\"" make
	for game in $(ALL); do cp $$game nb$$game ;done;
	for manpage in $(ls man); do cp man/$$manpage man/nb$$manpage ;done;
nbinstall: nb 
	cp nb* $(GAMES_DIR)
nbmanpages: nb
	cp man/nb* $(MAN_DIR)
nbclean:
	for game in $(ALL); do rm nb$$game; done;
