#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <signal.h>
#include <time.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include "common.h"
#include "cmdArgs.h"
#include "responseReloader.h"
#include "intervalTimer.h"
#include "serverAsio.h"

///////////////////////////////////////////////////////////////////////

using namespace boost;

///////////////////////////////////////////////////////////////////////

std::mutex g_condMutex;

bool g_cond = {false};

std::condition_variable g_cv;

bool checkCondVar()
{
	return g_cond;
}

///////////////////////////////////////////////////////////////////////

// Thread procedure to reload the POST response from file.
void *tpReloadResponse(void *arg);

bool respond(asio::ip::tcp::socket& sock, ERequest requestType);

std::string getHeadResponse();

ERequest getMessageType(const std::string& msg);

bool exchangeMessages(UPSocket& sock, unsigned short serverPort, unsigned short clientPort);

void saveRequestToFile(const std::string& request, unsigned short serverPort, unsigned short clientPort);

void sigHandler(int arg);

///////////////////////////////////////////////////////////////////////

// Helper object to reload POST responses.
ResponseReloader g_responseReloader;

pthread_t g_tidMain = {};

pthread_t g_tidrr = {};

// Timer to reload POST responses.
std::unique_ptr<IntervalTimer<ResponseReloader> > g_spTimerPostReload;

// Boost.Asio server.
std::unique_ptr<ServerAsio> g_spServer;

///////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
	signal(SIGINT, sigHandler);
	
	g_tidMain = pthread_self();

    CmdLineArguments args;

    if (!CmdArgsHandler::parse(argc, argv, args))
    {
    	return 1;
    }

    g_responseReloader.m_responseFilePath = args.m_responseFilePath;

    g_responseReloader.m_delaySeconds = args.m_reloadSeconds;
    
    // Initial loading of the POST response.
    g_responseReloader.loadPostResponse();
    
    g_spTimerPostReload = std::make_unique<IntervalTimer<ResponseReloader> >(&g_responseReloader);

	try
	{
		int res = pthread_create(&g_tidrr, nullptr, &tpReloadResponse, nullptr);
			
		if (0 != res)
		{
			std::cerr << "Failed to create POST response reloader thread: " << res << '\n';
			return 2;
		}
		
		std::cout << "Started the POST response reloader thread" << std::endl;
		
		g_spServer = std::make_unique<ServerAsio>(args.m_port);

		g_spServer->listen();
		
		std::cout << "Press Ctrl+C to stop the server" << std::endl;

		while (true)    // accept client connections
		{
			UPSocket spSock = g_spServer->accept();

			std::string clientIp = spSock->remote_endpoint().address().to_string();
			
			unsigned short clientPort = spSock->remote_endpoint().port();
			
			std::cout << "Accepted client connection: " << clientIp << ":" << clientPort << std::endl;

			while (true)
			{
				if (!exchangeMessages(spSock, args.m_port, clientPort))
				{
					break;
				}
			}
			
			std::cout << "Client " << clientIp << " closed the connection" << std::endl;
			
			spSock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
			
			spSock->close();
		}   // accept client connections
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
	std::cout << "\nCtrl+C signal handler. Cleaning up..." << std::endl;

	// Notify the POST response reloader thread to exit.
	{
		std::unique_lock<std::mutex> lock(g_condMutex);
		g_cond = true;
		g_cv.notify_all();
	}

	g_spTimerPostReload->disarmAndDelete();
	
	g_spServer->close();
	
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
	
	// The main thread is stuck, usually on accept().
	// pthread_cancel() doesn't work, at least in Ubuntu.
	pthread_kill(g_tidMain, SIGKILL);
}

void *tpReloadResponse(void *arg)
{
	g_spTimerPostReload->create();
	g_spTimerPostReload->set();
	
	// TODO: temp
	std::cout << "tproc: waiting for condition variable..." << std::endl;

	{
		std::unique_lock<std::mutex> lock {g_condMutex};
		g_cv.wait(lock, checkCondVar);
	}

	// TODO: temp
	std::cout << "tproc: finished waiting for condition variable" << std::endl;
	
	std::cout << "POST response reload thread exit" << std::endl;
	
	return 0;
}

bool exchangeMessages(UPSocket& spSock, unsigned short serverPort, unsigned short clientPort)
{
	std::string fromClient = receiveData(*spSock);

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

	if (!respond(*spSock, requestType))
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
			response = g_responseReloader.getPostResponse();
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

