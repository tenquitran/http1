all: server client

server: server.cpp
	g++ -I ../../libs/boost_1_78_0 server.cpp -o server -DASIO_STANDALONE -pthread

client: client.cpp
	g++ -I ../../libs/boost_1_78_0 client.cpp -o client -DASIO_STANDALONE -pthread
