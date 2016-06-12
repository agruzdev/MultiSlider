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
        virtual void onCreated(RoomInfo & room) { };

        /**
         *  Is called after server room is closed
         */
        virtual void onClosed(RoomInfo & room) { };
    };

    class Host
    {
        shared_ptr<RakNet::TCPInterface> mTcp;
        shared_ptr<RakNet::SystemAddress> mServerAddress;
        
        HostCallback* mCallback;
        RoomInfo mMyRoom;
        bool mIsOpened;
        //-------------------------------------------------------

        static std::string makeMsgCreateRoom(const std::string & playerName, const std::string & roomName);

        Host(const Host &);
        Host & operator=(const Host &);

    public:
        Host(shared_ptr<RakNet::TCPInterface> connection, shared_ptr<RakNet::SystemAddress> address, const std::string & playerName, const std::string & roomName, HostCallback* callback);
    
        ~Host();

        /**
         *  Close the current room
         */
        MULTISLIDER_EXPORT
        void closeRoom();
    };

}

#endif
