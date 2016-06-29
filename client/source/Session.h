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
    class RakPeerInterface;
    struct SocketDescriptor;
}

namespace jsonxx
{
    class Object;
}

namespace multislider
{

    class SessionCallback
    {
    public:
        virtual ~SessionCallback() { }

        /**
         *  Is called as soon as all players have joined the session and are ready to start
         */
        virtual void onStart() { }

    }; 

    class Session
    {
        shared_ptr<RakNet::SocketDescriptor> mSocket;
        shared_ptr<RakNet::RakPeerInterface> mPeer;
        const std::string mServerIp;
        const uint16_t mServerPort;
        const std::string mPlayerName;
        const std::string mSessionName;
        const uint32_t mSessionId;
        SessionCallback* mCallback;
        bool mStarted;
        //-------------------------------------------------------
        
        jsonxx::Object makeEnvelop(const jsonxx::Object & obj) const;

    public:
        Session(std::string ip, uint16_t port, const std::string & playerName, const std::string & sessionName, uint32_t sessionId);

        ~Session();

        MULTISLIDER_EXPORT
        static void DestoyInstance(Session* session);

        /**
         *  Call as soon as you are ready to start
         *  @param callback Session callback can't be null
         */
        MULTISLIDER_EXPORT
        void Startup(SessionCallback* callback);

        /**
         *  Receive and handle incoming messages
         */
        MULTISLIDER_EXPORT
        uint32_t receive();
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

