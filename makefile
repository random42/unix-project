CC = gcc
BIN = bin/
BUILD = build/
SRC = src/
A = $(BUILD)type_a.o \
		$(BUILD)child.o \
		$(BUILD)sem.o
B = $(BUILD)type_b.o \
		$(BUILD)shm.o \
		$(BUILD)child.o \
		$(BUILD)sem.o
GESTORE = $(BUILD)gestore.o \
					$(BUILD)people.o \
					$(BUILD)shm.o \
					$(BUILD)sem.o

all: dir gestore a b
gestore: $(GESTORE)
	$(CC) -o $(BIN)$@ $^
a: $(A)
	$(CC) -o $(BIN)$@ $^
b: $(B)
	$(CC) -o $(BIN)$@ $^
$(BUILD)%.o:	$(SRC)%.c
	$(CC) -g -c $(SRC)$*.c -o $@
dir:
	mkdir -p $(BIN) $(BUILD)
clean:
	rm -rf $(BIN) $(BUILD)
