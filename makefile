CC = gcc
CFLAGS = -Wall -pthread
LIBS = -lhpdf

all: server client client_admin

server: server.c pdf_utils.c convert_utils.c
	$(CC) $(CFLAGS) -o server server.c pdf_utils.c convert_utils.c $(LIBS)

client: client.c 
	$(CC) $(CFLAGS) -o client client.c

client_admin: client_admin.c 
	$(CC) $(CFLAGS) -o client_admin client_admin.c

clean:
	rm -f server client client_admin
