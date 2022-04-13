#include <iostream>
#include <mutex>
#include "postReloaderThread.h"

///////////////////////////////////////////////////////////////////////


// TODO: not used
#if 0
PostReloaderThread::PostReloaderThread(const std::string& name)
	: WorkerThread(name)
{
}

void* PostReloaderThread::threadProc(void* arg)
{
	g_spTimerPostReload->create();
	g_spTimerPostReload->set();
	
	// TODO: temp
	std::cout << m_name << " thread: waiting for condition variable..." << std::endl;

	{
		std::unique_lock<std::mutex> lock {g_condMutex};
		g_cv.wait(lock, checkCondVar);
	}

	std::cout << m_name << " thread exit" << std::endl;
	
	return 0;
}
#endif

