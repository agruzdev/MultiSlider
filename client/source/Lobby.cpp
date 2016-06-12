/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/


#include <iostream>

#include "TCPInterface.h"

#include "Lobby.h"
#include "Constants.h"
#include "Utility.h"

using namespace RakNet;

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
            throw NetworkError("Lobby[Lobby]: Failed to start TCP interface");
        }
        SystemAddress address = mTcp->Connect(SERVER_ADDRESS, SERVER_FRONTEND_PORT);
        if (address == UNASSIGNED_SYSTEM_ADDRESS) {
            throw NetworkError("Lobby[Lobby]: Failed to connect to server");
        }
        if (!responsed(awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS), constants::RESPONSE_GREET)) {
            throw ProtocolError("Lobby[Lobby]: Unrecognized greeting");
        }
    }
    //-------------------------------------------------------

    Lobby::~Lobby()
    {
        std::cout << "Shutdown" << std::endl;
        mTcp->Stop();
    }
    //-------------------------------------------------------

    Host* Lobby::becomeHost(const std::string & playerName, const std::string & roomName, HostCallback* callback)
    {
        mHostInstance.reset(new Host(mTcp, playerName, roomName, callback));
        return mHostInstance.get();
    }
}
