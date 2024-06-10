CC = gcc
CFLAGS = -Wall
LIBS = -lhpdf

all: server client

server: server.c pdf_utils.c convert_utils.c utils.c
	$(CC) $(CFLAGS) -o server server.c pdf_utils.c convert_utils.c utils.c $(LIBS)

client: client.c utils.c
	$(CC) $(CFLAGS) -o client client.c utils.c

clean:
	rm -f server client