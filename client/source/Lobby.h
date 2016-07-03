/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_LOBBY_H_
#define _MULTI_SLIDER_LOBBY_H_

#include <vector>

#include "Host.h"
#include "Client.h"

namespace multislider
{

    class Lobby
    {
        shared_ptr<RakNet::TCPInterface> mTcp;
        shared_ptr<RakNet::SystemAddress> mServerAddress;

        shared_ptr<Host>   mHostInstance;
        shared_ptr<Client> mClientInstance;

        Lobby(const Lobby &);
        Lobby & operator=(const Lobby &);

    public:
        MULTISLIDER_EXPORT
        Lobby();

        MULTISLIDER_EXPORT
        ~Lobby();

        /**
         *  Create new room on the server
         *  @param playerName name of the current player. It will be assigned as host of the created room
         *  @param roomName name of the room
         *  @param palyersLimit maximum number of players in the room (should be >= 1)
         *  @param callback callback for host events, can't be null
         */
        MULTISLIDER_EXPORT
        Host* createRoom(const std::string & playerName, const std::string & roomName, uint32_t playersLimit, HostCallback* callback);

        /**
         *  Get a list of all opened rooms on the server
         */
        MULTISLIDER_EXPORT
        std::vector<RoomInfo> getRooms() const;

        /**
         *  Join a room
         *  @param playerName name of the current player
         *  @param room reference to the room info to join
         *  @param callback callback for client events, can't be null
         */
        MULTISLIDER_EXPORT
        Client* joinRoom(const std::string & playerName, const RoomInfo & room, ClientCallback* callback, bool & isFull);
    };

}

#endif
