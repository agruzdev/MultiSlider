/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_HOST_H_
#define _MULTI_SLIDER_HOST_H_

#include "CommonIncludes.h"

namespace RakNet
{
    class TCPInterface;
}

namespace multislider
{

    class HostCallback
    {
    public:
        ~HostCallback() {}

    };

    class Host
    {
        shared_ptr<RakNet::TCPInterface> mTcp;
        HostCallback* mCallback;

        Host(const Host &);
        Host & operator=(const Host &);

    public:
        Host(shared_ptr<RakNet::TCPInterface> connection, HostCallback* callback);
        ~Host();

        //-------------------------------------------------------
        friend class Lobby;
    };

}

#endif
