/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/


#include <iostream>
#include <thread>

#include "Lobby.h"
#include "Constants.h"
#include "Utility.h"

#include "TCPInterface.h"
#include "RakString.h"
#include "jsonxx.h"


using namespace RakNet;
using namespace jsonxx;

namespace multislider
{
    std::string Host::makeMsgCreateRoom(const std::string & playerName, const std::string & roomName)
    {
        assert(!playerName.empty());
        assert(!roomName.empty());
        
        Object message;
        message << constants::MESSAGE_KEY_CLASS << constants::MESSAGE_CLASS_CREATE_ROOM;
        message << constants::MESSAGE_KEY_PLAYER_NAME << playerName;
        message << constants::MESSAGE_KEY_ROOM_NAME   << roomName;
        return message.write(JSON);
    }

    //-------------------------------------------------------

    Host::Host(shared_ptr<RakNet::TCPInterface> connection, const std::string & playerName, const std::string & roomName, HostCallback* callback)
        : mTcp(connection), mCallback(callback), mPlayerName(playerName), mRoomName(roomName)
    {
        assert(mTcp.get() != NULL);
        assert(!playerName.empty());
        assert(!roomName.empty());

        std::string createRoomMessage = makeMsgCreateRoom(mPlayerName, mRoomName);
        RakNet::SystemAddress serverAddr = mTcp->HasCompletedConnectionAttempt();
        mTcp->Send(createRoomMessage.c_str(), createRoomMessage.size(), serverAddr, false);
        if (!responsed(awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS), constants::RESPONSE_SUCC)) {
            throw ServerError("Host[Host]: Failed to create a new room!");
        }
    }
    //-------------------------------------------------------

    Host::~Host()
    { }

}
