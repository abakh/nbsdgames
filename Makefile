# -*- Makefile -*-
all: jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare
scorefiles:
	touch $(PREFIX)/pp_scores
	touch $(PREFIX)/jw_scores
	touch $(PREFIX)/mt_scores
	touch $(PREFIX)/mnch_scores
	touch $(PREFIX)/fsh_scores
	chmod 666 $(PREFIX)/pp_scores
	chmod 666 $(PREFIX)/jw_scores
	chmod 666 $(PREFIX)/mt_scores
	chmod 666 $(PREFIX)/mnch_scores
	chmod 666 $(PREFIX)/fsh_scores
	
jewels: jewels.c config.h
	$(CC) jewels.c -lncurses -o ./jewels
sudoku: sudoku.c
	$(CC) sudoku.c -lncurses -lm -o ./sudoku
mines: mines.c
	$(CC) mines.c -lncurses  -o ./mines
reversi: reversi.c
	$(CC) reversi.c -lncurses  -o ./reversi
checkers: checkers.c
	$(CC) checkers.c -lncurses -o ./checkers
battleship: battleship.c
	$(CC) battleship.c -lncurses -o ./battleship
rabbithole: rabbithole.c
	$(CC) rabbithole.c -lncurses -o ./rabbithole
sos: sos.c
	$(CC) sos.c -lncurses -o ./sos
pipes: pipes.c config.h
	$(CC) pipes.c -lncurses -o ./pipes
fifteen: fifteen.c
	$(CC) fifteen.c -lncurses -o ./fifteen
memoblocks: memoblocks.c
	$(CC) memoblocks.c -lncurses -o ./memoblocks
fisher: fisher.c config.h
	$(CC) fisher.c -lncurses -o ./fisher
muncher: muncher.c config.h
	$(CC) muncher.c -lncurses -o ./muncher
miketron: miketron.c config.h
	$(CC) miketron.c -lncurses -o ./miketron
redsquare: redsquare.c
	$(CC) redsquare.c -lncurses -o ./redsquare
clean:
	rm ./jewels ./sudoku ./checkers ./mines ./reversi ./battleship ./rabbithole ./sos ./pipes ./fifteen ./memoblocks ./fisher ./muncher ./miketron ./redsquare
uninstall:
	rm $(PREFIX)/jewels $(PREFIX)/sudoku $(PREFIX)/checkers $(PREFIX)/mines $(PREFIX)/reversi $(PREFIX)/battleship $(PREFIX)/rabbithole $(PREFIX)/sos $(PREFIX)/pipes $(PREFIX)/fifteen $(PREFIX)/memoblocks $(PREFIX)/fisher $(PREFIX)/muncher $(PREFIX)/miketron $(PREFIX)/redsquare
copy_sources:
	cp Makefile config.h jewels.c sudoku.c mines.c reversi.c checkers.c battleship.c rabbithole.c sos.c pipes.c fifteen.c memoblocks.c fisher.c muncher.c miketron.c redsquare.c $(PREFIX)
install: scorefiles jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare
	cp jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare $(PREFIX)

