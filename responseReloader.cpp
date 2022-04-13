#include <iostream>
#include <fstream>
#include <sstream>
#include "responseReloader.h"

///////////////////////////////////////////////////////////////////////


ResponseReloader::ResponseReloader()
	: m_delaySeconds(5)
{
}

void ResponseReloader::periodicAction()
{
	loadPostResponse();
}

void ResponseReloader::loadPostResponse()
{
	// TODO: temp
	std::cout << "RR" << std::endl;
	
	std::ifstream fs(m_responseFilePath);
		
	if (fs.fail())
	{
		std::cerr << "Failed to load POST response from the file "
		          << "\"" << m_responseFilePath << "\"\n";
		return;
	}
	
	std::stringstream buffer;
	buffer << fs.rdbuf();
	
	fs.close();
	
	std::string payload = buffer.str();

	// 2 bytes for "\r\n" following the payload.
	std::string payloadLen = std::to_string(payload.length() + 2);

	std::string response = "HTTP/1.1 200 OK\r\n"
						   "Content-Length: " + payloadLen + "\r\n"
						   "Content-Type: application/json; utf-8\r\n";
	
	response += payload + "\r\n\r\n";

	try
	{
		std::lock_guard<std::mutex> lock(m_responseLock);
	
		m_response = response;
	}
	catch (std::logic_error&)
	{
		std::cout << "POST response reloader thread: lock_guard exception";
	}
}

std::string ResponseReloader::getPostResponse()
{
	std::string request;

	try
	{
		std::lock_guard<std::mutex> lock(m_responseLock);
		
		request = m_response;
	}
	catch (std::logic_error&)
	{
		std::cout << __FUNCTION__ << ": lock_guard exception";
	}
	
	return request;
}

