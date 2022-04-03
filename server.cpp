#include <iostream>
#include <boost/asio.hpp>

using namespace boost;

///////////////////////////////////////////////////////////////////////

bool receiveData(asio::ip::tcp::socket& sock);

///////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
	// TODO: hard-coded
    const unsigned short port = 8976;

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
			
			// TODO: serve the client
			if (!receiveData(sock))
			{
				std::cerr << "Failed to receive data from the client\n";
			}
			
			if (!receiveData(sock))
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

bool receiveData(asio::ip::tcp::socket& sock)
{
	try
	{
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
		
		for (auto c : req)
		{
			std::cout << c;
		}
		std::cout << std::endl << std::endl;

		// TODO: process the request
		;
	}
	catch (system::system_error& ex)
	{
		std::cerr << "Exception on receiving: " << ex.what() << '\n';
		//std::cerr << "Exception: " << ex.what() << " (" << ex.code() << ")\n";
		return false;
	}
	
	return true;
}

