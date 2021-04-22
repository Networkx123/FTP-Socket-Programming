CXX = clang++ -std=c++17 

build: client server

client: 
	# build client
	${CXX} -o Client/client Client/client.cpp

server:
	# build server 
	${CXX} -o Server/server Server/server.cpp

.PHONY: build client server
