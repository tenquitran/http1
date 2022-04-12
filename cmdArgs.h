#ifndef _CMD_ARGS_H
#define _CMD_ARGS_H


// Command-line arguments.
#ifdef SERVER
struct CmdLineArguments
{
	// Port number.
	unsigned short m_port = {};
	
	// HTTP response reload period, in seconds.
	int m_reloadSeconds = {};
	
	// Path to the file containg the POST response payload.
	std::string m_responseFilePath;
};
#else
struct CmdLineArguments
{
	// IP address.
	std::string m_ipAddress;
	
	// Port number.
	unsigned short m_port = {};
	
	// Path to the file containg the POST request payload.
	std::string m_requestFilePath;
	
	// For the keep-alive mode:
	// period to send keep-alive HEAD requests, in seconds.
	int m_keepAliveSeconds = {};
	
	// true if the client will send only HEAD keep-alive requests (keep-alive mode), 
	// false if it will send only POST requests (POST mode).
	bool m_keepAliveMode = {false};
	
	// For the POST mode: 
	// period to send POST requests.
	int m_postSeconds = {};
	
	// For the POST mode: 
	// period to reload POST requests from the file.
	int m_reloadSeconds = {};
};
#endif


// Command-line arguments processor.
class CmdArgsHandler
{
public:
	// Parse the command-line arguments.
	// Parameters: argc - number of command-line arguments;
	//             argv - command-line arguments;
	//             args - parsed arguments (valid if the function returns true).
	// Returns: true on success, false otherwise.
	static bool parse(int argc, char* argv[], CmdLineArguments& args);

private:
	// Parse the command-line arguments (server version).
	// Parameters: argc - number of command-line arguments;
	//             argv - command-line arguments;
	//             args - parsed arguments (valid if the function returns true).
	// Returns: true on success, false otherwise.
	static bool parseInternal(int argc, char* argv[], CmdLineArguments& args);
	
	// Display instructions on the command-line arguments usage.
	// Parameters: exeName - executable name.
	static void displayUsage(const char * const exeName);
};

#endif

