# 'make compress' to build the docs as gzipped manpages.
# 'make txt' to build the docs as plain text manpages.
# 'make html' to build the docs as html-ized manpages.

all: txt html compress
txt: bb.py.txt Client.py.txt BubBob.py.txt
html: bb.py.html Client.py.html BubBob.py.html

compress: bb.py.1.gz Client.py.1.gz BubBob.py.1.gz

%.html : %.1
	groff -man -Thtml $< > $@

%.txt : %.1
	groff -man -Tascii $< > $@

%.1.gz : %.1
	gzip -c $< > $@

clean:
	rm -f bb.py.txt Client.py.txt BubBob.py.txt
	rm -f bb.py.html Client.py.html BubBob.py.html
	rm -f *.gz

install:
	$(INSTALL) -c -o $(MANOWNER) -g $(MANGROUP) -m 644 bb.py.1.gz $(MANDIR)/man1/bubnbros-server.1.gz
	$(INSTALL) -c -o $(MANOWNER) -g $(MANGROUP) -m 644 Client.py.1.gz $(MANDIR)/man1/bubnbros.1.gz
	$(INSTALL) -c -o $(MANOWNER) -g $(MANGROUP) -m 644 BubBob.py.1.gz $(MANDIR)/man1/BubBob.1.gz

	### TODO: install for html and txt files!
