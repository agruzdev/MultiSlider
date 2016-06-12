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

#include "jsonxx.h"

using namespace RakNet;
using namespace jsonxx;

namespace
{
    static const char SERVER_ADDRESS[] = "127.0.0.1";
    static const uint16_t SERVER_FRONTEND_PORT = 8800;
    static const uint16_t SERVER_BACKEND_PORT  = 8700;

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

    Lobby::Lobby()
    {
        mTcp.reset(RakNet::TCPInterface::GetInstance(), TcpDeleter());
        if (!mTcp->Start(8801, 64)) {
            throw NetworkError("Lobby[Lobby]: Failed to start TCP interface");
        }
        mServerAddress.reset(new SystemAddress);
        *mServerAddress = mTcp->Connect(SERVER_ADDRESS, SERVER_FRONTEND_PORT);
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

    Host* Lobby::createRoom(const std::string & playerName, const std::string & roomName, HostCallback* callback)
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
        mHostInstance.reset(new Host(mTcp, mServerAddress, playerName, roomName, callback));
        return mHostInstance.get();
    }
    //-------------------------------------------------------

    std::vector<RoomInfo> Lobby::getRooms() const
    {
        assert(mTcp.get() != NULL);

        Object jsonGetRooms;
        jsonGetRooms << constants::MESSAGE_KEY_CLASS << constants::MESSAGE_CLASS_GET_ROOMS;
        std::string message = jsonGetRooms.write(JSON);
        mTcp->Send(message.c_str(), message.size(), *mServerAddress, false);
        Packet* packet = awaitResponse(mTcp, constants::DEFAULT_TIMEOUT_MS);
        if (packet == NULL || responsed(packet, constants::RESPONSE_SUCK)) {
            throw ServerError("Lobby[Lobby]: Failed to get rooms list!");
        }

        Array roomsArray;
        roomsArray.parse(std::string(castPointerTo<char*>(packet->data), packet->length));
        std::vector<RoomInfo> rooms;
        rooms.resize(roomsArray.size());
        for (size_t i = 0; i < roomsArray.size(); ++i) {
            Object jsonRoom = roomsArray.get<Object>(i);
            rooms[i].roomName = jsonRoom.get<std::string>("name", "<Unknown>");
            rooms[i].hostName = jsonRoom.get<std::string>("host", "<Unknown>");
        }
        return rooms;
    }
}
