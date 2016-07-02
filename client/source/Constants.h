/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_CONSTANTS_H_
#define _MULTI_SLIDER_CONSTANTS_H_

#include <cstdint>

namespace multislider
{
    namespace constants
    {

        static const char RESPONSE_GREET[] = "HALO";
        static const char RESPONSE_SUCC[]  = "SUCC";
        static const char RESPONSE_SUCK[]  = "SUCK";

        static const char MESSAGE_KEY_CLASS[]        = "jsonClass";
        static const char MESSAGE_KEY_PLAYER_NAME[]  = "playerName";
        static const char MESSAGE_KEY_ROOM_NAME[]    = "roomName";
        static const char MESSAGE_KEY_DATA[]         = "data";
        static const char MESSAGE_KEY_ADDRESS[]      = "address";
        static const char MESSAGE_KEY_IP[]           = "ip";
        static const char MESSAGE_KEY_PORT[]         = "port";
        static const char MESSAGE_KEY_NAME[]         = "name";
        static const char MESSAGE_KEY_ID[]           = "id";
        static const char MESSAGE_KEY_SESSION_ID[]   = "sessionId";
        static const char MESSAGE_KEY_TIMESTAMP[]    = "timestamp";
        static const char MESSAGE_KEY_DELAY[]        = "delay";
        static const char MESSAGE_KEY_SYNC_ID[]      = "syncId";

        namespace frontend
        {
            static const char CREATE_ROOM[]   = "FrontendMessage$CreateRoom";
            static const char GET_ROOMS[]     = "FrontendMessage$GetRooms";
            static const char CLOSE_ROOM[]    = "FrontendMessage$CloseRoom";

            static const char JOIN_ROOM[]     = "FrontendMessage$JoinRoom";
            static const char LEAVE_ROOM[]    = "FrontendMessage$LeaveRoom";

            static const char BROADCAST[]       = "FrontendMessage$Update";
            static const char START_SESSION[]   = "FrontendMessage$StartSession";
            static const char SESSION_STARTED[] = "FrontendMessage$SessionStarted";
        }

        namespace backend
        {
            static const char ENVELOP[] = "BackendMessage$Envelop";

            // Outcoming
            static const char READY[]  = "BackendMessage$Ready";
            static const char UPDATE[] = "BackendMessage$Update";
            static const char SYNC_REQUEST[] = "BackendMessage$RequestSync";

            // Incoming
            static const char START[] = "BackendMessage$Start";
            static const char STATE[] = "BackendMessage$SessionState";
            static const char SYNC[]  = "BackendMessage$Sync";
        }

        static const uint64_t DEFAULT_TIMEOUT_MS = 10000; // 10 seconds
    }
}


#endif
