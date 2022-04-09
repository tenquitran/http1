all: server client

server: server.cpp
	g++ -I ../../libs/boost_1_78_0 common.cpp server.cpp ../../libs/boost_1_78_0/stage/lib/libboost_program_options.a -o server -DASIO_STANDALONE -pthread

client: client.cpp
	g++ -I ../../libs/boost_1_78_0 common.cpp client.cpp ../../libs/boost_1_78_0/stage/lib/libboost_program_options.a -o client -DASIO_STANDALONE -pthread
