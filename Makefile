# -*- Makefile -*-
ifndef $(CFLAGS)
	CFLAGS= -O3 -lncurses -Wno-unused-result
	#-O3 --std=c99 -lcurses -DNO_MOUSE for NetBSD curses
	#adding --std=c99 makes warnings in GNU, and the blame is upon glibc feature test macros. my code is correct.
endif
ifndef $(GAMES_DIR)
	GAMES_DIR=/usr/games
endif
ifndef $(SCORES_DIR)
	SCORES_DIR=/usr/games
endif
all: jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare darrt snakeduel
scorefiles:
	touch $(SCORES_DIR)/pp_scores
	touch $(SCORES_DIR)/jw_scores
	touch $(SCORES_DIR)/mt_scores
	touch $(SCORES_DIR)/mnch_scores
	touch $(SCORES_DIR)/fsh_scores
	touch $(SCORES_DIR)/drt_scores
	chmod 666 $(SCORES_DIR)/pp_scores
	chmod 666 $(SCORES_DIR)/jw_scores
	chmod 666 $(SCORES_DIR)/mt_scores
	chmod 666 $(SCORES_DIR)/mnch_scores
	chmod 666 $(SCORES_DIR)/fsh_scores
	chmod 666 $(SCORES_DIR)/drt_scores
	
jewels: jewels.c config.h
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
fisher: fisher.c config.h
	$(CC) fisher.c $(CFLAGS) -o ./fisher
muncher: muncher.c config.h
	$(CC) muncher.c $(CFLAGS) -o ./muncher
miketron: miketron.c config.h
	$(CC) miketron.c $(CFLAGS) -o ./miketron
redsquare: redsquare.c config.h
	$(CC) redsquare.c $(CFLAGS) -o ./redsquare
darrt: darrt.c config.h
	$(CC) darrt.c $(CFLAGS) -lm -o ./darrt

snakeduel: snakeduel.c config.h
	$(CC) snakeduel.c $(CFLAGS)  -o ./snakeduel
clean:
	rm ./jewels ./sudoku ./checkers ./mines ./reversi ./battleship ./rabbithole ./sos ./pipes ./fifteen ./memoblocks ./fisher ./muncher ./miketron ./redsquare ./darrt ./snakeduel
uninstall:
	rm $(GAMES_DIR)/jewels $(GAMES_DIR)/sudoku $(GAMES_DIR)/checkers $(GAMES_DIR)/mines $(GAMES_DIR)/reversi $(GAMES_DIR)/battleship $(GAMES_DIR)/rabbithole $(GAMES_DIR)/sos $(GAMES_DIR)/pipes $(GAMES_DIR)/fifteen $(GAMES_DIR)/memoblocks $(GAMES_DIR)/fisher $(GAMES_DIR)/muncher $(GAMES_DIR)/miketron $(GAMES_DIR)/redsquare $(GAMES_DIR)/darrt $(GAMES_DIR)/snakeduel
install: scorefiles jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare darrt snakeduel
	cp jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare darrt snakeduel $(GAMES_DIR)

