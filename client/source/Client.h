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
        virtual void onJoined(const std::string & playerName, const RoomInfo & room) { };

        /**
         *  Is called after the player left the room
         */
        virtual void onLeft(const std::string & playerName, const RoomInfo & room) { };
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

        MULTISLIDER_EXPORT
        void leaveRoom();
    };

}

#endif
