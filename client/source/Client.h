/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_CLIENT_H_
#define _MULTI_SLIDER_CLIENT_H_

#include "LobbyCallback.h"
#include "RoomInfo.h"

namespace multislider
{
    class Client
    {
        shared_ptr<RakNet::TCPInterface> mTcp;
        shared_ptr<RakNet::SystemAddress> mServerAddress;

        LobbyCallback* mCallback;
        RoomInfo mMyRoom;
        std::string mPlayerName;
        bool mIsJoined;

        //-------------------------------------------------------

        Client(const Client &);
        Client & operator=(const Client &);

    public:
        Client(shared_ptr<RakNet::TCPInterface> connection, shared_ptr<RakNet::SystemAddress> address, const std::string & playerName, const RoomInfo & room, LobbyCallback* callback);

        ~Client();

        /**
         * Connect server and join the room
         */
        int join(const std::string & roomName);

        /**
         *  Leave the current room
         *  After leaving Client instance can't be reused for another room
         */
        MULTISLIDER_EXPORT
        void leaveRoom();

        /**
         *  Broadcast data to all players in the room
         *  Not blocking call
         */
        MULTISLIDER_EXPORT
        void broadcast(const std::string & data, bool toSelf);

        /**
         *  Check incoming broadcast messages and call callback for the each message
         *  @return number of processed messages
         */
        MULTISLIDER_EXPORT
        uint32_t receive();

    };

}

#endif
