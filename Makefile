# Constants
CC = gcc
PROGS = 1712695
# code files
SOURCES := $(shell find . -iname '*.c')
OBJECTS := $(SOURCES:.c=.o)

$(PROGS):	$(OBJECTS)
	rm -f exe/$(PROGS)
	$(CC) $(OBJECTS) $(LFLAGS) -o $(PROGS)
	mv $(PROGS) exe/$(PROGS)
	rm -f $(OBJECTS)

all: $(PROGS)
.PHONY: clean
clean:
	rm -f exe/$(PROGS)
