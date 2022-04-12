#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include "common.h"
#include "cmdArgs.h"
#include "workerThread.h"

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

pthread_t g_tidMain = {};

pthread_t g_tidrr = {};

std::unique_ptr<asio::ip::tcp::acceptor> g_spAcceptor;

///////////////////////////////////////////////////////////////////////

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
	
	g_tidMain = pthread_self();
	
	// TODO: temp: testing worker thread code
#if 1
	PostReloader pr("testOne");
	if (!pr.launch())
	{
		std::cout << "Failed to launch wt" << std::endl;
	}
	else
	{
		std::cout << "wt launched" << std::endl;
	}
	void *pExit;
	int res = pthread_join(pr.m_tid, &pExit);
#endif

    CmdLineArguments args;

    if (!CmdArgsHandler::parse(argc, argv, args))
    {
    	return 1;
    }

    reloadResponseArgs.m_responseFilePath = args.m_responseFilePath;

    reloadResponseArgs.m_delaySeconds = args.m_reloadSeconds;
    
    // Initial loading of the POST response.
    loadPostResponse(reloadResponseArgs.m_responseFilePath);

	try
	{
		int res = pthread_create(&g_tidrr, nullptr, &tpReloadResponse, (void *)&reloadResponseArgs);
			
		if (0 != res)
		{
			std::cerr << "Failed to create POST response reloader thread: " << res << '\n';
			return 2;
		}
		
		std::cout << "Started the POST response reloader thread" << std::endl;
		
		asio::ip::address serverIp = asio::ip::address_v4::any();

		asio::ip::tcp::endpoint ep = asio::ip::tcp::endpoint(serverIp, args.m_port);
		
		asio::io_service io;
		
		g_spAcceptor = std::make_unique<asio::ip::tcp::acceptor>(io, ep);

		g_spAcceptor->listen();
		
		std::cout << "Press Ctrl+C to stop the server" << std::endl;

		while (false == reloadResponseArgs.m_exitThread.load())
		{
			asio::ip::tcp::socket sock(io);
			
			g_spAcceptor->accept(sock);

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
		}
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
	// Notify the POST response reloader thread to exit.
	reloadResponseArgs.m_exitThread.store(true);
	
	g_spAcceptor->close();
	
	void *rrExit;
			
	int res = pthread_join(g_tidrr, &rrExit);
	
	if (0 != res)
	{
		std::cerr << "Failed to join the POST response reloader thread: " << res << '\n';
	}
	else
	{
		std::cout << "POST response reloader thread returned " << (long)rrExit << std::endl;
	}
	
	std::cout << "Canceling the main thread" << std::endl;
	
	pthread_cancel(g_tidMain);
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
	return "HTTP/1.1 200 OK\r\n"
           "Content-Length: 27\r\n"
           "Content-Type: text/html; charset=UTF-8\r\n\r\n";
}

std::string getPostResponse()
{
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

