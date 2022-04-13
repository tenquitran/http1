#ifndef _RESPONSE_RELOADER_H
#define _RESPONSE_RELOADER_H

#include <mutex>
#include <string>


// Helper class for POST response reloading.
class ResponseReloader
{
public:
	ResponseReloader();
	
	// Action performed by periodic timer.
	void periodicAction();
	
	// Load POST response from the file.
	void loadPostResponse();
	
	// Get POST response as a string.
	std::string getPostResponse();
	
public:
	int m_delaySeconds = {};
	
	std::mutex m_responseLock;
	
	std::string m_response;
	
	std::string m_responseFilePath;
};

#endif

