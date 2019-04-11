all: Server Client utils.o

Server: Server.c utils.o
	gcc -o Server Server.c utils.o

Client: Client.c utils.o
	gcc -o Client Client.c utils.o

utils.o: utils.c
	gcc -c utils.c

clean:
	rm *.o
