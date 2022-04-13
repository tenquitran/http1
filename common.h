#ifndef _COMMON_H
#define _COMMON_H

#include <memory>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include "requestReloader.h"

///////////////////////////////////////////////////////////////////////

enum class ERequest
{
	Undefined,
	Head,
	Post
};


typedef std::unique_ptr<boost::asio::ip::tcp::socket> UPSocket;

///////////////////////////////////////////////////////////////////////

std::string requestTypeToStr(ERequest rt);

bool sendMsgLength(boost::asio::ip::tcp::socket& sock, int len);

bool sendMsg(boost::asio::ip::tcp::socket& sock, const std::string& msg);

std::string receiveData(boost::asio::ip::tcp::socket& sock);

// For client only.
// Get text of the HEAD request.
std::string getHeadRequest();

// For client only.
// Get text of the keep-alive HEAD request.
std::string getHeadRequestKeepAlive();

// For client only.
// Exchange messages with the server.
void exchangeMessages(boost::asio::ip::tcp::socket& sock, ERequest requestType, const std::string& postRequest = "");

///////////////////////////////////////////////////////////////////////

#endif

