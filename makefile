CC = gcc
BIN = ./bin/
BUILD = ./build/

all: gestore.o people.o shm.o type_a.o type_b.o
	make gestore && make a && make b
gestore: gestore.o people.o shm.o
	${CC} -o ${BIN}$@ ${BUILD}gestore.o ${BUILD}people.o ${BUILD}shm.o
a: type_a.o
	${CC} -o ${BIN}$@ ${BUILD}type_a.o
b: type_b.o shm.o
	${CC} -o ${BIN}$@ ${BUILD}type_b.o ${BUILD}shm.o
%.o:	%.c
	${CC} -g -c $*.c -o ${BUILD}$@
clean:
	rm -f ${BIN}* ${BUILD}*
