/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_SESSION_H_
#define _MULTI_SLIDER_SESSION_H_


#include "CommonIncludes.h"
#include "SessionCallback.h"

namespace jsonxx
{
    class Object;
}

namespace multislider
{
    class UdpSocket;
    struct MsgInfo;

    class Session
    {
        shared_ptr<UdpSocket> mSocket;
        const std::string mServerIp;
        const uint16_t mServerPort;
        const std::string mPlayerName;
        const std::string mSessionName;
        const uint32_t mSessionId;
        SessionCallback* mCallback;
        uint64_t mLastTimestamp;
        uint64_t mLastPing;
        bool mStarted;

        std::vector<uint8_t> mReceiveBuffer;

        /*
         * Support reliable connection
         */
        uint32_t mLocalSeqIdx;
        uint32_t mRemoteSeqIdx;
        uint32_t mPreviousIdxBits;
        std::vector<shared_ptr<MsgInfo>> mOutputQueue;

        //-------------------------------------------------------
        

        jsonxx::Object makeEnvelop(const jsonxx::Object & obj) const;

        void sendUpdDatagram(const std::string & message) const;

        /*
         *  Generate next seq idx
         *  Wrap on overflow
         */
        uint32_t getNextSeqIdx();

        // Returns false if message should be discarded, true - if processed
        bool checkAcknowledgement(uint32_t ackIdx);

        // Removes acknowledged package from an output buffer
        void removeAcknowledged(uint32_t ackIdx);

        // Update current ping
        void updatePing();

        /*
         *  Returns datagram length
         */
        size_t awaitUdpDatagram(uint64_t timeoutMilliseconds, uint32_t attemptsTimeoutMilliseconds = 100);

        // No copy
        Session(const Session&);

        // No assign
        Session& operator= (const Session&);

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
        *  Is treated as implicit KeepAlive
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
         *  Send synchronization message to all players
         *  @param syncId arbitrary id of the synchronization
         *  @param delay delay time before sending synchronization message to all players [in milliseconds]
         *  @param relaible is true then make sure message will reach the server
         *  @return false if output buffer is full
         */
        MULTISLIDER_EXPORT
        bool sync(uint32_t syncId, uint64_t delay, bool relaible);

        /**
         *  Receive and handle incoming messages
         */
        MULTISLIDER_EXPORT
        uint32_t receive();

        /**
         *  Send KeepAlive message to server to prevent it from closing connection
         *  Broadcast messages are treated as KeepAlive as well.
         *  Using explicit KeepAlive is necessary only if the broadcast can't be sent enough frequently
         */
        MULTISLIDER_EXPORT
        void keepAlive();

        /**
         *  Get last ping value
         *  @return ping value in milliseconds
         */
        MULTISLIDER_EXPORT
        uint64_t getLastPing() const;

        /**
         *  Get this session name
         */
        const std::string & getSessionName() const
        {
            return mSessionName;
        }

        /**
         *  Get this player name
         */
        const std::string & getPlayerName() const
        {
            return mPlayerName;
        }

        //-------------------------------------------------------
        /**
         *  Get connection timeout 
         */
        MULTISLIDER_EXPORT
        static uint64_t getConnectionTimeout();
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

