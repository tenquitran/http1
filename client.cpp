#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

using namespace boost;

///////////////////////////////////////////////////////////////////////

struct TpPostTimerArgs
{
	TpPostTimerArgs()
	{
		m_exitThread.store(false);
	}

	std::atomic<bool> m_exitThread;
	int m_delaySeconds = {};
	std::string m_someStr = "String12345";    // TODO: temp
} tpPostTimerArgs;

///////////////////////////////////////////////////////////////////////

bool parseCmdLineArgs(int argc, char* argv[]);

bool sendHeadRequest(asio::ip::tcp::socket& sock);

// Thread procedure to reload the POST request from file.
void *tpPostTimer(void *arg);

///////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
	// TODO: hard-coded
	const std::string ipStr = "127.0.0.1";
    const unsigned short port = 8976;
    
    // TODO: hard-coded
    tpPostTimerArgs.m_delaySeconds = 5;

#if 0
    if (!parseCmdLineArgs(argc, argv))
    {
    	// TODO: show help
    	return 1;
    }
#endif

	try
	{
		pthread_t tid1;
		int t1 = pthread_create(&tid1, nullptr, &tpPostTimer, (void *)&tpPostTimerArgs);
		
		if (0 != t1)
		{
			std::cerr << "Failed to create POST timer thread: " << t1 << '\n';
			return 2;
		}

		asio::ip::tcp protocol = asio::ip::tcp::v4();
		
		asio::ip::tcp::endpoint ep(asio::ip::address::from_string(ipStr), port);
		
		asio::io_service io;
		
		asio::ip::tcp::socket sock(io, ep.protocol());
		
		sock.connect(ep);

		// TODO: interact with the server
		if (!sendHeadRequest(sock))
		{
			std::cerr << "Failed to send HEAD request to the server\n";
		}
		
		// Notify the POST timer thread to exit and wait for its exit.
		
		tpPostTimerArgs.m_exitThread.store(true);

		void *t1res;
		
		t1 = pthread_join(tid1, &t1res);
		
		if (0 != t1)
		{
			std::cerr << "Failed to join POST timer thread: " << t1 << '\n';
			return 3;
		}
		
		std::cout << "POST timer thread returned " << (long)t1res << std::endl;
    }
	catch (system::system_error& ex)
	{
		std::cerr << "Exception: " << ex.what() << '\n';
		//std::cerr << "Exception: " << ex.what() << " (" << ex.code() << ")\n";
		return 4;
	}

    return 0;
}

#if 0
bool parseCmdLineArgs(int argc, char* argv[])
{
	using namespace boost::program_options;
	
	if (argc < 3)
	{
		// TODO: display usage info
		return false;
	}

    // --host=hostname - the host of HTTP server
	// --port=portnumber - the port of HTTP server

    options_description desc{"CommandLineOptions"};
    
    desc.add_options()
      ("host", value<std::string>()->default_value("127.0.0.1"), "HostName")
      ("port", value<int>()->default_value(8976), "Port");
      
    variables_map vm;
    
	try
	{
		store(parse_command_line(argc, argv, desc), vm);
		notify(vm);
	}
	catch(const std::exception& ex)
	{
    	std::cerr << ex.what() << std::endl;
    	// TODO: display usage info
    	return false;
  	}

	if (vm.count("host"))
	{
		std::cout << "Host name: " << vm["host"].as<std::string>() << std::endl;
	}
	
	if (vm.count("port"))
	{
		std::cout << "Port: " << vm["port"].as<int>() << std::endl;
	}

	return true;
}
#endif

bool sendHeadRequest(asio::ip::tcp::socket& sock)
{
	/*
	HTTP HEAD request example:

	HEAD /echo/head/json HTTP/1.1
	Accept: application/json
	Host: reqbin.com
	*/
	
	/*
	Server Response example:

	HTTP/1.1 200 OK
	Content-Length: 19
	Content-Type: application/json
	*/

	std::string request = "HEAD /echo/head/json HTTP/1.1"
	                      "Accept: application/json"
	                      "Host: somehost.com";
	                      
	const size_t cbReq = request.length();
	
	std::size_t cbSent = {};

	try
	{
		cbSent = sock.send(
			asio::buffer(request.c_str(), cbReq), 
			0);
		
		std::cout << "Sent " << cbSent << " bytes of " << cbReq << std::endl;
	}
	catch (system::system_error& ex)
	{
		std::cerr << "Exception on sending HEADER: " << ex.what() << '\n';
		//std::cerr << "Exception: " << ex.what() << " (" << ex.code() << ")\n";
		return false;
	}
	
	return (cbReq == cbSent);
}

// Thread procedure to reload the POST request from file.
void *tpPostTimer(void *arg)
{
	TpPostTimerArgs *pArg = (TpPostTimerArgs *)arg;

	while (!pArg->m_exitThread)
	{
		sleep(pArg->m_delaySeconds);
		
		// TODO: reload the POST request field
		std::cout << "Thread arg: "
				  << pArg->m_someStr 
				  << std::endl;
	}
	
	std::cout << "POST timer thread exit" << std::endl;
	
	return 0;
}

