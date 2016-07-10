/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#include "TCPInterface.h"

#include "Lobby.h"
#include "Host.h"
#include "Client.h"
#include "Constants.h"
#include "Utility.h"
#include "Exception.h"

#include "jsonxx.h"

using namespace RakNet;
using namespace jsonxx;

namespace
{
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
        : mCallback(NULL), mIsJoined(false), mIsHost(false)
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
    }
    //-------------------------------------------------------

    Lobby::~Lobby()
    {
        if (mIsJoined) {
            try {
                if (mIsHost) {
                    closeRoom();
                }
            }
            catch (RuntimeError &) {
            }
        }
    }
    //-------------------------------------------------------

    bool Lobby::createRoom(const std::string & playerName, const std::string & roomName, uint32_t playersLimit, LobbyCallback* callback)
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
            return false;
        }
        mPlayerName = playerName;
        mCallback = callback;
        mCallback->onJoined(this, mMyRoom, mPlayerName);
        mIsJoined = true;
        mIsHost = true;
        return true;
    }
    //-------------------------------------------------------

    std::vector<RoomInfo> Lobby::getRooms() const
    {
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
            //Object jsonRoom = roomsArray.get<Object>(i);
            //rooms[i].roomName = jsonRoom.get<std::string>(MESSAGE_KEY_NAME, "<Unknown>");
            //rooms[i].hostName = jsonRoom.get<std::string>(MESSAGE_KEY_HOST, "<Unknown>");
            //rooms[i].playersLimit  = narrow_cast<uint32_t>(jsonRoom.get<jsonxx::Number>(MESSAGE_KEY_PLAYERS_LIMIT, 0));
            //rooms[i].playersNumber = narrow_cast<uint32_t>(jsonRoom.get<jsonxx::Number>(MESSAGE_KEY_PLAYERS_NUMBER, 0));
            rooms[i].deserialize(roomsArray.get<Object>(i));
        }
        return rooms;
    }
    //-------------------------------------------------------

    Client* Lobby::joinRoom(const std::string & playerName, const RoomInfo & room, LobbyCallback* callback, bool & isFull)
    {
        if (playerName.empty()) {
            throw ProtocolError("Lobby[createRoom]: playerName can't be empty!");
        }
        if (room.getName().empty() || room.getHostName().empty()) {
            throw ProtocolError("Lobby[createRoom]: room info is invalid!");
        }
        if (callback == NULL) {
            throw ProtocolError("Lobby[createRoom]: callback can't be null!");
        }
        mClientInstance.reset(new Client(mTcp, mServerAddress, playerName, room, callback));
        isFull = false;
        switch (mClientInstance->join(room.getName())) {
        case 0:
            return mClientInstance.get();
        case 2:
            isFull = true;
        case 1:
        default:
            mClientInstance.reset();
            return NULL;
        }
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
        std::string startSessionMessage = startSessionJson.write(JSON);
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
        std::string broadcastMessage = broadcastJson.write(JSON);
        mTcp->Send(broadcastMessage.c_str(), broadcastMessage.size(), *mServerAddress, false);
    }
    //-------------------------------------------------------

    uint32_t Lobby::receive()
    {
        if (!mIsJoined) {
            throw ProtocolError("Lobby[broadcast]: I'm not in a room!");
        }
        uint32_t counter = 0;
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
                    mCallback->onBroadcast(NULL, mMyRoom, mPlayerName, message);
                }
            }
            else if (isMessageClass(messageClass, frontend::SESSION_STARTED)) {
                SessionPtr session(new Session(
                    messageJson.get<jsonxx::String>(MESSAGE_KEY_IP, ""),
                    narrow_cast<uint16_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_PORT, 0.0)),
                    mPlayerName,
                    messageJson.get<jsonxx::String>(MESSAGE_KEY_NAME, ""),
                    narrow_cast<uint32_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_ID, 0.0))), details::SessionDeleter());
                mCallback->onSessionStart(NULL, mMyRoom, mPlayerName, session);
            }
            ++counter;
        }
        return counter;
    }
    //-------------------------------------------------------

}
