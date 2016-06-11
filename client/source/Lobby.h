/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_LOBBY_H_
#define _MULTI_SLIDER_LOBBY_H_

#include "LibInterface.h"
#include "CommonIncludes.h"
#include "Host.h"
#include "Exception.h"

namespace multislider
{
    
    class Lobby
    {
        shared_ptr<RakNet::TCPInterface> mTcp;

        shared_ptr<Host> mHostInstance;

        Lobby(const Lobby &);
        Lobby & operator=(const Lobby &);

    public:
        MULTISLIDER_EXPORT
        Lobby() throw(Fail);

        MULTISLIDER_EXPORT
        ~Lobby();

        MULTISLIDER_EXPORT
        Host* becomeHost(HostCallback* callback);
    };

}

#endif
