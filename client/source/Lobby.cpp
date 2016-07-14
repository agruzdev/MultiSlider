/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#include "TCPInterface.h"

#include "Lobby.h"
#include "Constants.h"
#include "Utility.h"
#include "Exception.h"
#include "Session.h"
#include "UdpInterface.h"

#include "jsonxx.h"


using namespace RakNet;
using namespace jsonxx;

namespace
{
    static const size_t RECEIVE_BIFFER_SIZE = 4096;

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
        if (!responsed(awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS), constants::RESPONSE_GREET)) {
            throw ProtocolError("Lobby[Lobby]: Unrecognized greeting");
        }
        mUpdSocket.reset(new UdpSocket);
        mReceiveBuffer.resize(RECEIVE_BIFFER_SIZE);
    }
    //-------------------------------------------------------

    Lobby::~Lobby()
    {
        if (mIsJoined) {
            try {
                if (mIsHost) {
                    closeRoom();
                }
                else {
                    leaveRoom();
                }
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

    Lobby::Status Lobby::createRoom(const std::string & playerName, const std::string & roomName, uint32_t playersLimit, LobbyCallback* callback)
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
        assert(mTcp.get() != NULL);
        assert(*mServerAddress != UNASSIGNED_SYSTEM_ADDRESS);

        Object createRoomJson;
        createRoomJson << MESSAGE_KEY_CLASS << frontend::CREATE_ROOM;
        createRoomJson << MESSAGE_KEY_PLAYER_NAME << playerName;
        createRoomJson << MESSAGE_KEY_ROOM_NAME << roomName;
        createRoomJson << MESSAGE_KEY_PLAYERS_LIMIT << playersLimit;
        std::string createRoomMessage = createRoomJson.write(JSON);

        mTcp->Send(createRoomMessage.c_str(), createRoomMessage.size(), *mServerAddress, false);
        shared_ptr<Packet> responce = awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS);
        if (!mMyRoom.deserialize(std::string(pointer_cast<const char*>(responce->data), responce->length))) {
            return FAIL;
        }
        mPlayerName = playerName;
        mCallback = callback;
        mCallback->onJoined(this, mMyRoom, mPlayerName);
        mIsJoined = true;
        mIsHost = true;
        return SUCCESS;
    }
    //-------------------------------------------------------

    std::vector<RoomInfo> Lobby::getRooms() const
    {
#if 0
        assert(mTcp.get() != NULL);

        Object jsonGetRooms;
        jsonGetRooms << MESSAGE_KEY_CLASS << frontend::GET_ROOMS;
        std::string message = jsonGetRooms.write(JSON);
        mTcp->Send(message.c_str(), message.size(), *mServerAddress, false);
        shared_ptr<Packet> packet = awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS);
        if (packet == NULL || responsed(packet, constants::RESPONSE_SUCK)) {
            throw ServerError("Lobby[Lobby]: Failed to get rooms list!");
        }

        Array roomsArray;
        roomsArray.parse(std::string(pointer_cast<char*>(packet->data), packet->length));
        std::vector<RoomInfo> rooms;
        rooms.resize(roomsArray.size());
        for (size_t i = 0; i < roomsArray.size(); ++i) {
            rooms[i].deserialize(roomsArray.get<Object>(i));
        }
        return rooms;
#else
        Object jsonGetRooms;
        jsonGetRooms << MESSAGE_KEY_CLASS << frontend::GET_ROOMS;
        UdpInterface::Instance().sendUpdDatagram(*mUpdSocket, mServerIp, mServerPort, jsonGetRooms.write(JSON));
        size_t len = UdpInterface::Instance().awaitUdpDatagram(*mUpdSocket, mReceiveBuffer, DEFAULT_TIMEOUT_MS);
        if(0 == len){
            throw ServerError("Lobby[Lobby]: Failed to get rooms list!");
        }
        Array roomsArray;
        roomsArray.parse(std::string(pointer_cast<char*>(&mReceiveBuffer[0]), len));
        std::vector<RoomInfo> rooms;
        rooms.resize(roomsArray.size());
        for (size_t i = 0; i < roomsArray.size(); ++i) {
            rooms[i].deserialize(roomsArray.get<Object>(i));
        }
        return rooms;
#endif
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
        std::string joinRoomMessage = joinRoomJson.write(JSON);
        mTcp->Send(joinRoomMessage.c_str(), joinRoomMessage.size(), *mServerAddress, false);
        shared_ptr<Packet> packet = awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS);
        if (responsed(packet, constants::RESPONSE_ROOM_IS_FULL)) {
            return ROOM_IS_FULL;
        }
        if (!mMyRoom.deserialize(std::string(pointer_cast<const char*>(packet->data), packet->length))) {
            return FAIL;
        }

        mPlayerName = playerName;
        mCallback = callback;
        mCallback->onJoined(this, mMyRoom, mPlayerName);
        mIsJoined = true;
        mIsHost = false;
        return SUCCESS;
    }
    //-------------------------------------------------------

    void Lobby::leaveRoom()
    {
        if (!(mIsJoined && !mIsHost)) {
            throw ProtocolError("Lobby[closeRoom]: Can't close room. I'm not the host!");
        }
        Object leaveRoomJson;
        leaveRoomJson << MESSAGE_KEY_CLASS << frontend::LEAVE_ROOM;
        std::string leaveRoomMessage = makeEnvelop(leaveRoomJson).write(JSON);
        mTcp->Send(leaveRoomMessage.c_str(), leaveRoomMessage.size(), *mServerAddress, false);
        //if (!responsed(awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS), constants::RESPONSE_SUCC)) {
        //    throw ServerError("Client[Client]: Failed to leave a room");
        //}
        mCallback->onLeft(this, mMyRoom, mPlayerName, 0);
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
        std::string ejectPlayerMessage = makeEnvelop(ejectPlayerJson).write(JSON);
        mTcp->Send(ejectPlayerMessage.c_str(), ejectPlayerMessage.size(), *mServerAddress, false);
    }
    //-------------------------------------------------------

    void Lobby::closeRoom()
    {
        if (!(mIsJoined && mIsHost)) {
            throw ProtocolError("Lobby[closeRoom]: Can't close room. I'm not the host!");
        }
        Object closeRoomJson;
        closeRoomJson << MESSAGE_KEY_CLASS << frontend::CLOSE_ROOM;
        std::string closeRoomMessage = closeRoomJson.write(JSON);
        mTcp->Send(closeRoomMessage.c_str(), closeRoomMessage.size(), *mServerAddress, false);
        if (!responsed(awaitResponse(mTcp, DEFAULT_TIMEOUT_MS), RESPONSE_SUCC)) {
            throw ServerError("Host[closeRoom]: failed to close the current room!");
        }
        mCallback->onLeft(this, mMyRoom, mPlayerName, 0);
        mIsJoined = false;
    }
    //-------------------------------------------------------

    void Lobby::startSession()
    {
        Object startSessionJson;
        startSessionJson << MESSAGE_KEY_CLASS << frontend::START_SESSION;
        std::string startSessionMessage = makeEnvelop(startSessionJson).write(JSON);
        mTcp->Send(startSessionMessage.c_str(), startSessionMessage.size(), *mServerAddress, false);
    }
    //-------------------------------------------------------

    void Lobby::broadcast(const std::string & data, bool toSelf)
    {
        if (!mIsJoined) {
            throw ProtocolError("Lobby[broadcast]: I'm not in a room!");
        }
        Object broadcastJson;
        broadcastJson << MESSAGE_KEY_CLASS << frontend::BROADCAST;
        broadcastJson << MESSAGE_KEY_ROOM << jsonxx::Null();
        broadcastJson << MESSAGE_KEY_DATA << data;
        broadcastJson << MESSAGE_KEY_TO_SELF << toSelf;
        std::string broadcastMessage = makeEnvelop(broadcastJson).write(JSON);
        mTcp->Send(broadcastMessage.c_str(), broadcastMessage.size(), *mServerAddress, false);
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
            std::string messageClass(messageJson.get<std::string>(MESSAGE_KEY_CLASS));
            if (isMessageClass(messageClass, frontend::BROADCAST)) {
                std::string message = messageJson.get<std::string>(constants::MESSAGE_KEY_DATA, "");
                if (mMyRoom.deserialize(messageJson.get<Object>(constants::MESSAGE_KEY_ROOM, Object()))) {
                    mCallback->onBroadcast(this, mMyRoom, mPlayerName, message);
                }
            }
            else if (isMessageClass(messageClass, frontend::SESSION_STARTED)) {
                SessionPtr session(new Session(
                    messageJson.get<jsonxx::String>(MESSAGE_KEY_IP, ""),
                    narrow_cast<uint16_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_PORT, 0.0)),
                    mPlayerName,
                    messageJson.get<jsonxx::String>(MESSAGE_KEY_NAME, ""),
                    narrow_cast<uint32_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_ID, 0.0))), details::SessionDeleter());
                mCallback->onSessionStart(this, mMyRoom, mPlayerName, session);
            }
            else if (isMessageClass(messageClass, frontend::EJECTED)) {
                mCallback->onLeft(this, mMyRoom, mPlayerName, narrow_cast<uint8_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_FLAGS, 0.0)));
                mIsJoined = false;
            }
            ++counter;
        }
        return counter;
    }
    //-------------------------------------------------------

}
