/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#if defined(_MSC_VER) && (_MSC_VER < 1800)
# include "CommonIncludes.h"
# ifndef _STDINT_H_
#  define _STDINT_H_
# endif
#endif

#include "TCPInterface.h"

#include "Lobby.h"
#include "Constants.h"
#include "Utility.h"
#include "Exception.h"
#include "Session.h"
#include "UdpInterface.h"

#include <boost/asio.hpp>

#pragma warning(push)
#pragma warning(disable: 4127)
#include <jsonxx.h>
#pragma warning(pop)


using namespace RakNet;
using namespace jsonxx;

namespace
{
    static const size_t RECEIVE_BIFFER_SIZE = 32 * 1024; // 32 Kb

    struct TcpDeleter
    {
        void operator()(RakNet::TCPInterface* tcp) const
        {
            tcp->Stop();
            RakNet::TCPInterface::DestroyInstance(tcp);
        }
    };
}

namespace multislider
{
    using namespace constants;
    //-------------------------------------------------------

    const uint8_t LobbyCallback::FLAG_IS_EJECTED            = 1;
    const uint8_t LobbyCallback::FLAG_JOINED                = 1 << 1;
    const uint8_t LobbyCallback::FLAG_LEFT                  = 1 << 2;
    const uint8_t LobbyCallback::FLAG_NEW_HOST              = 1 << 3;
    const uint8_t LobbyCallback::FLAG_ROOM_CLOSED_BY_HOST   = 1 << 4;
    const uint8_t LobbyCallback::FLAG_RECONFIGURED_BY_HOST  = 1 << 5;
    const uint8_t LobbyCallback::FLAG_RECONFIGURE_FAIL      = 1 << 6;
    //-------------------------------------------------------

