#$@	Le nom de la cible
#$<	Le nom de la première dépendance
#$^	La liste des dépendances
#$?	La liste des dépendances plus récentes que la cible
#$*	Le nom du fichier sans suffixe

CC=gcc
CFLAGS=-std=c11 -ggdb -I/usr/include/fuse3
LDFLAGS=-lfuse3 -pthread -lcurl
EXEC=prnt_sc_fs
SRC=$(wildcard *.c)
OBJ=$(SRC:%.c=%.o)
all: $(EXEC)

$(EXEC): $(OBJ)
	@$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Linking $^"

%.o: %.c
	@$(CC) -c $^ $(CFLAGS)
	@echo "Compiling $^ -> $<"

.PHONY: clean mrproper

clean:
	rm *.o
	
mrproper: clean
	rm $(EXEC)
