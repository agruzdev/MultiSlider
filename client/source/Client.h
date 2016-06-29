/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_CLIENT_H_
#define _MULTI_SLIDER_CLIENT_H_

#include "CommonIncludes.h"
#include "RoomInfo.h"
#include "LibInterface.h"
#include "Session.h"

namespace RakNet
{
    class TCPInterface;
    struct SystemAddress;
}

namespace multislider
{

    class ClientCallback
    {
    public:
        virtual ~ClientCallback() {}

        /**
         *  Is called as soon as player 'playerName' has joined the room 'room'
         */
        virtual void onJoined(const std::string & /*playerName*/, const RoomInfo & /*room*/) { }

        /**
         *  Is called after the player left the room
         */
        virtual void onLeft(const std::string & /*playerName*/, const RoomInfo & /*room*/) { }

        /**
         *  Is called for each broadcast message
         */
        virtual void onBroadcast(const std::string & /*playerName*/, const std::string & /*message*/) { }

        /**
         *  Is called as soon as a host has started a session
         */
        virtual void onSessionStart(const std::string & /*playerName*/, SessionPtr /*session*/) { }
    };

    class Client
    {
        shared_ptr<RakNet::TCPInterface> mTcp;
        shared_ptr<RakNet::SystemAddress> mServerAddress;

        ClientCallback* mCallback;
        RoomInfo mMyRoom;
        std::string mPlayerName;
        bool mIsJoined;

        //-------------------------------------------------------

        Client(const Client &);
        Client & operator=(const Client &);

    public:
        Client(shared_ptr<RakNet::TCPInterface> connection, shared_ptr<RakNet::SystemAddress> address, const std::string & playerName, const RoomInfo & room, ClientCallback* callback);

        ~Client();

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
        void broadcast(const std::string & data);

        /**
         *  Check incoming broadcast messages and call callback for the each message
         *  @return number of processed messages
         */
        MULTISLIDER_EXPORT
        uint32_t receive() const;

    };

}

#endif
