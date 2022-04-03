#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include "common.h"

///////////////////////////////////////////////////////////////////////

using namespace boost;

///////////////////////////////////////////////////////////////////////


struct TpPostTimerArgs
{
	TpPostTimerArgs()
		: m_delaySeconds(5)
	{
		m_exitThread.store(false);
	}
	
	void setDelaySeconds(int val)
	{
		m_delaySeconds = val;
	}

	std::atomic<bool> m_exitThread;

	int m_delaySeconds = {};
	
	std::mutex m_requestLock;
	
	std::string m_request;
} postTimerArgs;

///////////////////////////////////////////////////////////////////////

bool parseCmdLineArgs(int argc, char* argv[]);

// Thread procedure to reload the POST request from file.
void *tpPostTimer(void *arg);

std::string getHeadRequest();

std::string getPostRequest();

void loadPostRequest();

///////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
	// TODO: hard-coded
	const std::string ipStr = "127.0.0.1";
    const unsigned short port = 8976;
    
    // TODO: hard-coded
    postTimerArgs.setDelaySeconds(5);
    
    // Initial loading of the POST request.
    loadPostRequest();

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
		int t1 = pthread_create(&tid1, nullptr, &tpPostTimer, (void *)&postTimerArgs);
		
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
		
		// Send the HEAD request length and the HEAD request itself.

		std::string request = getHeadRequest();
		
		std::cout << "HEAD request length: " << request.length() << std::endl;

		if (sendMsgLength(sock, request.length()))
		{
			std::cout << "Send length of the HEAD request to the server" << std::endl;
		}
		else
		{
			std::cerr << "Failed to send length of the HEAD request to the server\n";
		}

		// TODO: interact with the server
		if (sendMsg(sock, request))
		{
			std::cout << "Send HEAD request to the server" << std::endl;
		}
		else
		{
			std::cerr << "Failed to send HEAD request to the server\n";
		}
		
		// Send the POST request length and the POST request itself.

		request = getPostRequest();
		
		std::cout << "POST request length: " << request.length() << std::endl;
		
		if (sendMsgLength(sock, request.length()))
		{
			std::cout << "Send length of the POST request to the server" << std::endl;
		}
		else
		{
			std::cerr << "Failed to send length of the POST request to the server\n";
		}
		
		// TODO: interact with the server
		if (sendMsg(sock, request))
		{
			std::cout << "Send POST request to the server" << std::endl;
		}
		else
		{
			std::cerr << "Failed to send POST request to the server\n";
		}
		
		// Notify the POST timer thread to exit and wait for its exit.
		
		postTimerArgs.m_exitThread.store(true);

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

// Thread procedure to reload the POST request from file.
void *tpPostTimer(void *arg)
{
	TpPostTimerArgs *pArg = (TpPostTimerArgs *)arg;

	while (!pArg->m_exitThread)
	{
		sleep(pArg->m_delaySeconds);
		
		loadPostRequest();
	}
	
	std::cout << "POST timer thread exit" << std::endl;
	
	return 0;
}

std::string getHeadRequest()
{
	return "HEAD /echo/head/json HTTP/1.1\n"
           "Accept: application/json\n"
           "Host: somehost.com";
}

std::string getPostRequest()
{
	std::string request;

	try
	{
		std::lock_guard<std::mutex> lock(postTimerArgs.m_requestLock);
		
		request = postTimerArgs.m_request;
	}
	catch (std::logic_error&)
	{
		std::cout << __FUNCTION__ << ": lock_guard exception";
	}
	
	return request;
}

void loadPostRequest()
{
	std::ifstream fs("post_request.txt");
		
	if (fs.fail())
	{
		std::cerr << "Failed to load POST request from the file\n";
		return;
	}

	std::stringstream buffer;
	buffer << fs.rdbuf();
	
	try
	{
		std::lock_guard<std::mutex> lock(postTimerArgs.m_requestLock);
	
		postTimerArgs.m_request = buffer.str();
	}
	catch (std::logic_error&)
	{
		std::cout << "POST timer thread: lock_guard exception";
	}
}

