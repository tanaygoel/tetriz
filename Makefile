CC=gcc
CFLAGS=-Wall -Wextra -O2 -DNDEBUG
LDFLAGS=-pthread -lncurses

OBJS=main.o graphics.o gameplay.o

tetriz: $(OBJS)
	$(CC) $(LDFLAGS) -o tetriz $(OBJS)

main.o: main.c tetriz.h
	$(CC) $(CFLAGS) -c main.c

gameplay.o: gameplay.c tetriz.h
	$(CC) $(CFLAGS) -c gameplay.c

graphics.o: graphics.c tetriz.h
	$(CC) $(CFLAGS) -c graphics.c

clean:
	rm *.o tetriz

