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

struct ReloadPostArgs
{
	ReloadPostArgs()
		: m_delaySeconds(5)
	{
		m_exitThread.store(false);
	}

	std::atomic<bool> m_exitThread;

	int m_delaySeconds = {};
	
	std::mutex m_requestLock;
	
	std::string m_request;
	
	std::string m_requestFilePath;
} reloadPostArgs;


struct SendPostArgs
{
	SendPostArgs()
		: m_delaySeconds(5)
	{
		m_exitThread.store(false);
	}

	std::atomic<bool> m_exitThread;

	int m_delaySeconds = {};

	std::weak_ptr<asio::ip::tcp::socket> m_spSocket;
} sendPostArgs;


struct KeepAliveArgs
{
	KeepAliveArgs()
		: m_delaySeconds(7)
	{
		m_exitThread.store(false);
	}

	std::atomic<bool> m_exitThread;
	
	int m_delaySeconds = {};
	
	std::weak_ptr<asio::ip::tcp::socket> m_spSocket;
} keepAliveArgs;


struct CmdLineArguments
{
	std::string m_ipAddress;
	
	unsigned short m_port = {};
	
	std::string m_requestFilePath;
	
	int m_keepAliveSeconds = {};
	
	// true if the client will send only HEAD keep-alive requests, 
	// false if it will send only POST requests.
	bool m_keepAliveMode = {false};
	
	int m_postSeconds = {};
	
	int m_reloadSeconds = {};
};

///////////////////////////////////////////////////////////////////////

void displayUsage(const char* programName);

bool parseCmdLineArgs(int argc, char* argv[], CmdLineArguments& args);

// Thread procedure to send keep-alive HEAD requests.
void *tpKeepAliveTimer(void *arg);

// Thread procedure to reload the POST request from file.
void *tpReloadPost(void *arg);

// Thread procedure to send the POST request.
void *tpSendPost(void *arg);

std::string getHeadRequest();

std::string getPostRequest();

std::string getHeadRequestKeepAlive();

void loadPostRequest(const std::string& filePath);

void exchangeMessages(asio::ip::tcp::socket& sock, ERequest requestType);

void sigHandler(int arg);

///////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
    signal(SIGINT, sigHandler);

    CmdLineArguments args;

    if (!parseCmdLineArgs(argc, argv, args))
    {
    	displayUsage(argv[0]);
    	return 1;
    }

    // Initial loading of the POST request.
    loadPostRequest(args.m_requestFilePath);
    
    reloadPostArgs.m_requestFilePath = args.m_requestFilePath;

    reloadPostArgs.m_delaySeconds = args.m_reloadSeconds;
    
    sendPostArgs.m_delaySeconds = args.m_postSeconds;
    
    keepAliveArgs.m_delaySeconds = args.m_keepAliveSeconds;

	try
	{
		asio::ip::tcp protocol = asio::ip::tcp::v4();
		
		asio::ip::tcp::endpoint ep(
			asio::ip::address::from_string(args.m_ipAddress), 
			args.m_port);
		
		asio::io_service io;
		
		std::shared_ptr<asio::ip::tcp::socket> sock = 
			std::make_shared<asio::ip::tcp::socket>(io, ep.protocol());
		
		std::cout << "Trying to connect to " << args.m_ipAddress 
		          << ":" << args.m_port << std::endl;
		
		sock->connect(ep);
		
		sendPostArgs.m_spSocket  = sock;
		keepAliveArgs.m_spSocket = sock;
		
		pthread_t tidKeepAlive;
		pthread_t tidPostReloader;
		pthread_t tidPostSender;
		
		if (args.m_keepAliveMode)
		{
			int res = pthread_create(&tidKeepAlive, nullptr, &tpKeepAliveTimer, (void *)&keepAliveArgs);
			
			if (0 != res)
			{
				std::cerr << "Failed to create keep-alive thread: " << res << '\n';
				return 2;
			}
			
			std::cout << "Started the keep-alive thread" << std::endl;
		}
		else
		{
			int res = pthread_create(&tidPostReloader, nullptr, &tpReloadPost, (void *)&reloadPostArgs);
			
			if (0 != res)
			{
				std::cerr << "Failed to create POST reloader thread: " << res << '\n';
				return 2;
			}
			
			std::cout << "Started the POST reloader thread" << std::endl;
			
			res = pthread_create(&tidPostSender, nullptr, &tpSendPost, (void *)&sendPostArgs);
			
			if (0 != res)
			{
				std::cerr << "Failed to create POST sender thread: " << res << '\n';
				return 2;
			}
			
			std::cout << "Started the POST sender thread" << std::endl;
		}

		if (args.m_keepAliveMode)
		{
			while (false == keepAliveArgs.m_exitThread.load())
			{
				sleep(1);
			}
		}
		else
		{
			while (false == reloadPostArgs.m_exitThread.load())
			{
				sleep(1);
			}
		}

		// Wait for the worker threads to exit.
		
		if (args.m_keepAliveMode)
		{
			void *keepAliveExit;
		
			int res = pthread_join(tidKeepAlive, &keepAliveExit);
			
			if (0 != res)
			{
				std::cerr << "Failed to join the keep-alive thread: " << res << '\n';
				
				sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
				sock->close();
			
				return 4;
			}
			
			std::cout << "Keep-alive timer thread returned " << (long)keepAliveExit << std::endl;
		}
		else
		{
			void *postReloaderExit;
			
			int res = pthread_join(tidPostReloader, &postReloaderExit);
			
			if (0 != res)
			{
				std::cerr << "Failed to join the POST reloader thread: " << res << '\n';
				
				sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
				sock->close();
				
				return 3;
			}
			
			std::cout << "POST reloader thread returned " << (long)postReloaderExit << std::endl;

			void *postSenderExit;
			
			res = pthread_join(tidPostSender, &postSenderExit);
			
			if (0 != res)
			{
				std::cerr << "Failed to join the POST sender thread: " << res << '\n';
				
				sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
				sock->close();
				
				return 3;
			}
			
			std::cout << "POST sender thread returned " << (long)postSenderExit << std::endl;
		}

		sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		
		sock->close();
    }
	catch (system::system_error& ex)
	{
		std::cerr << "Exception: " << ex.what() << '\n';
		return 5;
	}

    return 0;
}

