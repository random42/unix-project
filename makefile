CC = gcc
BIN = bin/
BUILD = build/
SRC = src/
A = $(BUILD)type_a.o \
		$(BUILD)child.o
B = $(BUILD)type_b.o \
		$(BUILD)shm.o \
		$(BUILD)child.o
GESTORE = $(BUILD)gestore.o \
					$(BUILD)people.o \
					$(BUILD)shm.o

all: gestore a b
gestore: $(GESTORE)
	$(CC) -o $(BIN)$@ $^
a: $(A)
	$(CC) -o $(BIN)$@ $^
b: $(B)
	$(CC) -o $(BIN)$@ $^
$(BUILD)%.o:	$(SRC)%.c
	$(CC) -g -c $(SRC)$*.c -o $@
clean:
	rm -rf $(BIN) $(BUILD)
