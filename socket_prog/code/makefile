all:clean server client

server: server.cc message.h
	g++ -std=c++11 server.cc -o server

client: client.cc message.h
	g++ -std=c++11 client.cc -o client

clean: 
	rm -rf server client
