/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_LOBBY_H_
#define _MULTI_SLIDER_LOBBY_H_

#include <vector>

#include "LibInterface.h"
#include "CommonIncludes.h"
#include "Host.h"
#include "Exception.h"

namespace multislider
{

    struct RoomInfo
    {
        std::string hostName;
        std::string roomName;
    };

    class Lobby
    {
        shared_ptr<RakNet::TCPInterface> mTcp;

        shared_ptr<Host> mHostInstance;

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
         *  @param callback callback for reacting events from the server, can't be null
         */
        MULTISLIDER_EXPORT
        Host* createRoom(const std::string & playerName, const std::string & roomName, HostCallback* callback);

        MULTISLIDER_EXPORT
        std::vector<RoomInfo> getRooms() const;

    };

}

#endif
