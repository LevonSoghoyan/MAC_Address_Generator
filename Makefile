CC = gcc
<<<<<<< HEAD
TARGET = web_server

all:
	$(CC) server.c -o $(TARGET) -lcrypto -lsqlite3

clean:
	rm -f $(TARGET)
=======
GENERATOR_OBJ = generator.o
SRC = generator.c 


generator:$(GENERATOR_OBJ)
	$(CC) $(GENERATOR_OBJ) -o GENERATOR -lreadline -lcrypto -lsqlite3
clean:
	rm -f *.o GENERATOR 
>>>>>>> 7615c3941350ca5ef6f1ae500b1e67d3128400c8
