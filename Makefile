CC = gcc
GENERATOR_OBJ = generator.o
SRC = generator.c 


generator:$(GENERATOR_OBJ)
	$(CC) $(GENERATOR_OBJ) -o GENERATOR -lreadline -lcrypto -lsqlite3
clean:
	rm -f *.o GENERATOR 
