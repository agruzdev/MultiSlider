/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#include <RakPeerInterface.h>
#include <RakNetTypes.h>

#include "Session.h"

using namespace RakNet;

namespace multislider
{
    void Session::DestoyInstance(Session* ptr)
    {
        if (ptr != nullptr) {
            delete ptr;
        }
    }
    //-------------------------------------------------------

    Session::Session(std::string address, const std::string & playerName, const std::string & sessionName, uint32_t sessionId)
        : mPlayerName(playerName), mSessionName(sessionName), mSessionId(sessionId)
    {
        assert(!mPlayerName.empty());
        
        
    }
    //-------------------------------------------------------

    Session::~Session()
    { }

}



