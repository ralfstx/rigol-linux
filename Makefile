
CC = gcc
SRCDIR = src
BINDIR = bin

$(BINDIR)/rigol: $(SRCDIR)/rigol.c
	$(CC) -O $(SRCDIR)/rigol.c -o $(BINDIR)/rigol -l readline

install:
	cp $(BINDIR)/rigol /usr/local/bin

clean:
	rm -f $(BINDIR)/* core

