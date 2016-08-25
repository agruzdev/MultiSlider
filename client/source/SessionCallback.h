/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_SESSION_CALLBACK_H_
#define _MULTI_SLIDER_SESSION_CALLBACK_H_

#include <map>
#include <vector>

#include "CommonIncludes.h"

namespace multislider
{
    struct PlayerData
    {
        std::string data;
        uint64_t timestamp;
    };

    typedef std::map<std::string, PlayerData> SessionData;

    class SessionCallback
    {
    public:
        virtual ~SessionCallback() { }

        /**
         *  Is called as soon as all players have joined the session and are ready to start
         */
        virtual void onStart(Session* session) { }

        /**
         *  Is called as soon as server responsed broadcast message
         */
        virtual void onUpdate(Session* session, const SessionData & /*data*/, const PlayerData & /*sharedData*/) { }

        /**
         *  Is called as soon as synchronization message is got
         *  @param wasLost set true if Sync message wasn't delivered
         */
        virtual void onSync(Session* session, uint32_t /*syncId*/, bool /*wasLost*/) { }

        /**
         *  Is called as soon as the player quit the session
         *  @param byTimeout is true is the player was disconnected by timeout
         */
        virtual void onQuit(Session* session, bool /*byTimeout*/) throw () { }
    }; 
}

#endif 

