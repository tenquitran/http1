#include <iostream>
#include <string>
#include <boost/asio.hpp>

using namespace boost;


int main()
{
	// TODO: hard-coded
	const std::string ipStr = "127.0.0.1";
    const unsigned short port = 8976;

	try
	{
		asio::ip::tcp protocol = asio::ip::tcp::v4();
		
		asio::ip::tcp::endpoint ep(asio::ip::address::from_string(ipStr), port);
		
		asio::io_service io;
		
		asio::ip::tcp::socket sock(io, ep.protocol());
		
		sock.connect(ep);
		
		// TODO: interact with the server
    }
	catch (system::system_error& ex)
	{
		std::cerr << "Exception: " << ex.what() << '\n';
		//std::cerr << "Exception: " << ex.what() << " (" << ex.code() << ")\n";
		return 1;
	}

    return 0;
}

