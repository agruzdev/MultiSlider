/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/


#include <iostream>
#include <thread>
#include <cstdlib>

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
    using namespace constants;

    Host::Host(shared_ptr<RakNet::TCPInterface> connection, shared_ptr<RakNet::SystemAddress> address, const std::string & playerName, const std::string & roomName, uint32_t playersLimit, HostCallback* callback)
        : mTcp(connection), mCallback(callback), mServerAddress(address)
    {
        assert(mTcp.get() != NULL);
        assert(!playerName.empty());
        assert(!roomName.empty());
        assert(mCallback != NULL);
        assert(*mServerAddress != UNASSIGNED_SYSTEM_ADDRESS);

        //mMyRoom.hostName = playerName;
        //mMyRoom.roomName = roomName;

        Object createRoomJson;
        createRoomJson << MESSAGE_KEY_CLASS << frontend::CREATE_ROOM;
        createRoomJson << MESSAGE_KEY_PLAYER_NAME << playerName;
        createRoomJson << MESSAGE_KEY_ROOM_NAME << roomName;
        createRoomJson << MESSAGE_KEY_PLAYERS_LIMIT << playersLimit;
        std::string createRoomMessage = createRoomJson.write(JSON);

        mTcp->Send(createRoomMessage.c_str(), createRoomMessage.size(), *mServerAddress, false);
        //if (!responsed(awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS), constants::RESPONSE_SUCC)) {
        //    throw ServerError("Host[Host]: Failed to create a new room!");
        //}
        //shared_ptr<Packet> responce = awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS);
        //if (responsed(responce, constants::RESPONSE_SUCK)) {
        //    throw ServerError("Host[Host]: Failed to create a new room!");
        //}
        shared_ptr<Packet> responce = awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS);
        if (!mMyRoom.deserialize(std::string(pointer_cast<const char*>(responce->data), responce->length)))
        {
            throw ServerError("Host[Host]: Failed to create a new room!");
        }
        mCallback->onCreated(this, mMyRoom);
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
        closeRoomJson << MESSAGE_KEY_CLASS << frontend::CLOSE_ROOM;
        std::string closeRoomMessage = closeRoomJson.write(JSON);
        mTcp->Send(closeRoomMessage.c_str(), closeRoomMessage.size(), *mServerAddress, false);
        if (!responsed(awaitResponse(mTcp, DEFAULT_TIMEOUT_MS), RESPONSE_SUCC)) {
            throw ServerError("Host[closeRoom]: failed to close the current room!");
        }
        mCallback->onClosed(this, mMyRoom);
        mIsOpened = false;
    }

    //-------------------------------------------------------
    void Host::broadcast(const std::string & data, bool toSelf)
    {
        Object broadcastJson;
        broadcastJson << MESSAGE_KEY_CLASS << frontend::BROADCAST;
        broadcastJson << MESSAGE_KEY_ROOM << jsonxx::Null();
        broadcastJson << MESSAGE_KEY_DATA << data;
        broadcastJson << MESSAGE_KEY_TO_SELF << toSelf;
        std::string broadcastMessage = broadcastJson.write(JSON);
        mTcp->Send(broadcastMessage.c_str(), broadcastMessage.size(), *mServerAddress, false);
    }
    //-------------------------------------------------------

    uint32_t Host::receive()
    {
        uint32_t counter(0);
        for (;;) {
            shared_ptr<Packet> packet = awaitResponse(mTcp);
            if (packet == NULL) {
                break;
            }
            Object messageJson;
            messageJson.parse(std::string(pointer_cast<char*>(packet->data), packet->length));
            std::string messageClass(messageJson.get<std::string>(MESSAGE_KEY_CLASS));
            if (isMessageClass(messageClass, frontend::BROADCAST)) {
                std::string message = messageJson.get<std::string>(constants::MESSAGE_KEY_DATA, "");
                if (mMyRoom.deserialize(messageJson.get<Object>(constants::MESSAGE_KEY_ROOM, Object()))) {
                    mCallback->onBroadcast(this, mMyRoom, message);
                }
            }
            else if (isMessageClass(messageClass, frontend::SESSION_STARTED)) {
                SessionPtr session(new Session(
                    messageJson.get<jsonxx::String>(MESSAGE_KEY_IP, ""),
                    narrow_cast<uint16_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_PORT, 0.0)),
                    mMyRoom.getHostName(),
                    messageJson.get<jsonxx::String>(MESSAGE_KEY_NAME, ""),
                    narrow_cast<uint32_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_ID, 0.0))), details::SessionDeleter());
                mCallback->onSessionStart(this, mMyRoom, session);
            }
            ++counter;
        }
        return counter;
    }
    //-------------------------------------------------------

    void Host::startSession()
    {
        Object startSessionJson;
        startSessionJson << MESSAGE_KEY_CLASS << frontend::START_SESSION;
        std::string startSessionMessage = startSessionJson.write(JSON);
        mTcp->Send(startSessionMessage.c_str(), startSessionMessage.size(), *mServerAddress, false);
    }

}
