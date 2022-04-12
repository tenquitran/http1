#ifndef _WORKER_THREAD_H
#define _WORKER_THREAD_H

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

// TODO: move to separate .h and .cpp files

// Server: POST response reloader thread.
class PostReloader
	: public WorkerThread
{
public:
	// Parameters: name - thread name.
	explicit PostReloader(const std::string& name);

	// Thread procedure.
	virtual void* threadProc(void* arg) override;
	
private:
	// TODO: temp
	int m_num = {123};
};

#endif

