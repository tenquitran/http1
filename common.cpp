#include "common.h"
#include <iostream>

///////////////////////////////////////////////////////////////////////

using namespace boost;

///////////////////////////////////////////////////////////////////////


std::string requestTypeToStr(ERequest rt)
{
	switch (rt)
	{
		case ERequest::Head:
			return "HEAD";
		case ERequest::Post:
			return "POST";
		default:
			return "Unknown";
	}
}

bool sendMsgLength(boost::asio::ip::tcp::socket& sock, int len)
{
	std::size_t cbSent = {};
	
	uint16_t rl = htons(len);
	
	size_t cbrl = sizeof(rl);

	std::vector<unsigned char> v(2);

	v[0] = rl >> 8;
	v[1] = rl & 0x0F;
	
	//std::cout << "cbrl: " << std::hex << rl << std::endl;
	//std::cout << "cbrl[0]: " << std::hex << (rl >> 8)     << std::endl;
	//std::cout << "cbrl[1]: " << std::hex << (rl & 0x0F) << std::endl;

	try
	{
		cbSent = sock.send(
			asio::buffer(v, v.size()), 
			0);
		
		std::cout << "Sent " << cbSent << " bytes of " << cbrl << std::endl;
	}
	catch (system::system_error& ex)
	{
		std::cerr << "Exception on sending HEADER: " << ex.what() << '\n';
		return false;
	}
	
	return (cbrl == cbSent);
}

bool sendMsg(boost::asio::ip::tcp::socket& sock, const std::string& msg)
{
	const size_t cbReq = msg.length();
	
	std::size_t cbSent = {};

	try
	{
		cbSent = sock.send(
			asio::buffer(msg.c_str(), cbReq), 
			0);
		
		std::cout << "Sent " << cbSent << " bytes of " << cbReq << std::endl;
	}
	catch (system::system_error& ex)
	{
		std::cerr << "Exception on sending HEADER: " << ex.what() << '\n';
		return false;
	}
	
	return (cbReq == cbSent);
}

std::string receiveData(asio::ip::tcp::socket& sock)
{
	std::string received;

	try
	{
		// Request length buffer.
		std::vector<unsigned char> lenBuff(2);
	
		std::size_t cbReceived = sock.receive(
			boost::asio::buffer(lenBuff),
			0);
		
		if (cbReceived < 2)
		{
			std::cerr << "Failed to receive message length\n";
			return "";
		}
		
		uint16_t n = lenBuff[1];
		n += lenBuff[0] << 8;
		
		uint16_t reqLen = ntohs(n);

		std::cout << "Request length: " << reqLen << std::endl;
	
		// Request buffer.
		std::vector<char> req(reqLen);
	
		cbReceived = sock.receive(
			boost::asio::buffer(req),
			0);

		std::cout << "Received:\n\n";

		received.reserve(reqLen);
		
		for (auto c : req)
		{
			std::cout << c;
			received += c;
		}
		std::cout << std::endl << std::endl;
	}
	catch (system::system_error& ex)
	{
		std::cerr << "Exception on receiving: " << ex.what() << '\n';
		return "";
	}
	
	return received;
}

std::string getHeadRequest()
{
	return "HEAD /index.html HTTP/1.1\r\n"
           "Accept: application/json\r\n"
           "Host: somehost.com\r\n\r\n";
}

std::string getHeadRequestKeepAlive()
{
	return "HEAD /index.html HTTP/1.1\r\n"
           "Accept: text/html\r\n"
           "Keep-Alive: timeout=5, max=1000\r\n"
           "Host: somehost.com\r\n\r\n";
}

void exchangeMessages(asio::ip::tcp::socket& sock, ERequest requestType, const std::string& postRequest /*= ""*/)
{
	std::string request;
	
	switch (requestType)
	{
		case ERequest::Head:
			//request = getHeadRequest();
			request = getHeadRequestKeepAlive();
			break;
		case ERequest::Post:
			if (postRequest.empty())
			{
				std::cerr << __FUNCTION__ << ": empty POST request\n";
				return;
			}
		
			request = postRequest;
			break;
		default:
			std::cerr << __FUNCTION__ << ": invalid message type\n";
			return;
	}
	
	// Send the request length.

	std::string reqTypeStr = requestTypeToStr(requestType);

	std::cout << reqTypeStr << " request length: " << request.length() << std::endl;

	if (sendMsgLength(sock, request.length()))
	{
		std::cout << "Sent length of the " << reqTypeStr << " request to the server" << std::endl;
	}
	else
	{
		std::cerr << "Failed to send length of the " << reqTypeStr << " request to the server\n";
	}
	
	// Send the request itself.

	if (sendMsg(sock, request))
	{
		std::cout << "Sent " << reqTypeStr << " request to the server" << std::endl;
	}
	else
	{
		std::cerr << "Failed to send " << reqTypeStr << " request to the server\n";
	}
	
	// Receive the server response.
	
	std::string response = receiveData(sock);
	
	if (response.length() > 0)
	{
		std::cerr << "Received " << reqTypeStr << " response from the server\n";
	}
	else
	{
		std::cerr << "Failed to receive " << reqTypeStr << " response from the server\n";
	}
}

