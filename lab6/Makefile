CC      = gcc
CFLAGS  = -O3 -Wall
OMPFLAGS = -fopenmp

BINS    = mm mmp

all: $(BINS)

mm: mm.c
	$(CC) $(CFLAGS) -o $@ $<

mmp: mmp.c
	$(CC) $(CFLAGS) $(OMPFLAGS) -o $@ $<

clean:
	rm -f $(BINS)

.PHONY: all clean