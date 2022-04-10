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

struct ReloadResponseArgs
{
	ReloadResponseArgs()
		: m_delaySeconds(5)
	{
		m_exitThread.store(false);
	}

	std::atomic<bool> m_exitThread;

	int m_delaySeconds = {};
	
	std::mutex m_responseLock;
	
	std::string m_response;
	
	std::string m_responseFilePath;
} reloadResponseArgs;


struct CmdLineArguments
{
	unsigned short m_port = {};
	
	int m_reloadSeconds = {};
	
	std::string m_responseFilePath;
};

///////////////////////////////////////////////////////////////////////

void displayUsage(const char* programName);

bool parseCmdLineArgs(int argc, char* argv[], CmdLineArguments& args);

// Thread procedure to reload the POST response from file.
void *tpReloadResponse(void *arg);

void loadPostResponse(const std::string& filePath);

bool respond(asio::ip::tcp::socket& sock, ERequest requestType);

std::string getHeadResponse();

std::string getPostResponse();

ERequest getMessageType(const std::string& msg);

bool exchangeMessages(asio::ip::tcp::socket& sock, unsigned short serverPort, unsigned short clientPort);

void saveRequestToFile(const std::string& request, unsigned short serverPort, unsigned short clientPort);

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
    
    // TODO: reload and response options
    
    reloadResponseArgs.m_responseFilePath = args.m_responseFilePath;

    reloadResponseArgs.m_delaySeconds = args.m_reloadSeconds;
    
    // Initial loading of the POST response.
    loadPostResponse(reloadResponseArgs.m_responseFilePath);

    ERequest requestType = ERequest::Undefined;

	try
	{
		pthread_t tidrr;
		
		int res = pthread_create(&tidrr, nullptr, &tpReloadResponse, (void *)&reloadResponseArgs);
			
		if (0 != res)
		{
			std::cerr << "Failed to create POST response reloader thread: " << res << '\n';
			return 2;
		}
		
		std::cout << "Started the POST response reloader thread" << std::endl;
	
		asio::ip::tcp protocol = asio::ip::tcp::v4();
		
		asio::ip::address serverIp = asio::ip::address_v4::any();

		asio::ip::tcp::endpoint ep = asio::ip::tcp::endpoint(serverIp, args.m_port);
		
		asio::io_service io;
		
		asio::ip::tcp::acceptor acceptor = asio::ip::tcp::acceptor(io, ep);

		acceptor.listen();
		
		std::cout << "Press Ctrl+C to stop the server" << std::endl;

		while (false == reloadResponseArgs.m_exitThread.load())
		{
#if 1
			asio::ip::tcp::socket sock(io);
			
			// TODO: temp
			std::cout << "Before accept()" << std::endl;

			acceptor.accept(sock);
			
			// TODO: temp
			std::cout << "After accept()" << std::endl;

			std::string clientIp = sock.remote_endpoint().address().to_string();
			
			unsigned short clientPort = sock.remote_endpoint().port();
			
			std::cout << "Accepted client connection: " << clientIp << ":" << clientPort << std::endl;

			while (true)
			{
				if (!exchangeMessages(sock, args.m_port, clientPort))
				{
					break;
				}
			}
			
			std::cout << "Client " << clientIp << " closed the connection" << std::endl;
			
			sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
			
			sock.close();
#else
			sleep(1);
#endif
		}
		
Exit:
		// TODO: temp
		std::cout << "Exit label" << std::endl;

		void *rrExit;
			
		res = pthread_join(tidrr, &rrExit);
		
		if (0 != res)
		{
			std::cerr << "Failed to join the POST response reloader thread: " << res << '\n';
		}
		else
		{
			std::cout << "POST response reloader thread returned " << (long)rrExit << std::endl;
		}

		acceptor.close();
	}
	catch (system::system_error& ex)
	{
		std::cerr << "Exception: " << ex.what() << '\n';
		return 3;
	}
	
    return 0;
}

void sigHandler(int arg)
{
	// TODO: temp
	std::cout << "Ctrl+C" << std::endl;

	// Notify the POST response reloader thread to exit.
	reloadResponseArgs.m_exitThread.store(true);
}

void displayUsage(const char* programName)
{
	// --port=portnumber - the number of listener port
	// --reload=XX - every XX seconds reload the file where the HTTP Response is stored and use it when process next HTTP Requests
	// --response=/path/to/Response.json - path to Response.html used by server when receive Response from client.

	std::cout << "Usage:\n\n"
			  << programName 
			  << " --port=<port_number> --reload=<seconds> "
			     "--response=/path/to/Response.json\n"

			  << "\nExample:\n\n"
	          << programName 
	          << " --port=8976 --reload=5 "
	             "--response=responses/post_response.json"
	          << std::endl;
}

bool parseCmdLineArgs(int argc, char* argv[], CmdLineArguments& args)
{
	using namespace boost::program_options;

	if (argc < 4)
		{return false;}

    options_description desc{"CommandLineOptions"};
    
    desc.add_options()
      ("port",     value<int>()->default_value(8976),       "Port")
      ("reload",   value<int>()->default_value(5),          "Reload")
      ("response", value<std::string>()->default_value(""), "ResponsePath");
      
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

	if (vm.count("response"))
	{
		std::cout << "Response path: " << vm["response"].as<std::string>() << std::endl;
		args.m_responseFilePath = vm["response"].as<std::string>();
	}
	else
		{return false;}

	return true;
}

