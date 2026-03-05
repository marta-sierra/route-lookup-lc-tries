SRC = my_route_lookup.c trie.c io.c utils.c
CFLAGS = -Wall -O3

all: my_route_lookup

my_route_lookup: $(SRC)
	gcc $(CFLAGS) $(SRC) -o my_route_lookup -lm

.PHONY: clean

clean:
	rm -f my_route_lookup