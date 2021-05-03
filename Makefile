# -*- Makefile -*-

#-O3 --std=c99 -lcurses -DNO_MOUSE for NetBSD curses
#adding --std=c99 makes warnings in GNU, and the blame is upon glibc feature test macros. my code is correct.

GAMES_DIR?=/usr/games
SCORES_DIR?=/var/games
MAN_DIR?=/usr/share/man/man6
CFLAGS+= -O3 -Wno-unused-result -D SCORES_DIR=\"$(SCORES_DIR)\"
LDFLAGS+= -lncurses -lm


ALL= jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare darrt snakeduel
SCORE_FILES= pipes_scores jewels_scores miketron_scores muncher_scores fisher_scores darrt_scores

all: $(ALL)

scorefiles:
	for sf in $(SCORE_FILES); do touch $(SCORES_DIR)/$$sf ; chown :games $(SCORES_DIR)/$$sf ; done;
	for game in $(ALL); do chown :games $(GAMES_DIR)/$$game; chmod +s $(GAMES_DIR)/$$game ; done;

manpages:
	cp man/* $(MAN_DIR)
jewels: jewels.c config.h common.h
	$(CC) jewels.c $(LDFLAGS) $(CFLAGS) -o ./jewels
sudoku: sudoku.c config.h 
	$(CC) sudoku.c $(LDFLAGS) $(CFLAGS)  -o ./sudoku
mines: mines.c config.h
	$(CC) mines.c $(LDFLAGS) $(CFLAGS) -o ./mines
reversi: reversi.c config.h
	$(CC) reversi.c $(LDFLAGS) $(CFLAGS)  -o ./reversi
checkers: checkers.c config.h
	$(CC) checkers.c $(LDFLAGS) $(CFLAGS) -o ./checkers
battleship: battleship.c config.h
	$(CC) battleship.c $(LDFLAGS) $(CFLAGS) -o ./battleship
rabbithole: rabbithole.c config.h
	$(CC) rabbithole.c $(LDFLAGS) $(CFLAGS) -o ./rabbithole
sos: sos.c config.h
	$(CC) sos.c $(LDFLAGS) $(CFLAGS) -o ./sos
pipes: pipes.c config.h common.h
	$(CC) pipes.c $(LDFLAGS) $(CFLAGS) -o ./pipes
fifteen: fifteen.c config.h
	$(CC) fifteen.c $(LDFLAGS) $(CFLAGS) -o ./fifteen
memoblocks: memoblocks.c
	$(CC) memoblocks.c $(LDFLAGS) $(CFLAGS) -o ./memoblocks
fisher: fisher.c config.h common.h
	$(CC) fisher.c $(LDFLAGS) $(CFLAGS) -o ./fisher
muncher: muncher.c config.h common.h
	$(CC) muncher.c $(LDFLAGS) $(CFLAGS) -o ./muncher
miketron: miketron.c config.h common.h
	$(CC) miketron.c $(LDFLAGS) $(CFLAGS) -o ./miketron
redsquare: redsquare.c config.h
	$(CC) redsquare.c $(LDFLAGS) $(CFLAGS) -o ./redsquare
darrt: darrt.c config.h common.h
	$(CC) darrt.c $(LDFLAGS) $(CFLAGS)  -o ./darrt

snakeduel: snakeduel.c config.h
	$(CC) snakeduel.c $(LDFLAGS) $(CFLAGS)  -o ./snakeduel
clean:
	rm $(ALL)
uninstall:
	for game in $(ALL); do rm $(GAMES_DIR)/$$game; rm $(MAN_DIR)/$$game.6.gz done;
install: $(ALL)
	cp $(ALL) $(GAMES_DIR)

