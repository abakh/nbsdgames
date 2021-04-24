# -*- Makefile -*-

CFLAGS+= -O3 -lncurses -Wno-unused-result
#-O3 --std=c99 -lcurses -DNO_MOUSE for NetBSD curses
#adding --std=c99 makes warnings in GNU, and the blame is upon glibc feature test macros. my code is correct.

GAMES_DIR?=/usr/games
SCORES_DIR?=/var/games

all: jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare darrt snakeduel
scorefiles:
	touch $(SCORES_DIR)/pipes_scores
	touch $(SCORES_DIR)/jewels_scores
	touch $(SCORES_DIR)/miketron_scores
	touch $(SCORES_DIR)/muncher_scores
	touch $(SCORES_DIR)/fisher_scores
	touch $(SCORES_DIR)/darrt_scores
	chmod 666 $(SCORES_DIR)/pipes_scores
	chmod 666 $(SCORES_DIR)/jewels_scores
	chmod 666 $(SCORES_DIR)/miketron_scores
	chmod 666 $(SCORES_DIR)/muncher_scores
	chmod 666 $(SCORES_DIR)/fisher_scores
	chmod 666 $(SCORES_DIR)/darrt_scores
	
jewels: jewels.c config.h common.h
	$(CC) jewels.c $(CFLAGS) -o ./jewels
sudoku: sudoku.c config.h 
	$(CC) sudoku.c $(CFLAGS) -lm -o ./sudoku
mines: mines.c config.h
	$(CC) mines.c $(CFLAGS) -o ./mines
reversi: reversi.c config.h
	$(CC) reversi.c $(CFLAGS)  -o ./reversi
checkers: checkers.c config.h
	$(CC) checkers.c $(CFLAGS) -o ./checkers
battleship: battleship.c config.h
	$(CC) battleship.c $(CFLAGS) -o ./battleship
rabbithole: rabbithole.c config.h
	$(CC) rabbithole.c $(CFLAGS) -o ./rabbithole
sos: sos.c config.h
	$(CC) sos.c $(CFLAGS) -o ./sos
pipes: pipes.c config.h
	$(CC) pipes.c $(CFLAGS) -o ./pipes
fifteen: fifteen.c config.h
	$(CC) fifteen.c $(CFLAGS) -o ./fifteen
memoblocks: memoblocks.c
	$(CC) memoblocks.c $(CFLAGS) -o ./memoblocks
fisher: fisher.c config.h common.h
	$(CC) fisher.c $(CFLAGS) -o ./fisher
muncher: muncher.c config.h common.h
	$(CC) muncher.c $(CFLAGS) -o ./muncher
miketron: miketron.c config.h common.h
	$(CC) miketron.c $(CFLAGS) -o ./miketron
redsquare: redsquare.c config.h
	$(CC) redsquare.c $(CFLAGS) -o ./redsquare
darrt: darrt.c config.h common.h
	$(CC) darrt.c $(CFLAGS) -lm -o ./darrt

snakeduel: snakeduel.c config.h
	$(CC) snakeduel.c $(CFLAGS)  -o ./snakeduel
clean:
	rm ./jewels ./sudoku ./checkers ./mines ./reversi ./battleship ./rabbithole ./sos ./pipes ./fifteen ./memoblocks ./fisher ./muncher ./miketron ./redsquare ./darrt ./snakeduel
uninstall:
	rm $(GAMES_DIR)/jewels $(GAMES_DIR)/sudoku $(GAMES_DIR)/checkers $(GAMES_DIR)/mines $(GAMES_DIR)/reversi $(GAMES_DIR)/battleship $(GAMES_DIR)/rabbithole $(GAMES_DIR)/sos $(GAMES_DIR)/pipes $(GAMES_DIR)/fifteen $(GAMES_DIR)/memoblocks $(GAMES_DIR)/fisher $(GAMES_DIR)/muncher $(GAMES_DIR)/miketron $(GAMES_DIR)/redsquare $(GAMES_DIR)/darrt $(GAMES_DIR)/snakeduel
install: scorefiles jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare darrt snakeduel
	cp jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare darrt snakeduel $(GAMES_DIR)