void sigHandler(int arg)
{
	// Notify the POST reloader thread to exit.
	reloadPostArgs.m_exitThread.store(true);
	
	// Notify the POST sender thread to exit.
	sendPostArgs.m_exitThread.store(true);
	
	// Notify the keep-alive thread to exit.
	keepAliveArgs.m_exitThread.store(true);
}

void displayUsage(const char* programName)
{
	// --host=hostname    - the host of HTTP server
	// --port=portnumber  - the port of HTTP server
	// --keep-alive=XX    - run HTTP client with keep-alive HEAD requests every XX seconds
	// --post-request=XX  - send HTTP POST request every XX seconds
	// --reload=XX        - every XX seconds reload the file where the Request is stored
	// --request=/path/to/Request.json  - path to Request.html used by client to send to the Server

	std::cout << "Usage (variant 1, sending keep-alive HEAD requests):\n\n"
			  << programName 
			  << " --host=<ip_address> --port=<port_number> "
			     "--keep-alive=<seconds>\n"

			  << "\nUsage (variant 2, sending POST requests from the specified file):\n\n"
			  << programName
			  << " --host=<ip_address> --port=<port_number> "
			     "--post-request=<seconds> "
			     "--reload=<seconds> --request=/path/to/Request.json\n"
			  
			  << "\nExample 1:\n\n"
	          << programName 
	          << " --host=127.0.0.1 --port=8976 --keep-alive=7\n\n" 
	          
	          << "\nExample 2:\n\n"
	          << programName 
	          << " --host=127.0.0.1 --port=8976 "
	             "--post-request=5 --request=requests/post_request.json --reload=5"
	          << std::endl;
}

