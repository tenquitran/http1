#include <iostream>
#include <boost/asio.hpp>

using namespace boost;


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
			;
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

