OWNER=root
GROUP=games
LIBDIR=/usr/local/lib
BINDIR=/usr/local/games

MANOWNER=root
MANGROUP=root
MANDIR=/usr/local/man

INSTALL=install
PYTHON=python

export # we export all variales to sub-makes

all:
	if [ -e bubbob ]; then make -C bubbob; fi
	make -C display
	@echo -------------------------------------------------------------
	@echo \'make\' successful.
	@echo ' '
	@echo ' Start the game interactively with: python BubBob.py'
	@if [ -e bubbob ]; then echo ' Server only (pure command-line): python bubbob/bb.py --help'; else echo ' Only the client is installed here.'; fi
	@echo ' '
	@echo -------------------------------------------------------------

clean:
	-rm -f `find -name "*~"`
	-rm -f `find -name "*.py[co]"`
	-rm -fr `find -name "build"`
	make -C doc clean
	cd bubbob/images && python buildcolors.py -c
	rm -fr cache

sync: magma-sync codespeak-sync

magma-sync:
	rsync --delete -avz -e ssh ~/games/* magma:games/x/

codespeak-sync:
	rsync --delete -avz -e ssh ${HOME}/games/metaserver ${HOME}/games/common codespeak.net:games/

meta:
	ssh codespeak.net python games/metaserver/metaserver.py -f

docs:
	make -C doc

install-docs:
	make -C doc install

# crude install
install: install-docs
# install fanciness not yet implemented :)
#	make -C bubbob install
#	make -C display install	
	$(INSTALL) -d $(LIBDIR)/bub-n-bros
	cp -R . $(LIBDIR)/bub-n-bros
	chown -R $(OWNER):$(GROUP) $(LIBDIR)/bub-n-bros
	ln -s $(LIBDIR)/bub-n-bros/display/Client.py $(BINDIR)/bubnbros
	ln -s $(LIBDIR)/bub-n-bros/bubbob/bb.py $(BINDIR)/bubnbros-server
	chmod +x $(BINDIR)/bubnbros
	chmod +x $(BINDIR)/bubnbros-server
	$(PYTHON) $(LIBDIR)/bub-n-bros/bubbob/images/buildcolors.py
