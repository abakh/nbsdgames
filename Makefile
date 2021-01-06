# -*- Makefile -*-

CFLAGS= -O3 -lncurses -Wno-unused-result
#-O3 --std=c99 -lcurses -DNO_MOUSE for BSD curses
#adding --std=c99 makes warnings in GNU, and the blame is upon glibc feature test macros. my code is

all: jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare
scorefiles:
	touch /usr/games/pp_scores
	touch /usr/games/jw_scores
	touch /usr/games/mt_scores
	touch /usr/games/mnch_scores
	touch /usr/games/fsh_scores
	chmod 666 /usr/games/pp_scores
	chmod 666 /usr/games/jw_scores
	chmod 666 /usr/games/mt_scores
	chmod 666 /usr/games/mnch_scores
	chmod 666 /usr/games/fsh_scores
	
jewels: jewels.c config.h
	$(CC) jewels.c $(CFLAGS) -o ./jewels
sudoku: sudoku.c
	$(CC) sudoku.c $(CFLAGS) -lm -o ./sudoku
mines: mines.c
	$(CC) mines.c $(CFLAGS) -o ./mines
reversi: reversi.c
	$(CC) reversi.c $(CFLAGS)  -o ./reversi
checkers: checkers.c
	$(CC) checkers.c $(CFLAGS) -o ./checkers
battleship: battleship.c
	$(CC) battleship.c $(CFLAGS) -o ./battleship
rabbithole: rabbithole.c
	$(CC) rabbithole.c $(CFLAGS) -o ./rabbithole
sos: sos.c
	$(CC) sos.c $(CFLAGS) -o ./sos
pipes: pipes.c config.h
	$(CC) pipes.c $(CFLAGS) -o ./pipes
fifteen: fifteen.c
	$(CC) fifteen.c $(CFLAGS) -o ./fifteen
memoblocks: memoblocks.c
	$(CC) memoblocks.c $(CFLAGS) -o ./memoblocks
fisher: fisher.c config.h
	$(CC) fisher.c $(CFLAGS) -o ./fisher
muncher: muncher.c config.h
	$(CC) muncher.c $(CFLAGS) -o ./muncher
miketron: miketron.c config.h
	$(CC) miketron.c $(CFLAGS) -o ./miketron
redsquare: redsquare.c
	$(CC) redsquare.c $(CFLAGS) -o ./redsquare
clean:
	rm ./jewels ./sudoku ./checkers ./mines ./reversi ./battleship ./rabbithole ./sos ./pipes ./fifteen ./memoblocks ./fisher ./muncher ./miketron ./redsquare
uninstall:
	rm $(PREFIX)/jewels $(PREFIX)/sudoku $(PREFIX)/checkers $(PREFIX)/mines $(PREFIX)/reversi $(PREFIX)/battleship $(PREFIX)/rabbithole $(PREFIX)/sos $(PREFIX)/pipes $(PREFIX)/fifteen $(PREFIX)/memoblocks $(PREFIX)/fisher $(PREFIX)/muncher $(PREFIX)/miketron $(PREFIX)/redsquare
copy_sources:
	cp Makefile config.h jewels.c sudoku.c mines.c reversi.c checkers.c battleship.c rabbithole.c sos.c pipes.c fifteen.c memoblocks.c fisher.c muncher.c miketron.c redsquare.c $(PREFIX)
install: scorefiles jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare
	cp jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare $(PREFIX)

