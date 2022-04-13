#ifndef _WORKER_THREAD_H
#define _WORKER_THREAD_H

// TODO: not used
#if 0
class WorkerThread
{
public:
	// Parameters: name - thread name.
	explicit WorkerThread(const std::string& name);
	
	// Start the thread.
	// Returns: true on success, false otherwise.
	bool launch();
	
	WorkerThread(const WorkerThread&) = delete;
	WorkerThread& operator=(const WorkerThread&) = delete;
	
private:
	// Thread procedure helper.
    static void* tpHelper(void* context);
    
    // Thread procedure.
	virtual void* threadProc(void* arg) = 0;

// TODO: make private
public:
	const std::string m_name;

	pthread_t m_tid = {};
};
#endif

#endif

