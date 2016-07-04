/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_HOST_H_
#define _MULTI_SLIDER_HOST_H_

#include "CommonIncludes.h"
#include "LibInterface.h"
#include "RoomInfo.h"
#include "Session.h"

namespace RakNet
{
    class TCPInterface;
    struct SystemAddress;
}

namespace multislider
{

    class HostCallback
    {
    public:
        virtual ~HostCallback() {}

        /**
         *  Is called as soon as the room is created on the server
         */
        virtual void onCreated(const RoomInfo & /*room*/) { }

        /**
         *  Is called after server room is closed
         */
        virtual void onClosed(const RoomInfo & /*room*/) { }

        /**
         *  Is called for each broadcast message
         */
        virtual void onBroadcast(const RoomInfo & /*room*/, const std::string & /*message*/) { }

        /**
         *  Is called as soon as a host has started a session
         */
        virtual void onSessionStart(const RoomInfo & /*room*/, SessionPtr /*session*/) { }
    };

    class Host
    {
        shared_ptr<RakNet::TCPInterface> mTcp;
        shared_ptr<RakNet::SystemAddress> mServerAddress;
        
        HostCallback* mCallback;
        RoomInfo mMyRoom;
        bool mIsOpened;
        //-------------------------------------------------------

        Host(const Host &);
        Host & operator=(const Host &);

    public:
        Host(shared_ptr<RakNet::TCPInterface> connection, shared_ptr<RakNet::SystemAddress> address, const std::string & playerName, const std::string & roomName, uint32_t playersLimit, HostCallback* callback);
    
        ~Host();

        /**
         *  Close the current room
         */
        MULTISLIDER_EXPORT
        void closeRoom();

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
        uint32_t receive() const;

        /**
         *  Start game session for all joined players
         */
        MULTISLIDER_EXPORT
        void startSession();
    };

}

#endif
