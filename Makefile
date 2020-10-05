all:
	gcc main.c -Ofast -march=native -Wall -Wextra -lSDL2 -lSDL2_image && ./a.out
