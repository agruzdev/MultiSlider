/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/


#include <iostream>

#include "TCPInterface.h"

#include "Lobby.h"

namespace
{
    static const char SERVER_ADDRESS[] = "127.0.0.1";
    static const uint16_t SERVER_FRONTEND_PORT = 8800;
    static const uint16_t SERVER_BACKEND_PORT  = 8700;

    struct TcpDeleter
    {
        void operator()(RakNet::TCPInterface* tcp) const
        {
            RakNet::TCPInterface::DestroyInstance(tcp);
        }
    };

}

namespace multislider
{

    Lobby::Lobby()
    {
        std::cout << "Init..." << std::endl;
        mTcp.reset(RakNet::TCPInterface::GetInstance(), TcpDeleter());
        if (!mTcp->Start(8801, 64)) {
            throw Fail("Lobby[Lobby]: Failed to start TCP interface");
        }
        mTcp->Connect(SERVER_ADDRESS, SERVER_FRONTEND_PORT);
    }
    //-------------------------------------------------------

    Lobby::~Lobby()
    {
        std::cout << "Shutdown" << std::endl;
        mTcp->Stop();
    }
    //-------------------------------------------------------

    Host* Lobby::becomeHost(HostCallback* callback)
    {
        mHostInstance.reset(new Host(mTcp, callback));
        return mHostInstance.get();
    }
}
