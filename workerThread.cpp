#include <iostream>
#include <pthread.h>
#include "workerThread.h"

///////////////////////////////////////////////////////////////////////


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


///////////////////////////////////////////////////////////////////////
// TODO: move to separate .h and .cpp files
///////////////////////////////////////////////////////////////////////

PostReloader::PostReloader(const std::string& name)
	: WorkerThread(name)
{
}

void* PostReloader::threadProc(void* arg)
{
	PostReloader *pThis = (PostReloader *)arg;
	
	// TODO: implement
	std::cout << "My field is " << pThis->m_num << std::endl;
	
	return 0;
}

