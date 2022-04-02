#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

using namespace boost;

///////////////////////////////////////////////////////////////////////

bool parseCmdLineArgs(int argc, char* argv[]);

bool sendHeadRequest(asio::ip::tcp::socket& sock);

///////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
	// TODO: hard-coded
	const std::string ipStr = "127.0.0.1";
    const unsigned short port = 8976;

#if 0
    if (!parseCmdLineArgs(argc, argv))
    {
    	// TODO: show help
    	return 1;
    }
#endif

	try
	{
		asio::ip::tcp protocol = asio::ip::tcp::v4();
		
		asio::ip::tcp::endpoint ep(asio::ip::address::from_string(ipStr), port);
		
		asio::io_service io;
		
		asio::ip::tcp::socket sock(io, ep.protocol());
		
		sock.connect(ep);

		// TODO: interact with the server
		if (!sendHeadRequest(sock))
		{
			std::cerr << "Failed to send HEAD request to the server\n";
		}
    }
	catch (system::system_error& ex)
	{
		std::cerr << "Exception: " << ex.what() << '\n';
		//std::cerr << "Exception: " << ex.what() << " (" << ex.code() << ")\n";
		return 2;
	}

    return 0;
}

#if 0
bool parseCmdLineArgs(int argc, char* argv[])
{
	using namespace boost::program_options;
	
	if (argc < 3)
	{
		// TODO: display usage info
		return false;
	}

    // --host=hostname - the host of HTTP server
	// --port=portnumber - the port of HTTP server

    options_description desc{"CommandLineOptions"};
    
    desc.add_options()
      ("host", value<std::string>()->default_value("127.0.0.1"), "HostName")
      ("port", value<int>()->default_value(8976), "Port");
      
    variables_map vm;
    
	try
	{
		store(parse_command_line(argc, argv, desc), vm);
		notify(vm);
	}
	catch(const std::exception& ex)
	{
    	std::cerr << ex.what() << std::endl;
    	// TODO: display usage info
    	return false;
  	}

	if (vm.count("host"))
	{
		std::cout << "Host name: " << vm["host"].as<std::string>() << std::endl;
	}
	
	if (vm.count("port"))
	{
		std::cout << "Port: " << vm["port"].as<int>() << std::endl;
	}

	return true;
}
#endif

bool sendHeadRequest(asio::ip::tcp::socket& sock)
{
	/*
	HTTP HEAD request example:

	HEAD /echo/head/json HTTP/1.1
	Accept: application/json
	Host: reqbin.com
	*/
	
	/*
	Server Response example:

	HTTP/1.1 200 OK
	Content-Length: 19
	Content-Type: application/json
	*/

	std::string request = "HEAD /echo/head/json HTTP/1.1"
	                      "Accept: application/json"
	                      "Host: somehost.com";
	                      
	const size_t cbReq = request.length();
	
	std::size_t cbSent = {};

	try
	{
		cbSent = sock.send(
			asio::buffer(request.c_str(), cbReq), 
			0);
		
		std::cout << "Sent " << cbSent << " bytes of " << cbReq << std::endl;
	}
	catch (system::system_error& ex)
	{
		std::cerr << "Exception on sending HEADER: " << ex.what() << '\n';
		//std::cerr << "Exception: " << ex.what() << " (" << ex.code() << ")\n";
		return false;
	}
	
	return (cbReq == cbSent);
}

