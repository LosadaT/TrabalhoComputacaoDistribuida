CC=gcc
CFLAGS=-Wall -Wextra -O2

all: server client

server: chat_server.c
	$(CC) $(CFLAGS) -o chat_server chat_server.c

client: chat_client.c
	$(CC) $(CFLAGS) -o chat_client chat_client.c

clean:
	rm -f chat_server chat_client

run-server:
	./chat_server 5001

run-client:
	./chat_client 127.0.0.1 5001

	