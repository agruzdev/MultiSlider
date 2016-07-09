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
    using namespace constants;

    Client::Client(shared_ptr<RakNet::TCPInterface> connection, shared_ptr<RakNet::SystemAddress> address, const std::string & playerName, const RoomInfo & room, ClientCallback* callback)
        : mTcp(connection), mServerAddress(address), mCallback(callback), mMyRoom(room), mPlayerName(playerName)
    {
        assert(mTcp.get() != NULL);
        assert(!playerName.empty());
        assert(mCallback != NULL);
        assert(*mServerAddress != UNASSIGNED_SYSTEM_ADDRESS);
    }

    //-------------------------------------------------------
    int Client::join(const std::string & roomName)
    {
        Object joinRoomJson;
        joinRoomJson << MESSAGE_KEY_CLASS << frontend::JOIN_ROOM;
        joinRoomJson << MESSAGE_KEY_PLAYER_NAME << mPlayerName;
        joinRoomJson << MESSAGE_KEY_ROOM_NAME << roomName;
        std::string joinRoomMessage = joinRoomJson.write(JSON);
        mTcp->Send(joinRoomMessage.c_str(), joinRoomMessage.size(), *mServerAddress, false);
        shared_ptr<Packet> packet = awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS);
        //if (responsed(packet, constants::RESPONSE_SUCK)) {
        //    //throw ServerError("Client[Client]: Failed to join a room");
        //    return 1;
        //}
        if (responsed(packet, constants::RESPONSE_ROOM_IS_FULL)) {
            //throw ServerError("Client[Client]: Failed to join a room");
            return 2;
        }
        if (!mMyRoom.deserialize(std::string(pointer_cast<const char*>(packet->data), packet->length))) {
            //throw ServerError("Client[Client]: Failed to join a room");
            return 1;
        }
        mCallback->onJoined(mPlayerName, mMyRoom);
        mIsJoined = true;
        return 0;
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
        leaveRoomJson << MESSAGE_KEY_CLASS << frontend::LEAVE_ROOM;
        std::string leaveRoomMessage = leaveRoomJson.write(JSON);
        mTcp->Send(leaveRoomMessage.c_str(), leaveRoomMessage.size(), *mServerAddress, false);
        if (!responsed(awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS), constants::RESPONSE_SUCC)) {
            throw ServerError("Client[Client]: Failed to leave a room");
        }
        mCallback->onLeft(mPlayerName, mMyRoom);
        mIsJoined = false;
    }
    //-------------------------------------------------------

    void Client::broadcast(const std::string & data, bool toSelf)
    {
        Object broadcastJson;
        broadcastJson << MESSAGE_KEY_CLASS << frontend::BROADCAST;
        broadcastJson << MESSAGE_KEY_DATA << data;
        broadcastJson << MESSAGE_KEY_TO_SELF << toSelf;
        std::string broadcastMessage = broadcastJson.write(JSON);
        mTcp->Send(broadcastMessage.c_str(), broadcastMessage.size(), *mServerAddress, false);
    }
    //-------------------------------------------------------

    uint32_t Client::receive()
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
                    mCallback->onBroadcast(mPlayerName, message);
                }
            }
            else if (isMessageClass(messageClass, frontend::SESSION_STARTED)) {
                SessionPtr session(new Session(
                    messageJson.get<jsonxx::String>(MESSAGE_KEY_IP, ""), 
                    narrow_cast<uint16_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_PORT, 0.0)),
                    mPlayerName, 
                    messageJson.get<jsonxx::String>(MESSAGE_KEY_NAME, ""),
                    narrow_cast<uint32_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_ID, 0.0))), details::SessionDeleter());
                mCallback->onSessionStart(mPlayerName, session);
            }
            ++counter;
        }
        return counter;
    }

}
