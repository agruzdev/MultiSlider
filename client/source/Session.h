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
    class UdpSocket;

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
        virtual void onUpdate(const std::string & /*sessionName*/, const std::string & /*playerName*/, const SessionData & /*data*/, const std::string & /*sharedData*/) { }

        /**
         *  Is called as soon as synchronization message is got
         */
        virtual void onSync(const std::string & /*sessionName*/, const std::string & /*playerName*/, uint32_t /*syncId*/) { }
    }; 

    class Session
    {
        shared_ptr<UdpSocket> mSocket;
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

        /**
         *  Exported for smart pointer
         *  Don't use manually!
         */
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
         *  @param data user data to send to the server
         *  @param forced if true then session stated will be send to all players, otherwise they will get it only after own update
         */
        MULTISLIDER_EXPORT
        void broadcast(const std::string & data, bool forced);

        /**
        *  Broadcast user data to all players
        *  @param data user data to send to the server
        *  @param sharedData data shared by all users, is overwritten by all users
        *  @param forced if true then session stated will be send to all players, otherwise they will get it only after own update
        */
        MULTISLIDER_EXPORT
        void broadcast(const std::string & data, const std::string & sharedData, bool forced);

        /**
         *  Send synchronization message to all players
         *  @param syncId arbitrary id of the synchronization 
         *  @param delay delay time before sending synchronization message to all players [in milliseconds]
         */
        MULTISLIDER_EXPORT
        void sync(uint32_t syncId, uint64_t delay);

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

}

#endif 

