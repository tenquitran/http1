#include "serverAsio.h"

///////////////////////////////////////////////////////////////////////

using namespace boost;

///////////////////////////////////////////////////////////////////////


ServerAsio::ServerAsio(unsigned short port)
	: m_port(port)
{
	asio::ip::address serverIp = asio::ip::address_v4::any();

	m_endpoint = asio::ip::tcp::endpoint(serverIp, m_port);
	
	m_spAcceptor = std::make_unique<asio::ip::tcp::acceptor>(m_io, m_endpoint);
}

void ServerAsio::listen()
{
	m_spAcceptor->listen();
}

UPSocket ServerAsio::accept()
{
	UPSocket spSock = std::make_unique<asio::ip::tcp::socket>(m_io);

	m_spAcceptor->accept(*spSock);
	
	return spSock;
}

void ServerAsio::close()
{
	shutdown(m_spAcceptor->native_handle(), SHUT_RD);
	
	m_spAcceptor->close();
}

