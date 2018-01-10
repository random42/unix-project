CC = gcc
BIN = ./bin/
BUILD = ./build/

gestore: gestore.o
	${CC} -o ${BIN}$@ ${BUILD}gestore.o
a: type_a.o
	${CC} -o ${BIN}$@ ${BUILD}type_a.o
b: type_b.o
	${CC} -o ${BIN}$@ ${BUILD}type_b.o
%.o:	%.c
	${CC} -c $*.c -o ${BUILD}$@
clean:
	rm -f ${BIN}* ${BUILD}*
