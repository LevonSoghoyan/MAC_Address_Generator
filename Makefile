CC = gcc
TARGET = web_server

all:
	$(CC) server.c -o $(TARGET) -lcrypto -lsqlite3

clean:
	rm -f $(TARGET)
