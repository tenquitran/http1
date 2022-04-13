#ifndef _REQUEST_RELOADER_H
#define _REQUEST_RELOADER_H

#include <mutex>
#include <string>


// Client: helper class for POST request reloading.
class RequestReloader
{
public:
	RequestReloader();
	
	// Action performed by periodic timer.
	void periodicAction();
	
	// Load POST request from the file.
	void loadPostRequest();
	
	// Get POST request as a string.
	std::string getPostRequest();
	
public:
	int m_delaySeconds = {};

	std::mutex m_requestLock;
	
	std::string m_request;
	
	std::string m_requestFilePath;
};

#endif

