#ifndef _COMMON_H
#define _COMMON_H

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

///////////////////////////////////////////////////////////////////////

enum class ERequest
{
	Undefined,
	Head,
	Post
};

///////////////////////////////////////////////////////////////////////

std::string requestTypeToStr(ERequest rt);

bool sendMsgLength(boost::asio::ip::tcp::socket& sock, int len);

bool sendMsg(boost::asio::ip::tcp::socket& sock, const std::string& msg);

std::string receiveData(boost::asio::ip::tcp::socket& sock);

///////////////////////////////////////////////////////////////////////

#endif

