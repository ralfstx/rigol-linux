
CC = gcc
CFLAGS = -O -Wall

SRCDIR = src
BINDIR = bin

LIBS = -lreadline

_OBJECTS = rigol.o connection.o
OBJECTS = $(patsubst %,$(BINDIR)/%,$(_OBJECTS))

$(BINDIR)/rigol: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(BINDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

install:
	cp $(BINDIR)/rigol /usr/local/bin

clean:
	rm -f $(BINDIR)/* core

