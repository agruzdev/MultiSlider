/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/


#include <iostream>
#include <thread>

#include "Host.h"
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

    Host::Host(shared_ptr<RakNet::TCPInterface> connection, shared_ptr<RakNet::SystemAddress> address, const std::string & playerName, const std::string & roomName, HostCallback* callback)
        : mTcp(connection), mCallback(callback), mServerAddress(address)
    {
        assert(mTcp.get() != NULL);
        assert(!playerName.empty());
        assert(!roomName.empty());
        assert(mCallback != NULL);
        assert(*mServerAddress != UNASSIGNED_SYSTEM_ADDRESS);

        mMyRoom.hostName = playerName;
        mMyRoom.roomName = roomName;

        std::string createRoomMessage = makeMsgCreateRoom(playerName, roomName);
        mTcp->Send(createRoomMessage.c_str(), createRoomMessage.size(), *mServerAddress, false);
        if (!responsed(awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS), constants::RESPONSE_SUCC)) {
            throw ServerError("Host[Host]: Failed to create a new room!");
        }
        mCallback->onCreated(mMyRoom);
        mIsOpened = true;
    }
    //-------------------------------------------------------

    Host::~Host()
    {
        if (mIsOpened) {
            try {
                closeRoom();
            }
            catch (RuntimeError &) { }
        }
    }

    //-------------------------------------------------------
    void Host::closeRoom()
    {
        Object closeRoomJson;
        closeRoomJson << constants::MESSAGE_KEY_CLASS << constants::MESSAGE_CLASS_CLOSE_ROOM;
        std::string closeRoomMessage = closeRoomJson.write(JSON);
        mTcp->Send(closeRoomMessage.c_str(), closeRoomMessage.size(), *mServerAddress, false);
        if (!responsed(awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS), constants::RESPONSE_SUCC)) {
            throw ServerError("Host[closeRoom]: failed to close the current room!");
        }
        mCallback->onClosed(mMyRoom);
        mIsOpened = false;
    }

    //-------------------------------------------------------
    void Host::broadcast(const std::string & data)
    {
        Object broadcastJson;
        broadcastJson << constants::MESSAGE_KEY_CLASS << constants::MESSAGE_CLASS_BROADCAST;
        broadcastJson << constants::MESSAGE_KEY_DATA << data;
        std::string broadcastMessage = broadcastJson.write(JSON);
        mTcp->Send(broadcastMessage.c_str(), broadcastMessage.size(), *mServerAddress, false);
    }
    //-------------------------------------------------------

    uint32_t Host::receive() const
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
                mCallback->onBroadcast(mMyRoom, message);
                ++counter;
            }
        }
        return counter;
    }

}
