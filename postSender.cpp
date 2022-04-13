#include <iostream>
#include <fstream>
#include <sstream>
#include "common.h"
#include "postSender.h"
#include "requestReloader.h"

///////////////////////////////////////////////////////////////////////

using namespace boost;

///////////////////////////////////////////////////////////////////////


PostSender::PostSender()
	: m_delaySeconds(5)
{
}

void PostSender::initialize(RequestReloader* pRequestReloader)
{
	m_pRequestReloader = pRequestReloader;
}

void PostSender::periodicAction()
{
	std::shared_ptr<asio::ip::tcp::socket> spSock = m_spSocket.lock();
		
	if (spSock)
	{
		exchangeMessages(*spSock, ERequest::Post, m_pRequestReloader->getPostRequest());
	}
}

