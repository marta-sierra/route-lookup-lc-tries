CC      ?= gcc
CFLAGS  ?= -Wall -O3
SRC     := $(wildcard src/*.c)
BIN     := my_route_lookup

.PHONY: all clean run test

all: $(BIN)

$(BIN): $(SRC) $(wildcard src/*.h)
	$(CC) $(CFLAGS) $(SRC) -o $@ -lm

# Small example: make run TABLE=data/routing_table_mid.txt PACKETS=tests/prueba0.txt
TABLE   ?= data/routing_table_simple.txt
PACKETS ?= tests/prueba0.txt
run: $(BIN)
	./$(BIN) $(TABLE) $(PACKETS)

clean:
	rm -f $(BIN) tests/*.out
