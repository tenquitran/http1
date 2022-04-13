#ifndef _SERVER_ASIO_H
#define _SERVER_ASIO_H

#include "common.h"


// Boost.Asio server wrapper.
class ServerAsio
{
public:
	explicit ServerAsio(unsigned short port);
	
	void listen();
	
	UPSocket accept();
	
	void close();

private:
	// Port number.
	unsigned short m_port = {};

	std::unique_ptr<boost::asio::ip::tcp::acceptor> m_spAcceptor;
	
	boost::asio::ip::tcp::endpoint m_endpoint;
	
	boost::asio::io_service m_io;
};

#endif

