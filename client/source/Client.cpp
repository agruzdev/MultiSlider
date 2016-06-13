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
        mIsJoined = true;
    }
    //-------------------------------------------------------

    Client::~Client()
    {
        if (mIsJoined) {
            try {
                leaveRoom();
            }
            catch (RuntimeError &) {
            }
        }
    }
    //-------------------------------------------------------

    void Client::leaveRoom()
    {
        Object leaveRoomJson;
        leaveRoomJson << constants::MESSAGE_KEY_CLASS << constants::MESSAGE_CLASS_LEAVE_ROOM;
        std::string leaveRoomMessage = leaveRoomJson.write(JSON);
        mTcp->Send(leaveRoomMessage.c_str(), leaveRoomMessage.size(), *mServerAddress, false);
        if (!responsed(awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS), constants::RESPONSE_SUCC)) {
            throw ServerError("Client[Client]: Failed to leave a room");
        }
        mCallback->onLeft(mPlayerName, mMyRoom);
        mIsJoined = false;
    }
    //-------------------------------------------------------

    void Client::broadcast(const std::string & data)
    {
        Object broadcastJson;
        broadcastJson << constants::MESSAGE_KEY_CLASS << constants::MESSAGE_CLASS_BROADCAST;
        broadcastJson << constants::MESSAGE_KEY_DATA << data;
        std::string broadcastMessage = broadcastJson.write(JSON);
        mTcp->Send(broadcastMessage.c_str(), broadcastMessage.size(), *mServerAddress, false);
    }
    //-------------------------------------------------------

    uint32_t Client::receive() const
    {
        uint32_t counter(0);
        for (;;) {
            Packet* packet = mTcp->Receive();
            if (packet == NULL) {
                break;
            }
            Object broadcastJson;
            broadcastJson.parse(std::string(castPointerTo<char*>(packet->data), packet->length));
            std::string message = broadcastJson.get<std::string>(constants::MESSAGE_KEY_DATA, "");
            if (!message.empty()) {
                mCallback->onBroadcast(mPlayerName, message);
                ++counter;
            }
        }
        return counter;
    }

}
