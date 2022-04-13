#include <iostream>
#include <fstream>
#include <sstream>
#include "requestReloader.h"

///////////////////////////////////////////////////////////////////////


RequestReloader::RequestReloader()
	: m_delaySeconds(5)
{
}

void RequestReloader::periodicAction()
{
	loadPostRequest();
}

void RequestReloader::loadPostRequest()
{
	std::ifstream fs(m_requestFilePath);
		
	if (fs.fail())
	{
		std::cerr << "Failed to load POST request from the file "
		          << "\"" << m_requestFilePath << "\"\n";
		return;
	}
	
	std::stringstream buffer;
	buffer << fs.rdbuf();
	
	fs.close();
	
	std::string payload = buffer.str();

	// 2 bytes for "\r\n" following the payload.
	std::string payloadLen = std::to_string(payload.length() + 2);

	std::string request = "POST /test HTTP/1.1\r\n"
						  "Host: somehost.com\r\n"
						  "Content-Type: application/json; utf-8\r\n"
						  "Content-Length: " + payloadLen + "\r\n"
						  "Accept: application/json\r\n\r\n";
	
	request += payload + "\r\n\r\n";

	try
	{
		std::lock_guard<std::mutex> lock(m_requestLock);

		m_request = request;
	}
	catch (std::logic_error&)
	{
		std::cout << "POST timer thread: lock_guard exception";
	}
}

std::string RequestReloader::getPostRequest()
{
	std::string request;

	try
	{
		std::lock_guard<std::mutex> lock(m_requestLock);
		
		request = m_request;
	}
	catch (std::logic_error&)
	{
		std::cout << __FUNCTION__ << ": lock_guard exception";
	}
	
	return request;
}

