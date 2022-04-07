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


struct CmdLineArguments
{
	std::string m_ipAddress;
	
	unsigned short m_port = {};
	
	int m_reloadSeconds = {};
};

///////////////////////////////////////////////////////////////////////

void displayUsage(const char* programName);

bool parseCmdLineArgs(int argc, char* argv[], CmdLineArguments& args);

// Thread procedure to reload the POST request from file.
void *tpPostTimer(void *arg);

std::string getHeadRequest();

std::string getPostRequest();

void loadPostRequest();

void exchangeMessages(asio::ip::tcp::socket& sock, ERequest requestType);

///////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
    // Server port is 8976

    // Initial loading of the POST request.
    loadPostRequest();
    
    CmdLineArguments args;

    if (!parseCmdLineArgs(argc, argv, args))
    {
    	displayUsage(argv[0]);
    	return 1;
    }

    postTimerArgs.setDelaySeconds(args.m_reloadSeconds);

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
		
		asio::ip::tcp::endpoint ep(
			asio::ip::address::from_string(args.m_ipAddress), 
			args.m_port);
		
		asio::io_service io;
		
		asio::ip::tcp::socket sock(io, ep.protocol());
		
		std::cout << "Trying to connect to " << args.m_ipAddress 
		          << ":" << args.m_port << std::endl;
		
		sock.connect(ep);

		exchangeMessages(sock, ERequest::Head);

		exchangeMessages(sock, ERequest::Post);
		
		sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		
		sock.close();
		
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

void displayUsage(const char* programName)
{
	// --host=hostname   - the host of HTTP server
	// --port=portnumber - the port of HTTP server
	// --reload=XX       - every XX seconds reload the file where the Request is stored

	std::cout << "Usage:\n"
			  << programName 
			  << " --host=<ip_address> --port=<port_number> --reload=<seconds>\n"
			  << "\nExample:\n"
	          << programName 
	          << " --host=127.0.0.1 --port=8976 --reload=5" 
	          << std::endl;
}

bool parseCmdLineArgs(int argc, char* argv[], CmdLineArguments& args)
{
	using namespace boost::program_options;
	
	if (argc < 4)
		{return false;}

    options_description desc{"CommandLineOptions"};
    
    desc.add_options()
      ("host",   value<std::string>()->default_value("127.0.0.1"), "HostName")
      ("port",   value<int>()->default_value(8976),                "Port")
      ("reload", value<int>()->default_value(5),                   "Reload");
      
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
		args.m_ipAddress = vm["host"].as<std::string>();
	}
	else
		{return false;}
	
	if (vm.count("port"))
	{
		std::cout << "Port: " << vm["port"].as<int>() << std::endl;
		args.m_port = vm["port"].as<int>();
	}
	else
		{return false;}
	
	if (vm.count("reload"))
	{
		std::cout << "Reload: " << vm["reload"].as<int>() << std::endl;
		args.m_reloadSeconds = vm["reload"].as<int>();
	}
	else
		{return false;}

	return true;
}

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
#if 1
	return "HEAD /index.html HTTP/1.1\r\n"
           "Accept: application/json\r\n"
           "Host: somehost.com\r\n\r\n";
#else
	return "HEAD /index.html HTTP/1.1\r\n"
           "Accept: text/html\r\n"
           "Host: somehost.com\r\n\r\n";
#endif
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
	
	std::string payload = buffer.str();

	// 2 bytes for "\r\n" following the payload.
	std::string payloadLen = std::to_string(payload.length() + 2);

	std::string request = "POST /test HTTP/1.1\r\n"
						  "Host: somehost.com\r\n"
						  "Content-Type: application/json; utf-8\r\n"
						  "Content-Length: " + payloadLen + "\r\n"
						  "Accept: application/json\r\n\r\n";
	
	request += payload + "\r\n\r\n";
	
#if 0
	std::string line;
	
	while (std::getline(fs, line))
	{
		msg += line + "\r\n";
	}
	msg += "\r\n\r\n";
#endif

	try
	{
		std::lock_guard<std::mutex> lock(postTimerArgs.m_requestLock);
	
		//postTimerArgs.m_request = buffer.str();
		postTimerArgs.m_request = request;
	}
	catch (std::logic_error&)
	{
		std::cout << "POST timer thread: lock_guard exception";
	}
}

void exchangeMessages(asio::ip::tcp::socket& sock, ERequest requestType)
{
	std::string request;
	
	switch (requestType)
	{
		case ERequest::Head:
			request = getHeadRequest();
			break;
		case ERequest::Post:
			request = getPostRequest();
			break;
		default:
			std::cerr << __FUNCTION__ << ": invalid message type\n";
			return;
	}
	
	// TODO: uncomment
#if 1
	// Send the request length.

	std::string reqTypeStr = requestTypeToStr(requestType);

	std::cout << reqTypeStr << " request length: " << request.length() << std::endl;

	if (sendMsgLength(sock, request.length()))
	{
		std::cout << "Sent length of the " << reqTypeStr << " request to the server" << std::endl;
	}
	else
	{
		std::cerr << "Failed to send length of the " << reqTypeStr << " request to the server\n";
	}
#else
	std::string reqTypeStr = requestTypeToStr(requestType);
#endif
	
	// Send the request itself.

	if (sendMsg(sock, request))
	{
		std::cout << "Sent " << reqTypeStr << " request to the server" << std::endl;
	}
	else
	{
		std::cerr << "Failed to send " << reqTypeStr << " request to the server\n";
	}
	
	// Receive the server response.
	
	std::string response = receiveData(sock);
	
	if (response.length() > 0)
	{
		std::cerr << "Received " << reqTypeStr << " response from the server\n";
	}
	else
	{
		std::cerr << "Failed to receive " << reqTypeStr << " response from the server\n";
	}
}

