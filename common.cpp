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
	
	std::cout << "cbrl: " << std::hex << rl << std::endl;
	
	std::vector<unsigned char> v(2);

	v[0] = rl >> 8;
	v[1] = rl & 0x0F;
	
	std::cout << "cbrl[0]: " << std::hex << (rl >> 8)     << std::endl;
	std::cout << "cbrl[1]: " << std::hex << (rl & 0x0F) << std::endl;

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

