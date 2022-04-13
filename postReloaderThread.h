#ifndef _POST_RELOADER_THREAD_H
#define _POST_RELOADER_THREAD_H

#include "workerThread.h"

// TODO: not used
#if 0
// Server: POST response reloader thread.
class PostReloaderThread
	: public WorkerThread
{
public:
	// Parameters: name - thread name.
	explicit PostReloaderThread(const std::string& name);

	// Thread procedure.
	virtual void* threadProc(void* arg) override;
};
#endif

#endif

