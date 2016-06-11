/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/


#include <iostream>
#include <thread>

#include "Lobby.h"

#include "TCPInterface.h"

namespace multislider
{

    Host::Host(shared_ptr<RakNet::TCPInterface> connection, HostCallback* callback)
        : mTcp(connection), mCallback(callback)
    {
        std::string mesage = "{\"jsonClass\":\"FrontendMessage$CreateRoom\",\"playerName\":\"doge\",\"roomName\":\"woof\"}";
        RakNet::SystemAddress serverAddr = mTcp->HasCompletedConnectionAttempt();
        mTcp->Send(mesage.c_str(), mesage.length(), serverAddr, false);
        std::this_thread::sleep_for(std::chrono::seconds(50));
    }
    //-------------------------------------------------------

    Host::~Host()
    { }

}
