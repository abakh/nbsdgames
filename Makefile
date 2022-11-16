# -*- Makefile -*-

#-O3 --std=c99 -lcurses -DNO_MOUSE for NetBSD curses
#adding --std=c99 makes warnings in GNU, and the blame is upon glibc feature test macros. my code is correct.
GAMES_DIR?=$(PREFIX)/usr/bin
SCORES_DIR?=$(PREFIX)/var/games
MAN_DIR?=$(PREFIX)/usr/share/man/man6
CFLAGS+= -Wno-unused-result -DSCORES_DIR=\"$(SCORES_DIR)\"
LIBS_PKG_CONFIG!=pkg-config --libs --cflags ncurses
LIBS=$(LIBS_PKG_CONFIG) -lm


ALL= nbsdgames jewels sudoku mines reversi checkers battleship rabbithole sos pipes fifteen memoblocks fisher muncher miketron redsquare darrt snakeduel tugow revenge trsr sjump
SCORE_FILES= pipes_scores jewels_scores miketron_scores muncher_scores fisher_scores darrt_scores tugow_scores revenge_scores sjump_scores
SRC!=echo *.c *.h
all: $(ALL)

scorefiles:
	for sf in $(SCORE_FILES); do touch $(DESTDIR)$(SCORES_DIR)/$$sf ; chmod 664 $(DESTDIR)$(SCORES_DIR)/$$sf; chown :games $(DESTDIR)$(SCORES_DIR)/$$sf ; done;
	for game in $(ALL); do chown :games $(DESTDIR)$(GAMES_DIR)/$$game; chmod g $(DESTDIR)$(GAMES_DIR)/$$game ; done;

manpages:
	cp man/* $(DESTDIR)$(MAN_DIR)

$(ALL): $(SRC) config.h common.h
	$(CC) $(CFLAGS) $@.c $(LDFLAGS) $(LIBS) -o $@

menu:
	cp nbsdgames.desktop $(DESTIDR)$(PREFIX)/usr/share/applications
	cp nbsdgames.svg $(DESTDIR)$(PREFIX)/usr/share/pixmaps
clean:
	for game in $(ALL); do rm $$game; done;
uninstall:
	for game in $(ALL); do rm $(GAMES_DIR)/$$game; rm $(MAN_DIR)/$$game.6.gz ;done;
install: $(ALL)
	cp $(ALL) $(DESTDIR)/$(GAMES_DIR)
test:
	for game in $(ALL); do ./$$game ;done;

#######for namespacing #######
nb:
	CFLAGS="$$CFLAGS -D NB=\\\"nb\\\"" $(MAKE)
	for game in $(ALL); do cp $$game nb$$game ;done;
	for manpage in $(ls man); do cp man/$$manpage man/nb$$manpage ;done;
nbinstall: nb 
	for game in $(ALL); do cp nb$$game $(DESTDIR)/$(GAMES_DIR) ;done;
	cp nbsdgames $(DESTDIR)/$(GAMES_DIR)
	rm $(DESTDIR)/$(GAMES_DIR)/nbnbsdgames
nbmanpages: nb
	cp man/nb* $(DESTDIR)/$(MAN_DIR)
nbclean: clean
	for game in $(ALL); do rm nb$$game ;done;
	
