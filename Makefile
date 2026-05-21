CC = gcc
GENERATOR_OBJ = generator.o
SCANNER_OBJ = scanner.o
SRC = generator.c scanner.c


generator:$(GENERATOR_OBJ)
	$(CC) $(GENERATOR_OBJ) -o GENERATOR -lreadline -lcrypto -lsqlite3
scanner:$(SCANNER_OBJ)
	$(CC) $(SCANNER_OBJ) -o SCANNER -lreadline -lcrypto -lsqlite3

clean:
	rm -f *.o SCANNER GENERATOR 
