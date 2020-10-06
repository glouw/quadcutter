BIN = quadcutter

CC = gcc

all: $(BIN)
	./$(BIN) mill.png

$(BIN): Makefile main.c
	$(CC) main.c -fsanitize=address -g -Ofast -march=native -Wall -Wextra -lSDL2 -lSDL2_image -o $(BIN)
