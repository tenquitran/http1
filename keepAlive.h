#ifndef _KEEP_ALIVE_H
#define _KEEP_ALIVE_H


// Client: helper class for POST request sending.
class KeepAlive
{
public:
	KeepAlive();

	// Action performed by periodic timer.
	void periodicAction();
	
public:
	int m_delaySeconds = {};

	std::weak_ptr<boost::asio::ip::tcp::socket> m_spSocket;
	
	RequestReloader *m_pRequestReloader = {nullptr};
};

#endif