bool parseCmdLineArgs(int argc, char* argv[], CmdLineArguments& args)
{
	using namespace boost::program_options;
	
	// There should be at least "host" and "port" options 
	// and either "keep-alive" or a set of POST options.
	if (argc < 4)
		{return false;}

    options_description desc{"CommandLineOptions"};
    
    desc.add_options()
      ("host",         value<std::string>()->default_value("127.0.0.1"), "HostName")
      ("port",         value<int>()->default_value(8976),                "Port")
      ("keep-alive",   value<int>()->default_value(7),                   "KeepAlive")
      ("post-request", value<int>()->default_value(5),                   "PostRequest")
      ("reload",       value<int>()->default_value(6),                   "Reload")
      ("request",      value<std::string>()->default_value(""),          "RequestPath");
      
    variables_map vm;
    
	try
	{
		store(parse_command_line(argc, argv, desc), vm);
		notify(vm);
	}
	catch(const std::exception& ex)
	{
    	std::cerr << ex.what() << std::endl;
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
	
	// We are suppose that the "keep-alive" command-line option 
	// is mutually exclusive with the set of options "post-request", "reload" and "request" -
	// that is, that the client will send either only keep-alive HEAD requests 
	// or only POST requests.
	
	if (!vm["keep-alive"].defaulted())
	{
		std::cout << "Keep-alive: " << vm["keep-alive"].as<int>() << std::endl;
		
		args.m_keepAliveSeconds = vm["keep-alive"].as<int>();
		args.m_keepAliveMode = true;
		
		std::cout << "Mode: keep-alive" << std::endl;
		
		return true;
	}
	else
	{
		if (vm.count("post-request"))
		{
			std::cout << "Post-request: " << vm["post-request"].as<int>() << std::endl;
			args.m_postSeconds = vm["post-request"].as<int>();
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

		if (vm.count("request"))
		{
			std::cout << "Request path: " << vm["request"].as<std::string>() << std::endl;
			args.m_requestFilePath = vm["request"].as<std::string>();
		}
		else
			{return false;}
			
		std::cout << "Mode: POST" << std::endl;
	}

	return true;
}

// Thread procedure to reload the POST request from file.
void *tpReloadPost(void *arg)
{
	ReloadPostArgs *pArg = (ReloadPostArgs *)arg;

	while (!pArg->m_exitThread)
	{
		loadPostRequest(pArg->m_requestFilePath);

		sleep(pArg->m_delaySeconds);
	}
	
	std::cout << "POST reload thread exit" << std::endl;
	
	return 0;
}

// Thread procedure to send the POST request.
void *tpSendPost(void *arg)
{
	SendPostArgs *pArg = (SendPostArgs *)arg;

	while (!pArg->m_exitThread)
	{
		std::shared_ptr<asio::ip::tcp::socket> spSock = pArg->m_spSocket.lock();
		
		if (spSock)
		{
			exchangeMessages(*spSock, ERequest::Post);
		}
		
		sleep(pArg->m_delaySeconds);
	}
	
	std::cout << "POST sender thread exit" << std::endl;

	return 0;
}

// Thread procedure to send keep-alive HEAD requests.
void *tpKeepAliveTimer(void *arg)
{
	KeepAliveArgs *pArg = (KeepAliveArgs *)arg;

	while (!pArg->m_exitThread)
	{
		std::shared_ptr<asio::ip::tcp::socket> spSock = pArg->m_spSocket.lock();

		if (spSock)
		{
			exchangeMessages(*spSock, ERequest::Head);
		}
		
		sleep(pArg->m_delaySeconds);
	}
	
	std::cout << "Keep-alive thread exit" << std::endl;
	
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

std::string getHeadRequestKeepAlive()
{
	return "HEAD /index.html HTTP/1.1\r\n"
           "Accept: text/html\r\n"
           "Keep-Alive: timeout=5, max=1000\r\n"
           "Host: somehost.com\r\n\r\n";
}

std::string getPostRequest()
{
	std::string request;

	try
	{
		std::lock_guard<std::mutex> lock(reloadPostArgs.m_requestLock);
		
		request = reloadPostArgs.m_request;
	}
	catch (std::logic_error&)
	{
		std::cout << __FUNCTION__ << ": lock_guard exception";
	}
	
	return request;
}

void loadPostRequest(const std::string& filePath)
{
	std::ifstream fs(filePath);
		
	if (fs.fail())
	{
		std::cerr << "Failed to load POST request from the file "
		          << "\"" << filePath << "\"\n";
		return;
	}
	
	std::stringstream buffer;
	buffer << fs.rdbuf();
	
	fs.close();
	
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
		std::lock_guard<std::mutex> lock(reloadPostArgs.m_requestLock);
	
		//reloadPostArgs.m_request = buffer.str();
		reloadPostArgs.m_request = request;
	}
	catch (std::logic_error&)
	{
		std::cout << "POST timer thread: lock_guard exception";
	}
}

void exchangeMessages(asio::ip::tcp::socket& sock, ERequest requestType)
//void exchangeMessages(std::shared_ptr<asio::ip::tcp::socket>& spSock, ERequest requestType)
{
	std::string request;
	
	switch (requestType)
	{
		case ERequest::Head:
			//request = getHeadRequest();
			request = getHeadRequestKeepAlive();
			break;
		case ERequest::Post:
			request = getPostRequest();
			break;
		default:
			std::cerr << __FUNCTION__ << ": invalid message type\n";
			return;
	}
	
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

