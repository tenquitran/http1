#include "common.h"
#include "keepAlive.h"

///////////////////////////////////////////////////////////////////////

using namespace boost;

///////////////////////////////////////////////////////////////////////


KeepAlive::KeepAlive()
	: m_delaySeconds(7)
{
}

void KeepAlive::periodicAction()
{
	std::shared_ptr<asio::ip::tcp::socket> spSock = m_spSocket.lock();
		
	if (spSock)
	{
		exchangeMessages(*spSock, ERequest::Head);
	}
}

