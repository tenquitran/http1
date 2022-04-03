#include <iostream>
#include <boost/asio.hpp>
#include "common.h"

///////////////////////////////////////////////////////////////////////

using namespace boost;

///////////////////////////////////////////////////////////////////////

bool receiveData(asio::ip::tcp::socket& sock, ERequest& requestType);

bool respond(asio::ip::tcp::socket& sock, ERequest requestType);

///////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
	// TODO: hard-coded
    const unsigned short port = 8976;
    
    ERequest requestType = ERequest::Undefined;

	try
	{
		asio::ip::tcp protocol = asio::ip::tcp::v4();
		
		asio::ip::address serverIp = asio::ip::address_v4::any();

		asio::ip::tcp::endpoint ep = asio::ip::tcp::endpoint(serverIp, port);
		
		asio::io_service io;
		
		asio::ip::tcp::acceptor acceptor = asio::ip::tcp::acceptor(io, ep);

		acceptor.listen();
		
		// TODO: implement more graceful exit
		std::cout << "Press Ctrl+C to stop the server" << std::endl;
		
		while (true)
		{
			asio::ip::tcp::socket sock(io);
			
			acceptor.accept(sock);
			
			std::string clientIp = sock.remote_endpoint().address().to_string();
			
			unsigned short clientPort = sock.remote_endpoint().port();
			
			std::cout << "Accepted client connection: " << clientIp << ":" << clientPort << std::endl;

			if (receiveData(sock, requestType))
			{
				if (!respond(sock, requestType))
				{
					std::cerr << "Failed to respond to the client\n";
				}
			}
			else
			{
				std::cerr << "Failed to receive data from the client\n";
			}
			
			if (receiveData(sock, requestType))
			{
				if (!respond(sock, requestType))
				{
					std::cerr << "Failed to respond to the client\n";
				}
			}
			else
			{
				std::cerr << "Failed to receive data from the client\n";
			}
		}
	}
	catch (system::system_error& ex)
	{
		std::cerr << "Exception: " << ex.what() << '\n';
		//std::cerr << "Exception: " << ex.what() << " (" << ex.code() << ")\n";
		return 1;
	}
	
    return 0;
}

bool receiveData(asio::ip::tcp::socket& sock, ERequest& requestType)
{
	try
	{
		requestType = ERequest::Undefined;
	
		// Request length buffer.
		std::vector<unsigned char> lenBuff(2);
	
		std::size_t cbReceived = sock.receive(
			boost::asio::buffer(lenBuff),
			0);
		
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
		
		std::string reqStr;
		reqStr.reserve(reqLen);
		
		for (auto c : req)
		{
			std::cout << c;
			reqStr += c;
		}
		std::cout << std::endl << std::endl;

		// Determine whether the client have sent a HEAD or a POST request.

		if (0 == reqStr.find("HEAD"))
		{
			requestType = ERequest::Head;
		}
		else if (0 == reqStr.find("POST"))
		{
			requestType = ERequest::Post;
		}
		
		std::cout << "Received a " << requestTypeToStr(requestType) << " request" << std::endl;
		
		if (ERequest::Undefined == requestType)
		{
			std::cerr << "Invalid request type\n";
			return false;
		}
	}
	catch (system::system_error& ex)
	{
		std::cerr << "Exception on receiving: " << ex.what() << '\n';
		//std::cerr << "Exception: " << ex.what() << " (" << ex.code() << ")\n";
		return false;
	}
	
	return true;
}

bool respond(asio::ip::tcp::socket& sock, ERequest requestType)
{
	std::string response;
	
	switch (requestType)
	{
		case ERequest::Head:
			response = "HTTP/1.1 200 OK\n"
                       "Content-Length: 19\n"
                       "Content-Type: application/json";
			break;
		case ERequest::Post:
			response = "HTTP/1.1 200 OK\n"
                       "Content-Length: 19\n"
                       "Content-Type: application/json\n\n"
                       "{\"success\":\"true\"}";
			break;
		default:
			std::cerr << __FUNCTION__ << ": unsupported request type\n";
			return false;
	}
	
	std::string reqStr = requestTypeToStr(requestType);
	
	std::cout << reqStr << " response length: " << response.length() << std::endl;

	if (sendMsgLength(sock, response.length()))
	{
		std::cout << "Send length of the " << reqStr << " response to the server" << std::endl;
	}
	else
	{
		std::cerr << "Failed to send length of the " << reqStr << " response to the server\n";
	}
	
	if (sendMsg(sock, response))
	{
		std::cout << "Send " << reqStr << " response to the server" << std::endl;
	}
	else
	{
		std::cerr << "Failed to send " << reqStr << " response to the server\n";
	}
	
	return true;
}

