# Constants
CC = gcc
PROGS = 1712695
# code files
SOURCES := $(shell find . -iname '*.c')
OBJECTS := $(SOURCES:.c=.o)

$(PROGS):	$(OBJECTS)
	$(CC) $(OBJECTS) $(LFLAGS) -o $(PROGS)

all: $(PROGS)
clean:
	rm $(OBJECTS) $(PROGS)
