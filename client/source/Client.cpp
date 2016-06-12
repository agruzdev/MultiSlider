/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/


#include <iostream>
#include <thread>

#include "Client.h"
#include "Constants.h"
#include "Utility.h"
#include "Exception.h"

#include "TCPInterface.h"
#include "RakString.h"
#include "jsonxx.h"


using namespace RakNet;
using namespace jsonxx;

namespace multislider
{

    Client::Client(shared_ptr<RakNet::TCPInterface> connection, shared_ptr<RakNet::SystemAddress> address, const std::string & playerName, const RoomInfo & room, ClientCallback* callback)
        : mTcp(connection), mServerAddress(address), mCallback(callback), mMyRoom(room), mPlayerName(playerName)
    {
        assert(mTcp.get() != NULL);
        assert(!playerName.empty());
        assert(mCallback != NULL);
        assert(*mServerAddress != UNASSIGNED_SYSTEM_ADDRESS);

        Object joinRoomJson;
        joinRoomJson << constants::MESSAGE_KEY_CLASS << constants::MESSAGE_CLASS_JOIN_ROOM;
        joinRoomJson << constants::MESSAGE_KEY_PLAYER_NAME << mPlayerName;
        joinRoomJson << constants::MESSAGE_KEY_ROOM_NAME << mMyRoom.roomName;
        std::string joinRoomMessage = joinRoomJson.write(JSON);
        mTcp->Send(joinRoomMessage.c_str(), joinRoomMessage.size(), *mServerAddress, false);
        if (!responsed(awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS), constants::RESPONSE_SUCC)) {
            throw ServerError("Client[Client]: Failed to join a room");
        }
        mCallback->onJoined(mPlayerName, mMyRoom);
    }
    //-------------------------------------------------------

    Client::~Client()
    {

    }

    //-------------------------------------------------------


}