void *tpReloadResponse(void *arg)
{
	ReloadResponseArgs *pArg = (ReloadResponseArgs *)arg;

	while (!pArg->m_exitThread)
	{
		loadPostResponse(pArg->m_responseFilePath);

		sleep(pArg->m_delaySeconds);
	}
	
	std::cout << "POST response reload thread exit" << std::endl;
	
	return 0;
}

void loadPostResponse(const std::string& filePath)
{
	std::ifstream fs(filePath);
		
	if (fs.fail())
	{
		std::cerr << "Failed to load POST response from the file "
		          << "\"" << filePath << "\"\n";
		return;
	}
	
	std::stringstream buffer;
	buffer << fs.rdbuf();
	
	fs.close();
	
	std::string payload = buffer.str();

	// 2 bytes for "\r\n" following the payload.
	std::string payloadLen = std::to_string(payload.length() + 2);

	std::string response = "HTTP/1.1 200 OK\r\n"
						   "Content-Length: " + payloadLen + "\r\n"
						   "Content-Type: application/json; utf-8\r\n";
	
	response += payload + "\r\n\r\n";

	try
	{
		std::lock_guard<std::mutex> lock(reloadResponseArgs.m_responseLock);
	
		reloadResponseArgs.m_response = response;
	}
	catch (std::logic_error&)
	{
		std::cout << "POST response reloader thread: lock_guard exception";
	}
}

bool exchangeMessages(asio::ip::tcp::socket& sock, unsigned short serverPort, unsigned short clientPort)
{
	std::string fromClient = receiveData(sock);

	if (fromClient.length() < 1)
	{
		std::cerr << "Failed to receive data from the client\n";
		return false;
	}
	
	saveRequestToFile(fromClient, serverPort, clientPort);
	
	ERequest requestType = getMessageType(fromClient);

	std::cout << "Received a " << requestTypeToStr(requestType) 
		      << " request from the client" << std::endl;
	
	if (ERequest::Undefined == requestType)
	{
		std::cerr << "Invalid request type\n";
		return false;
	}

	if (!respond(sock, requestType))
	{
		std::cerr << "Failed to respond to the client\n";
		return false;
	}
	
	return true;
}

void saveRequestToFile(const std::string& request, unsigned short serverPort, unsigned short clientPort)
{
	// Output file name: <serverPort>_<clientPort>_<timestamp>.txt
	// , where timestamp is of the format YYYYMMDDHHmmSS

	std::string fileName;

	time_t tm0 = time(nullptr);
	
	tm* ltm = localtime(&tm0);
	
	char buff[100];
	
	snprintf(buff, 100, "%04d%02d%02d%02d%02d%02d", 
		ltm->tm_year + 1900, 
		ltm->tm_mon + 1, 
		ltm->tm_mday, 
		ltm->tm_hour, 
		ltm->tm_min, 
		ltm->tm_sec);
	
	std::string tms(buff);

	fileName = std::to_string(serverPort);
	fileName += "_";
	fileName += std::to_string(clientPort);
	fileName += "_";
	fileName += tms;
	fileName += ".txt";
	
	std::string path = "requestsFromClients/" + fileName;
	
	std::ofstream fs(path);
		
	if (fs.fail())
	{
		std::cerr << "Failed to open file "
		          << "\"" << path << "\"\n";
		return;
	}
	
	fs << request;
	fs.close();
}

bool respond(asio::ip::tcp::socket& sock, ERequest requestType)
{
	std::string response;
	
	switch (requestType)
	{
		case ERequest::Head:
			response = getHeadResponse();
			break;
		case ERequest::Post:
			response = getPostResponse();
			break;
		default:
			std::cerr << __FUNCTION__ << ": unsupported request type\n";
			return false;
	}
	
	std::string reqStr = requestTypeToStr(requestType);
	
	std::cout << reqStr << " response length: " << response.length() << std::endl;

	if (sendMsgLength(sock, response.length()))
	{
		std::cout << "Sent length of the " << reqStr << " response to the client" << std::endl;
	}
	else
	{
		std::cerr << "Failed to send length of the " << reqStr << " response to the client\n";
	}
	
	if (sendMsg(sock, response))
	{
		std::cout << "Sent " << reqStr << " response to the client" << std::endl;
	}
	else
	{
		std::cerr << "Failed to send " << reqStr << " response to the client\n";
	}
	
	return true;
}

ERequest getMessageType(const std::string& msg)
{
	if (0 == msg.find("HEAD"))
	{
		return ERequest::Head;
	}
	else if (0 == msg.find("POST"))
	{
		return ERequest::Post;
	}
	
	return ERequest::Undefined;
}

std::string getHeadResponse()
{
#if 0
	return "HTTP/1.1 200 OK\r\n"
           "Content-Length: 27\r\n"
           "Content-Type: application/json\r\n\r\n";
#else
	return "HTTP/1.1 200 OK\r\n"
           "Content-Length: 27\r\n"
           "Content-Type: text/html; charset=UTF-8\r\n\r\n";
#endif
}

std::string getPostResponse()
{
#if 0
	return "HTTP/1.1 200 OK\r\n"
           "Content-Length: 18\r\n"
           "Content-Type: application/json\r\n\r\n"
           "{\"success\":\"true\"}\r\n\r\n";
#endif

	std::string request;

	try
	{
		std::lock_guard<std::mutex> lock(reloadResponseArgs.m_responseLock);
		
		request = reloadResponseArgs.m_response;
	}
	catch (std::logic_error&)
	{
		std::cout << __FUNCTION__ << ": lock_guard exception";
	}
	
	return request;
}

