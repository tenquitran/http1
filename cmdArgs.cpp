#include <iostream>
#include <boost/program_options.hpp>
#include "cmdArgs.h"

///////////////////////////////////////////////////////////////////////


bool CmdArgsHandler::parse(int argc, char* argv[], CmdLineArguments& args)
{
	if (!parseInternal(argc, argv, args))
    {
    	displayUsage(argv[0]);
    	return false;
    }
    
    return true;
}

bool CmdArgsHandler::parseInternal(int argc, char* argv[], CmdLineArguments& args)
{
	using namespace boost::program_options;

	// Server: there should be executable name and 3 options.
	//
	// Client: there should be at least "host" and "port" options 
	// and either "keep-alive" or a set of POST options.
	if (argc < 4)
		{return false;}
		
	options_description desc{"CommandLineOptions"};

#ifdef SERVER
    desc.add_options()
      ("port",     value<int>()->default_value(8976),       "Port")
      ("reload",   value<int>()->default_value(5),          "Reload")
      ("response", value<std::string>()->default_value(""), "ResponsePath");
      
	variables_map vm;
    
	try
	{
		store(parse_command_line(argc, argv, desc), vm);
		notify(vm);
	}
	catch(const std::exception& ex)
	{
    	std::cerr << ex.what() << std::endl;
    	return false;
  	}

	if (vm.count("port"))
	{
		std::cout << "Port: " << vm["port"].as<int>() << std::endl;
		args.m_port = vm["port"].as<int>();
	}
	else
		{return false;}

	if (vm.count("reload"))
	{
		std::cout << "Reload: " << vm["reload"].as<int>() << std::endl;
		args.m_reloadSeconds = vm["reload"].as<int>();
	}
	else
		{return false;}

	if (vm.count("response"))
	{
		std::cout << "Response path: " << vm["response"].as<std::string>() << std::endl;
		args.m_responseFilePath = vm["response"].as<std::string>();
	}
	else
		{return false;}
#else
	desc.add_options()
      ("host",         value<std::string>()->default_value("127.0.0.1"), "HostName")
      ("port",         value<int>()->default_value(8976),                "Port")
      ("keep-alive",   value<int>()->default_value(7),                   "KeepAlive")
      ("post-request", value<int>()->default_value(5),                   "PostRequest")
      ("reload",       value<int>()->default_value(6),                   "Reload")
      ("request",      value<std::string>()->default_value(""),          "RequestPath");
      
    variables_map vm;
    
	try
	{
		store(parse_command_line(argc, argv, desc), vm);
		notify(vm);
	}
	catch(const std::exception& ex)
	{
    	std::cerr << ex.what() << std::endl;
    	return false;
  	}

	if (vm.count("host"))
	{
		std::cout << "Host name: " << vm["host"].as<std::string>() << std::endl;
		args.m_ipAddress = vm["host"].as<std::string>();
	}
	else
		{return false;}
	
	if (vm.count("port"))
	{
		std::cout << "Port: " << vm["port"].as<int>() << std::endl;
		args.m_port = vm["port"].as<int>();
	}
	else
		{return false;}
	
	// We are suppose that the "keep-alive" command-line option 
	// is mutually exclusive with the set of options "post-request", "reload" and "request" -
	// that is, that the client will send either only keep-alive HEAD requests 
	// or only POST requests.
	
	if (!vm["keep-alive"].defaulted())
	{
		std::cout << "Keep-alive: " << vm["keep-alive"].as<int>() << std::endl;
		
		args.m_keepAliveSeconds = vm["keep-alive"].as<int>();
		args.m_keepAliveMode = true;
		
		std::cout << "Mode: keep-alive" << std::endl;
		
		return true;
	}
	else
	{
		if (vm.count("post-request"))
		{
			std::cout << "Post-request: " << vm["post-request"].as<int>() << std::endl;
			args.m_postSeconds = vm["post-request"].as<int>();
		}
		else
			{return false;}
			
		if (vm.count("reload"))
		{
			std::cout << "Reload: " << vm["reload"].as<int>() << std::endl;
			args.m_reloadSeconds = vm["reload"].as<int>();
		}
		else
			{return false;}

		if (vm.count("request"))
		{
			std::cout << "Request path: " << vm["request"].as<std::string>() << std::endl;
			args.m_requestFilePath = vm["request"].as<std::string>();
		}
		else
			{return false;}
			
		std::cout << "Mode: POST" << std::endl;
	}
#endif
      
    return true;
}

void CmdArgsHandler::displayUsage(const char * const exeName)
{
#ifdef SERVER
	// --port=portnumber - the number of listener port
	// --reload=XX - every XX seconds reload the file where the HTTP Response is stored and use it when process next HTTP Requests
	// --response=/path/to/Response.json - path to Response.html used by server when receive Response from client.

	std::cout << "Usage:\n\n"
			  << exeName 
			  << " --port=<port_number> --reload=<seconds> "
			     "--response=/path/to/Response.json\n"

			  << "\nExample:\n\n"
	          << exeName 
	          << " --port=8976 --reload=5 "
	             "--response=responses/post_response.json"
	          << std::endl;
#else
	// --host=hostname    - the host of HTTP server
	// --port=portnumber  - the port of HTTP server
	// --keep-alive=XX    - run HTTP client with keep-alive HEAD requests every XX seconds
	// --post-request=XX  - send HTTP POST request every XX seconds
	// --reload=XX        - every XX seconds reload the file where the Request is stored
	// --request=/path/to/Request.json  - path to Request.html used by client to send to the Server

	std::cout << "Usage (variant 1, sending keep-alive HEAD requests):\n\n"
			  << exeName 
			  << " --host=<ip_address> --port=<port_number> "
			     "--keep-alive=<seconds>\n"

			  << "\nUsage (variant 2, sending POST requests from the specified file):\n\n"
			  << exeName
			  << " --host=<ip_address> --port=<port_number> "
			     "--post-request=<seconds> "
			     "--reload=<seconds> --request=/path/to/Request.json\n"
			  
			  << "\nExample 1:\n\n"
	          << exeName 
	          << " --host=127.0.0.1 --port=8976 --keep-alive=7\n\n" 
	          
	          << "\nExample 2:\n\n"
	          << exeName 
	          << " --host=127.0.0.1 --port=8976 "
	             "--post-request=5 --request=requests/post_request.json --reload=4"
	          << std::endl;
#endif
}

