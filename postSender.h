#ifndef _POST_SENDER_H
#define _POST_SENDER_H

#include <mutex>
#include <string>
#include <boost/asio.hpp>


class RequestReloader;

// Client: helper class for POST request sending.
class PostSender
{
public:
	PostSender();
	
	void initialize(RequestReloader* pRequestReloader);
	
	// Action performed by periodic timer.
	void periodicAction();
	
public:
	int m_delaySeconds = {};

	std::weak_ptr<boost::asio::ip::tcp::socket> m_spSocket;
	
	RequestReloader *m_pRequestReloader = {nullptr};
};

#endif

