/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_LOBBY_CALLBACK_H_
#define _MULTI_SLIDER_LOBBY_CALLBACK_H_

#include "CommonIncludes.h"

namespace multislider
{

    class LobbyCallback
    {
    public:
        virtual ~LobbyCallback() 
        { }

        /**
         *  Player has joined the room
         *  Is called immediately for host after creating room and for client after join request
         */
        virtual void onJoined(Lobby* /*lobby*/, const RoomInfo & /*room*/, const std::string & /*playerName*/)
        { }

        /**
         *  Player left the room
         */
        virtual void onLeft(Lobby* /*lobby*/, const RoomInfo & /*room*/, const std::string & /*playerName*/, uint8_t /*flags*/)
        { }

        /**
         *  Is called for each broadcast message
         */
        virtual void onBroadcast(Lobby* /*lobby*/, const RoomInfo & /*room*/, const std::string & /*playerName*/, const std::string & /*message*/)
        { }

        /**
         *  Is called as soon as a host has started a session
         */
        virtual void onSessionStart(Lobby* /*lobby*/, const RoomInfo & /*room*/, const std::string & /*playerName*/, SessionPtr /*session*/)
        { }
    };

}

#endif
