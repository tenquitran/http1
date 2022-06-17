all: server client

# Using Boost (in particular, Boost.ProgramOptions) from a custom directory
#server: server.cpp
#	g++ -Wall -I ../../libs/boost_1_78_0 -DSERVER common.cpp cmdArgs.cpp responseReloader.cpp intervalTimer.h serverAsio.cpp server.cpp ../../libs/boost_1_78_0/stage/lib/libboost_program_options.a -o server -DASIO_STANDALONE -pthread -lrt

# Using Boost (in particular, Boost.ProgramOptions) from "/usr/include" and "/usr/lib"
server: server.cpp
	g++ -Wall -I /usr/include/boost -DSERVER common.cpp cmdArgs.cpp responseReloader.cpp intervalTimer.h serverAsio.cpp server.cpp -L/usr/lib/boost -lboost_program_options -static -o server -DASIO_STANDALONE -pthread -lrt

# Using Boost (in particular, Boost.ProgramOptions) from a custom directory
#client: client.cpp
#	g++ -Wall -I ../../libs/boost_1_78_0 common.cpp cmdArgs.cpp requestReloader.cpp postSender.cpp keepAlive.cpp intervalTimer.h client.cpp ../../libs/boost_1_78_0/stage/lib/libboost_program_options.a -o client -DASIO_STANDALONE -pthread -lrt

# Using Boost (in particular, Boost.ProgramOptions) from "/usr/include" and "/usr/lib"
client: client.cpp
	g++ -Wall -I ./usr/include/boost common.cpp cmdArgs.cpp requestReloader.cpp postSender.cpp keepAlive.cpp intervalTimer.h client.cpp -L/usr/lib/boost -lboost_program_options -static -o client -DASIO_STANDALONE -pthread -lrt
