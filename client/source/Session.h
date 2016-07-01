/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_SESSION_H_
#define _MULTI_SLIDER_SESSION_H_

#include <vector>
#include <map>

#include "CommonIncludes.h"
#include "LibInterface.h"

namespace jsonxx
{
    class Object;
}

namespace multislider
{
    typedef std::map<std::string, std::string> SessionData;

    class SessionCallback
    {
    public:
        virtual ~SessionCallback() { }

        /**
         *  Is called as soon as all players have joined the session and are ready to start
         */
        virtual void onStart(const std::string & /*sessionName*/, const std::string & /*playerName*/) { }

        /**
         *  Is called as soon as server responsed broadcast message
         */
        virtual void onUpdate(const std::string & /*sessionName*/, const std::string & /*playerName*/, const SessionData & /*data*/) { }
    }; 

    struct SocketImpl;

    class Session
    {
        static bool msEnetInited;
        //-------------------------------------------------------

        shared_ptr<SocketImpl> mSocket;
        const std::string mServerIp;
        const uint16_t mServerPort;
        const std::string mPlayerName;
        const std::string mSessionName;
        const uint32_t mSessionId;
        SessionCallback* mCallback;
        bool mStarted;

        std::vector<uint8_t> mReceiveBuffer;
        //-------------------------------------------------------
        

        jsonxx::Object makeEnvelop(const jsonxx::Object & obj) const;

        void sendUpdDatagram(const std::string & message) const;

        /**
         *  Returns datagram length
         */
        size_t awaitUdpDatagram(uint64_t timeoutMilliseconds, uint32_t attemptsTimeoutMilliseconds = 100);
        //-------------------------------------------------------

    public:
        Session(std::string ip, uint16_t port, const std::string & playerName, const std::string & sessionName, uint32_t sessionId);

        ~Session();

        MULTISLIDER_EXPORT
        static void destoyInstance(Session* session);

        /**
         *  Call as soon as you are ready to start
         *  @param callback Session callback can't be null
         */
        MULTISLIDER_EXPORT
        void startup(SessionCallback* callback, uint64_t timeout);

        /**
         *  Broadcast user data to all players
         */
        MULTISLIDER_EXPORT
        void broadcast(const std::string & data);

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
                Session::destoyInstance(ptr);
            }
        };
    }

    typedef shared_ptr<Session> SessionPtr;

}

#endif 

