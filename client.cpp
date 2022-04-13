#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <signal.h>
#include <time.h>
#include <boost/asio.hpp>
#include "common.h"
#include "cmdArgs.h"
#include "intervalTimer.h"
#include "requestReloader.h"
#include "postSender.h"
#include "keepAlive.h"

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

// Helper object to reload POST requests.
RequestReloader g_requestReloader;

// Helper objects to send POST requests.
PostSender g_postSender;

// Helper objects to send keep-alive HEAD requests.
KeepAlive g_keepAlive;

///////////////////////////////////////////////////////////////////////

// Thread procedure to send keep-alive HEAD requests.
void *tpKeepAliveTimer(void *arg);

// Thread procedure to reload the POST request from file.
void *tpReloadPost(void *arg);

// Thread procedure to send the POST request.
void *tpSendPost(void *arg);

void sigHandler(int arg);

///////////////////////////////////////////////////////////////////////

// Timer to reload POST requests.
std::unique_ptr<IntervalTimer<RequestReloader> > g_spTimerPostReload;

// Timer to send POST requests.
std::unique_ptr<IntervalTimer<PostSender> > g_spTimerPostSend;

// Timer to send keep-alive HEAD requests.
std::unique_ptr<IntervalTimer<KeepAlive> > g_spTimerKeepAlive;

pthread_t g_tidKeepAlive;
pthread_t g_tidPostReloader;
pthread_t g_tidPostSender;

bool g_keepAliveMode = {false};

///////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
    signal(SIGINT, sigHandler);

    CmdLineArguments args;

	if (!CmdArgsHandler::parse(argc, argv, args))
	{
		return 1;
	}
	
	g_keepAliveMode = args.m_keepAliveMode;

    g_requestReloader.m_requestFilePath = args.m_requestFilePath;

    g_requestReloader.m_delaySeconds = args.m_reloadSeconds;
    
    g_postSender.m_delaySeconds = args.m_postSeconds;
    
    g_keepAlive.m_delaySeconds = args.m_keepAliveSeconds;
    
    g_postSender.initialize(&g_requestReloader);
    
    g_spTimerPostReload = std::make_unique<IntervalTimer<RequestReloader> >(&g_requestReloader);
    
    g_spTimerPostSend = std::make_unique<IntervalTimer<PostSender> >(&g_postSender);
    
    g_spTimerKeepAlive = std::make_unique<IntervalTimer<KeepAlive> >(&g_keepAlive);
    
    // Initial loading of the POST request.
    g_requestReloader.loadPostRequest();

	try
	{
		asio::ip::tcp::endpoint ep(
			asio::ip::address::from_string(args.m_ipAddress), 
			args.m_port);
		
		asio::io_service io;
		
		std::shared_ptr<asio::ip::tcp::socket> sock = 
			std::make_shared<asio::ip::tcp::socket>(io, ep.protocol());
		
		std::cout << "Trying to connect to " << args.m_ipAddress 
		          << ":" << args.m_port << std::endl;
		
		sock->connect(ep);
		
		g_postSender.m_spSocket = sock;
		g_keepAlive.m_spSocket  = sock;

		if (args.m_keepAliveMode)
		{
			int res = pthread_create(&g_tidKeepAlive, nullptr, &tpKeepAliveTimer, nullptr);
			
			if (0 != res)
			{
				std::cerr << "Failed to create keep-alive thread: " << res << '\n';
				return 2;
			}
			
			std::cout << "Started the keep-alive thread" << std::endl;
		}
		else
		{
			int res = pthread_create(&g_tidPostReloader, nullptr, &tpReloadPost, nullptr);
			
			if (0 != res)
			{
				std::cerr << "Failed to create POST reloader thread: " << res << '\n';
				return 2;
			}
			
			std::cout << "Started the POST reloader thread" << std::endl;
			
			res = pthread_create(&g_tidPostSender, nullptr, &tpSendPost, nullptr);
			
			if (0 != res)
			{
				std::cerr << "Failed to create POST sender thread: " << res << '\n';
				return 2;
			}
			
			std::cout << "Started the POST sender thread" << std::endl;
		}

		{
			std::unique_lock<std::mutex> lock {g_condMutex};
			g_cv.wait(lock, checkCondVar);
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
	std::cout << "\nCtrl+C signal handler. Preparing to stop..." << std::endl;

	// Notify the worker threads to exit.
	{
		std::unique_lock<std::mutex> lock(g_condMutex);
		g_cond = true;
		g_cv.notify_all();
	}
	
	g_spTimerPostReload->disarmAndDelete();
	
	// Wait for the worker threads to exit.
		
	if (g_keepAliveMode)
	{
		void *keepAliveExit;
	
		int res = pthread_join(g_tidKeepAlive, &keepAliveExit);
		
		if (0 != res)
		{
			std::cerr << "Failed to join the keep-alive thread: " << res << '\n';
		}
		else
		{
			std::cout << "Keep-alive timer thread returned " << (long)keepAliveExit << std::endl;
		}
	}
	else
	{
		void *postReloaderExit;
		
		int res = pthread_join(g_tidPostReloader, &postReloaderExit);
		
		if (0 != res)
		{
			std::cerr << "Failed to join the POST reloader thread: " << res << '\n';
		}
		else
		{
			std::cout << "POST reloader thread returned " << (long)postReloaderExit << std::endl;
		}

		void *postSenderExit;
		
		res = pthread_join(g_tidPostSender, &postSenderExit);
		
		if (0 != res)
		{
			std::cerr << "Failed to join the POST sender thread: " << res << '\n';
		}
		else
		{
			std::cout << "POST sender thread returned " << (long)postSenderExit << std::endl;
		}
	}
}

// Thread procedure to reload the POST request from file.
void *tpReloadPost(void *arg)
{
	g_spTimerPostReload->create();
	g_spTimerPostReload->set();

	{
		std::unique_lock<std::mutex> lock {g_condMutex};
		g_cv.wait(lock, checkCondVar);
	}

	std::cout << "POST reload thread exit" << std::endl;
	
	return 0;
}

// Thread procedure to send the POST request.
void *tpSendPost(void *arg)
{
	g_spTimerPostSend->create();
	g_spTimerPostSend->set();

	{
		std::unique_lock<std::mutex> lock {g_condMutex};
		g_cv.wait(lock, checkCondVar);
	}
	
	std::cout << "POST sender thread exit" << std::endl;

	return 0;
}

// Thread procedure to send keep-alive HEAD requests.
void *tpKeepAliveTimer(void *arg)
{
	g_spTimerKeepAlive->create();
	g_spTimerKeepAlive->set();

	{
		std::unique_lock<std::mutex> lock {g_condMutex};
		g_cv.wait(lock, checkCondVar);
	}
	
	std::cout << "Keep-alive thread exit" << std::endl;
	
	return 0;
}

