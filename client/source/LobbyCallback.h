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
        MULTISLIDER_EXPORT static const uint8_t FLAG_IS_EJECTED;               ///< The player was ejected from the room
        MULTISLIDER_EXPORT static const uint8_t FLAG_JOINED;                   ///< Somebody joined the room
        MULTISLIDER_EXPORT static const uint8_t FLAG_LEFT;                     ///< Somebody left the room
        MULTISLIDER_EXPORT static const uint8_t FLAG_NEW_HOST;                 ///< Room has a new host
        MULTISLIDER_EXPORT static const uint8_t FLAG_ROOM_CLOSED_BY_HOST;      ///< The player was ejected because the room was closed by host

        virtual ~LobbyCallback() 
        { }

        /**
         *  Player has joined the room
         *  Is called immediately for host after creating room and for client after join request
         */
        virtual void onJoined(Lobby* /*lobby*/, const RoomInfo & /*room*/)
        { }

        /**
         *  Player left the room
         */
        virtual void onLeft(Lobby* /*lobby*/, const RoomInfo & /*room*/, uint8_t /*flags*/)
        { }

        /**
         *  Is called for each broadcast message
         */
        virtual void onBroadcast(Lobby* /*lobby*/, const RoomInfo & /*room*/, const std::string & /*sender*/, const std::string & /*message*/, uint8_t /*flags*/)
        { }

        /**
         *  Is called as soon as a host has started a session
         */
        virtual void onSessionStart(Lobby* /*lobby*/, const RoomInfo & /*room*/, SessionPtr /*session*/, const std::string & sessionData)
        { }
    };

}

#endif
