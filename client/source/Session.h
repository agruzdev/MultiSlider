/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_SESSION_H_
#define _MULTI_SLIDER_SESSION_H_

#include "CommonIncludes.h"
#include "LibInterface.h"

namespace RakNet
{
    class RakPeer;
    struct SystemAddress;
}


namespace multislider
{
    
    class Session
    {
        shared_ptr<RakNet::SystemAddress> mServerAddress;
        const std::string mPlayerName;
        const std::string mSessionName;
        const uint32_t mSessionId;

    public:
        Session(std::string address, const std::string & playerName, const std::string & sessionName, uint32_t sessionId);

        ~Session();

        MULTISLIDER_EXPORT
        static void DestoyInstance(Session* session);
    };


    namespace details
    {
        struct SessionDeleter
        {
            void operator()(Session* ptr) const
            {
                Session::DestoyInstance(ptr);
            }
        };
    }

    typedef shared_ptr<Session> SessionPtr;

}

#endif 

