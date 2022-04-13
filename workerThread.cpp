#include <iostream>
#include <pthread.h>
#include "workerThread.h"

///////////////////////////////////////////////////////////////////////

// TODO: not used
#if 0
WorkerThread::WorkerThread(const std::string& name)
	: m_name(name)
{
}

bool WorkerThread::launch()
{
	//int res = pthread_create(&m_tid, nullptr, &tpReloadResponse, (void *)&reloadResponseArgs);
	int res = pthread_create(&m_tid, nullptr, &WorkerThread::tpHelper, (void *)this);
			
	if (0 != res)
	{
		// TODO: use proper thread name
		std::cerr << "Failed to create " << m_name << " thread: " << res << '\n';
		return false;
	}
	
	return true;
}

void* WorkerThread::tpHelper(void* context)
{
    return ((WorkerThread *)context)->threadProc((WorkerThread *)context);
}
#endif