    Lobby::Lobby(const std::string & serverIp, uint16_t serverPort)
        : mServerIp(serverIp), mServerPort(serverPort), mCallback(NULL), mIsJoined(false), mIsHost(false)
    {
        mTcp.reset(RakNet::TCPInterface::GetInstance(), TcpDeleter());
        if (!mTcp->Start(8801, 64)) {
            throw NetworkError("Lobby[Lobby]: Failed to start TCP interface");
        }
        mServerAddress.reset(new SystemAddress);
        *mServerAddress = mTcp->Connect(serverIp.c_str(), serverPort);
        if (*mServerAddress == UNASSIGNED_SYSTEM_ADDRESS) {
            throw NetworkError("Lobby[Lobby]: Failed to connect to server");
        }

        shared_ptr<Packet> packet = awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS);
        if (packet == NULL) {
            throw ProtocolError("Lobby[Lobby]: No responce from the server");
        }
        Object messageJson;
        messageJson.parse(std::string(pointer_cast<char*>(packet->data), packet->length));
        std::string messageClass(messageJson.get<std::string>(MESSAGE_KEY_CLASS, ""));
        if (isMessageClass(messageClass, frontend::GREETING)) {
            const uint32_t verMajor = narrow_cast<uint32_t>(messageJson.get<jsonxx::Number>(constants::MESSAGE_KEY_VERSION_MAJOR, 0));
            const uint32_t verMinor = narrow_cast<uint32_t>(messageJson.get<jsonxx::Number>(constants::MESSAGE_KEY_VERSION_MINOR, 0));
            if ((verMajor != constants::VERSION_MAJOR) || (verMinor != constants::VERSION_MINOR)) {
                throw ProtocolError("Lobby[Lobby]: Server version mismatch!");
            }
        }
        else {
            throw ProtocolError("Lobby[Lobby]: Unrecognized greeting");
        }
        mUdpInterface.reset(new UdpInterface(serverIp, serverPort));
    }
    //-------------------------------------------------------

    Lobby::~Lobby()
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

    jsonxx::Object Lobby::makeEnvelop(const jsonxx::Object & obj) const
    {
        assert(!mMyRoom.getName().empty());
        Object envelop;
        envelop << MESSAGE_KEY_CLASS << frontend::ENVELOP;
        envelop << MESSAGE_KEY_ROOM_NAME << mMyRoom.getName();
        envelop << MESSAGE_KEY_DATA << obj.write(JSON);
        return envelop;
    }
    //-------------------------------------------------------

    void Lobby::sendTcpMessage(const std::string & message) const
    {
        mTcp->Send(message.c_str(), narrow_cast<unsigned int>(message.size()), *mServerAddress, false);
    }
    //-------------------------------------------------------

    Lobby::Status Lobby::createRoom(const std::string & playerName, const std::string & roomName, const std::string & description, uint32_t playersLimit, uint32_t playersReserved, const std::string & userParameter, LobbyCallback* callback)
    {
        if (playerName.empty()) {
            throw ProtocolError("Lobby[createRoom]: playerName can't be empty!");
        }
        if (roomName.empty()) {
            throw ProtocolError("Lobby[createRoom]: roomName can't be empty!");
        }
        if (callback == NULL) {
            throw ProtocolError("Lobby[createRoom]: callback can't be null!");
        }
        if (playersLimit < 1) {
            throw ProtocolError("Lobby[createRoom]: players limit can't be less than 1");
        }
        if (playersReserved > playersLimit - 1) {
            throw ProtocolError("Lobby[createRoom]: players reserved number can't be greater or equal to player limit");
        }
        assert(mTcp.get() != NULL);
        assert(*mServerAddress != UNASSIGNED_SYSTEM_ADDRESS);

        Object createRoomJson;
        createRoomJson << MESSAGE_KEY_CLASS << frontend::CREATE_ROOM;
        createRoomJson << MESSAGE_KEY_PLAYER_NAME << playerName;
        createRoomJson << MESSAGE_KEY_ROOM_NAME << roomName;
        createRoomJson << MESSAGE_KEY_ROOM_DESC << description;
        createRoomJson << MESSAGE_KEY_PLAYERS_LIMIT << playersLimit;
        createRoomJson << MESSAGE_KEY_PLAYERS_RESERVED << playersReserved;
        createRoomJson << MESSAGE_KEY_USER_PARAM << userParameter;
        std::string createRoomMessage = createRoomJson.write(JSON);

        mTcp->Send(createRoomMessage.c_str(), createRoomMessage.size(), *mServerAddress, false);
        shared_ptr<Packet> responce = awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS);
        if (!mMyRoom.deserialize(std::string(pointer_cast<const char*>(responce->data), responce->length))) {
            if (responsed(responce, constants::RESPONSE_ROOM_EXISTS)) {
                return ROOM_EXISTS;
            }
            return FAIL;
        }
        mPlayerName = playerName;
        mCallback = callback;
        mCallback->onJoined(this, mMyRoom);
        mIsJoined = true;
        mIsHost = true;
        return SUCCESS;
    }
    //-------------------------------------------------------

    Lobby::Status Lobby::createRoom(const std::string & playerName, const std::string & roomName, const std::string & description, uint32_t playersLimit, uint32_t playersReserved, LobbyCallback* callback)
    {
        return createRoom(playerName, roomName, description, playersLimit, playersReserved, std::string(), callback);
    }
    //-------------------------------------------------------

    void Lobby::reconfigure(uint32_t playersLimit, uint32_t playersReserved)
    {
        if (!(mIsJoined && mIsHost)) {
            throw ProtocolError("Lobby[reconfigure]: Can't reconfigure the room. I'm not the host!");
        }
        if (playersLimit < 1) {
            throw ProtocolError("Lobby[reconfigure]: players limit can't be less than 1");
        }
        if (playersReserved > playersLimit - 1) {
            throw ProtocolError("Lobby[reconfigure]: players reserved number can't be greater or equal to player limit");
        }
        Object reconfigureJson;
        reconfigureJson << MESSAGE_KEY_CLASS << frontend::RECONFIGURE;
        reconfigureJson << MESSAGE_KEY_PLAYERS_LIMIT << playersLimit;
        reconfigureJson << MESSAGE_KEY_PLAYERS_RESERVED << playersReserved;
        sendTcpMessage(makeEnvelop(reconfigureJson).write(JSON));
    }

    //-------------------------------------------------------

    const std::vector<RoomInfo>& Lobby::getRooms() const
    {
        mRooms.clear();
        Object jsonGetRooms;
        jsonGetRooms << MESSAGE_KEY_CLASS << frontend::GET_ROOMS;
        //UdpInterface::Instance().sendUpdDatagram(*mUpdSocket, mServerIp, mServerPort, jsonGetRooms.write(JSON));
        if (!mUdpInterface->sendUpdDatagram(jsonGetRooms.write(JSON))) {
            throw ServerError("Lobby[Lobby]: Failed to request rooms list!");
        }
        //size_t len = UdpInterface::Instance().awaitUdpDatagram(*mUpdSocket, mReceiveBuffer, DEFAULT_TIMEOUT_MS);
        std::string message = mUdpInterface->awaitUdpDatagram(DEFAULT_TIMEOUT_MS);
        if(message.empty()){
            throw ServerError("Lobby[Lobby]: Failed to get rooms list!");
        }
        Array roomsArray;
        roomsArray.parse(message);
        
        mRooms.resize(roomsArray.size());
        for (size_t i = 0; i < roomsArray.size(); ++i) {
            mRooms[i].deserialize(roomsArray.get<Object>(i));
        }
        return mRooms;
    }
    //-------------------------------------------------------

    Lobby::Status Lobby::joinRoom(const std::string & playerName, const RoomInfo & room, LobbyCallback* callback)
    {
        if (playerName.empty()) {
            throw ProtocolError("Lobby[joinRoom]: PlayerName can't be empty!");
        }
        if (room.getName().empty() || room.getHostName().empty()) {
            throw ProtocolError("Lobby[joinRoom]: Room info is invalid!");
        }
        if (callback == NULL) {
            throw ProtocolError("Lobby[joinRoom]: Callback can't be null!");
        }
        if (mIsJoined) {
            throw ProtocolError("Lobby[joinRoom]: Already joined!");
        }

        assert(mTcp.get() != NULL);
        assert(*mServerAddress != UNASSIGNED_SYSTEM_ADDRESS);

        Object joinRoomJson;
        joinRoomJson << MESSAGE_KEY_CLASS << frontend::JOIN_ROOM;
        joinRoomJson << MESSAGE_KEY_PLAYER_NAME << playerName;
        joinRoomJson << MESSAGE_KEY_ROOM_NAME << room.getName();
        sendTcpMessage(joinRoomJson.write(JSON));

        // Await sync responce 
        shared_ptr<Packet> packet = awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS);
        
        // Check responce
        if (!mMyRoom.deserialize(std::string(pointer_cast<const char*>(packet->data), packet->length))) {
            if (responsed(packet, constants::RESPONSE_ROOM_IS_FULL)) {
                return ROOM_IS_FULL;
            }
            if (responsed(packet, constants::RESPONSE_NAME_EXISTS)) {
                return NAME_EXISTS;
            }
            return FAIL;
        }

        // Ok
        mPlayerName = playerName;
        mCallback = callback;
        mCallback->onJoined(this, mMyRoom);
        mIsJoined = true;
        mIsHost = false;
        return SUCCESS;
    }
    //-------------------------------------------------------

    void Lobby::leaveRoom()
    {
        if (!mIsJoined) {
            throw ProtocolError("Lobby[leaveRoom]: Can't leave room. I'm not joined!");
        }
        Object leaveRoomJson;
        leaveRoomJson << MESSAGE_KEY_CLASS << frontend::LEAVE_ROOM;
        sendTcpMessage(makeEnvelop(leaveRoomJson).write(JSON));
        mCallback->onLeft(this, mMyRoom, 0);
        mIsJoined = false;
    }
    //-------------------------------------------------------

    void Lobby::eject(const std::string & playerName)
    {
        if (!(mIsJoined && mIsHost)) {
            throw ProtocolError("Lobby[eject]: Can't eject a player. I'm not the host!");
        }
        Object ejectPlayerJson;
        ejectPlayerJson << MESSAGE_KEY_CLASS << frontend::EJECT_PLAYER;
        ejectPlayerJson << MESSAGE_KEY_PLAYER_NAME << playerName;
        sendTcpMessage(makeEnvelop(ejectPlayerJson).write(JSON));
    }
    //-------------------------------------------------------

    void Lobby::closeRoom()
    {
        if (!(mIsJoined && mIsHost)) {
            throw ProtocolError("Lobby[closeRoom]: Can't close room. I'm not the host!");
        }
        Object closeRoomJson;
        closeRoomJson << MESSAGE_KEY_CLASS << frontend::CLOSE_ROOM;
        sendTcpMessage(closeRoomJson.write(JSON));
    }
    //-------------------------------------------------------

    void Lobby::startSession(const std::string & sessionData)
    {
        Object startSessionJson;
        startSessionJson << MESSAGE_KEY_CLASS << frontend::START_SESSION;
        startSessionJson << MESSAGE_KEY_DATA << sessionData;
        sendTcpMessage(makeEnvelop(startSessionJson).write(JSON));
    }
    //-------------------------------------------------------

    void Lobby::startSession()
    {
        startSession(std::string());
    }
    //-------------------------------------------------------

    void Lobby::say(const std::string & message, bool toSelf)
    {
        if (!mIsJoined) {
            throw ProtocolError("Lobby[broadcast]: I'm not in a room!");
        }
        Object sayJson;
        sayJson << MESSAGE_KEY_CLASS   << frontend::MESSAGE;
        sayJson << MESSAGE_KEY_SENDER  << mPlayerName;
        sayJson << MESSAGE_KEY_DATA    << message;
        sayJson << MESSAGE_KEY_TO_SELF << toSelf;
        sendTcpMessage(makeEnvelop(sayJson).write(JSON));
    }
    //-------------------------------------------------------

    uint32_t Lobby::receive()
    {
        if (!mIsJoined) {
            throw ProtocolError("Lobby[broadcast]: I'm not in a room!");
        }
        uint32_t counter = 0;
        while (mIsJoined) {
            shared_ptr<Packet> packet = awaitResponse(mTcp);
            if (packet == NULL) {
                break;
            }
            Object messageJson;
            messageJson.parse(std::string(pointer_cast<char*>(packet->data), packet->length));
            std::string messageClass(messageJson.get<std::string>(MESSAGE_KEY_CLASS, ""));
            if (isMessageClass(messageClass, frontend::BROADCAST)) {
                if (mMyRoom.deserialize(messageJson.get<Object>(constants::MESSAGE_KEY_ROOM, Object()))) {
                    mCallback->onRoomUpdate(this, mMyRoom, messageJson.get<jsonxx::String>(MESSAGE_KEY_SENDER), narrow_cast<uint8_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_FLAGS, 0.0)));
                }
            }
            else if (isMessageClass(messageClass, frontend::SESSION_STARTED)) {
                SessionPtr session(new Session(
                    messageJson.get<jsonxx::String>(MESSAGE_KEY_IP, ""),
                    narrow_cast<uint16_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_PORT, 0.0)),
                    mPlayerName,
                    messageJson.get<jsonxx::String>(MESSAGE_KEY_NAME, ""),
                    narrow_cast<uint32_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_ID, 0.0))), details::SessionDeleter());
                mCallback->onSessionStart(this, mMyRoom, session, messageJson.get<jsonxx::String>(MESSAGE_KEY_DATA, ""));
            }
            else if (isMessageClass(messageClass, frontend::EJECTED)) {
                mCallback->onLeft(this, mMyRoom, narrow_cast<uint8_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_FLAGS, 0.0)));
                mIsJoined = false;
            }
            else if (isMessageClass(messageClass, frontend::MESSAGE)) {
                mCallback->onMessage(this, mMyRoom, messageJson.get<jsonxx::String>(MESSAGE_KEY_SENDER, ""), messageJson.get<jsonxx::String>(MESSAGE_KEY_DATA, ""));
            }
            else if (isMessageClass(messageClass, frontend::RECONFIGURE_SUCC)) {
                if (mMyRoom.deserialize(messageJson.get<Object>(constants::MESSAGE_KEY_ROOM, Object()))) {
                    mCallback->onRoomUpdate(this, mMyRoom, mMyRoom.getHostName(), LobbyCallback::FLAG_RECONFIGURED_BY_HOST);
                }
            }
            else if (isMessageClass(messageClass, frontend::RECONFIGURE_SUCK)) {
                assert(mMyRoom.getHostName() == mPlayerName);
                mCallback->onRoomUpdate(this, mMyRoom, mPlayerName, LobbyCallback::FLAG_RECONFIGURE_FAIL);
            }
            else {
                throw ProtocolError("Lobby[receive]: Unknown message type");
            }
            ++counter;
        }
        return counter;
    }
    //-------------------------------------------------------

}
