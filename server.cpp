#include <iostream>
#include <boost/asio.hpp>
#include "common.h"

///////////////////////////////////////////////////////////////////////

using namespace boost;

///////////////////////////////////////////////////////////////////////

bool respond(asio::ip::tcp::socket& sock, ERequest requestType);

std::string getHeadResponse();

std::string getPostResponse();

ERequest getMessageType(const std::string& msg);

bool exchangeMessages(asio::ip::tcp::socket& sock);

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
		
		std::cout << "Press Ctrl+C to stop the server" << std::endl;
		
		while (true)
		{
			asio::ip::tcp::socket sock(io);

			acceptor.accept(sock);

			std::string clientIp = sock.remote_endpoint().address().to_string();
			
			unsigned short clientPort = sock.remote_endpoint().port();
			
			std::cout << "Accepted client connection: " << clientIp << ":" << clientPort << std::endl;

			while (true)
			{
				if (!exchangeMessages(sock))
				{
					break;
				}
			}
			
			std::cout << "Client " << clientIp << " closed the connection" << std::endl;
			
			sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
			
			sock.close();
		}
	}
	catch (system::system_error& ex)
	{
		std::cerr << "Exception: " << ex.what() << '\n';
		return 1;
	}
	
    return 0;
}

bool exchangeMessages(asio::ip::tcp::socket& sock)
{
	std::string fromClient = receiveData(sock);

	if (fromClient.length() < 1)
	{
		std::cerr << "Failed to receive data from the client\n";
		return false;
	}
	
	ERequest requestType = getMessageType(fromClient);

	std::cout << "Received a " << requestTypeToStr(requestType) 
		      << " request from the client" << std::endl;
	
	if (ERequest::Undefined == requestType)
	{
		std::cerr << "Invalid request type\n";
		return false;
	}

	if (!respond(sock, requestType))
	{
		std::cerr << "Failed to respond to the client\n";
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
			response = getHeadResponse();
			break;
		case ERequest::Post:
			response = getPostResponse();
			break;
		default:
			std::cerr << __FUNCTION__ << ": unsupported request type\n";
			return false;
	}
	
	std::string reqStr = requestTypeToStr(requestType);
	
	std::cout << reqStr << " response length: " << response.length() << std::endl;

	if (sendMsgLength(sock, response.length()))
	{
		std::cout << "Sent length of the " << reqStr << " response to the client" << std::endl;
	}
	else
	{
		std::cerr << "Failed to send length of the " << reqStr << " response to the client\n";
	}
	
	if (sendMsg(sock, response))
	{
		std::cout << "Sent " << reqStr << " response to the client" << std::endl;
	}
	else
	{
		std::cerr << "Failed to send " << reqStr << " response to the client\n";
	}
	
	return true;
}

ERequest getMessageType(const std::string& msg)
{
	if (0 == msg.find("HEAD"))
	{
		return ERequest::Head;
	}
	else if (0 == msg.find("POST"))
	{
		return ERequest::Post;
	}
	
	return ERequest::Undefined;
}

std::string getHeadResponse()
{
#if 0
	return "HTTP/1.1 200 OK\r\n"
           "Content-Length: 27\r\n"
           "Content-Type: application/json\r\n\r\n";
#else
	return "HTTP/1.1 200 OK\r\n"
           "Content-Length: 27\r\n"
           "Content-Type: text/html; charset=UTF-8\r\n\r\n";
#endif
}

std::string getPostResponse()
{
#if 0
	return "HTTP/1.1 200 OK\r\n"
           "Content-Length: 32\r\n"
           "Content-Type: application/json\r\n\r\n"
           "{\"success\":\"true\"}\r\n\r\n";
#else
	return "HTTP/1.1 200 OK\r\n"
           "Content-Length: 18\r\n"
           "Content-Type: application/json\r\n\r\n"
           "{\"success\":\"true\"}\r\n\r\n";
#endif
}

