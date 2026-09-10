CC = gcc
CFLAGS = -Wall -Wextra -g

all: procshed

procshed: procshed.o tokenize.o
	$(CC) $(CFLAGS) procshed.o tokenize.o -o procshed

procshed.o: procshed.c
	$(CC) $(CFLAGS) -c procshed.c

tokenize.o: tokenize.c
	$(CC) $(CFLAGS) -c tokenize.c

clean:
	rm -f *.o procshed
