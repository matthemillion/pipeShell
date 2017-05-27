CC = gcc
FLAGS = -std=c99

SRC = pipeShell.c
OBJ = pipeShell.o
PROG = pipeShell

$(PROG): $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o $(PROG)

$(OBJ): $(SRC)
