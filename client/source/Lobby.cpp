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

    Lobby::Lobby(const std::string & serverIp, uint16_t serverPort)
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
    { }
    //-------------------------------------------------------

    Host* Lobby::createRoom(const std::string & playerName, const std::string & roomName, uint32_t playersLimit, HostCallback* callback)
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
        mHostInstance.reset(new Host(mTcp, mServerAddress, playerName, roomName, playersLimit, callback));
        return mHostInstance.get();
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
            Object jsonRoom = roomsArray.get<Object>(i);
            rooms[i].roomName = jsonRoom.get<std::string>(MESSAGE_KEY_NAME, "<Unknown>");
            rooms[i].hostName = jsonRoom.get<std::string>(MESSAGE_KEY_HOST, "<Unknown>");
            rooms[i].playersLimit  = narrow_cast<uint32_t>(jsonRoom.get<jsonxx::Number>(MESSAGE_KEY_PLAYERS_LIMIT, 0));
            rooms[i].playersNumber = narrow_cast<uint32_t>(jsonRoom.get<jsonxx::Number>(MESSAGE_KEY_PLAYERS_NUMBER, 0));
        }
        return rooms;
    }
    //-------------------------------------------------------

    Client* Lobby::joinRoom(const std::string & playerName, const RoomInfo & room, ClientCallback* callback, bool & isFull)
    {
        if (playerName.empty()) {
            throw ProtocolError("Lobby[createRoom]: playerName can't be empty!");
        }
        if (room.roomName.empty() || room.hostName.empty()) {
            throw ProtocolError("Lobby[createRoom]: room info is invalid!");
        }
        if (callback == NULL) {
            throw ProtocolError("Lobby[createRoom]: callback can't be null!");
        }
        mClientInstance.reset(new Client(mTcp, mServerAddress, playerName, room, callback));
        isFull = false;
        switch (mClientInstance->join()) {
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
}
