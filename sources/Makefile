# -*- Makefile -*-

all: jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks

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
clean:
	rm ./jewels ./sudoku ./checkers ./mines ./reversi ./battleship ./rabbithole ./sos ./pipes ./fifteen ./memoblocks
uninstall:
	rm $(PREFIX)/jewels $(PREFIX)/sudoku $(PREFIX)/checkers $(PREFIX)/mines $(PREFIX)/reversi $(PREFIX)/battleship $(PREFIX)/rabbithole $(PREFIX)/sos $(PREFIX)/pipes $(PREFIX)/fifteen $(PREFIX)/memoblocks 
copy_sources:
	cp Makefile config.h jewels.c sudoku.c mines.c reversi.c checkers.c battleship.c rabbithole.c sos.c pipes.c fifteen.c memoblocks.c $(PREFIX)
install: jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks
	cp jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks $(PREFIX)

