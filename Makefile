
all:
	gcc main.c dependencies.c scanner.c parser.c arena.c -o main -g -std=c99 -Wall -Wextra -Wno-sign-compare
