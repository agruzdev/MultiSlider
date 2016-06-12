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

        static const char MESSAGE_CLASS_CREATE_ROOM[] = "FrontendMessage$CreateRoom";

        static const char MESSAGE_KEY_CLASS[]        = "jsonClass";
        static const char MESSAGE_KEY_PLAYER_NAME[]  = "playerName";
        static const char MESSAGE_KEY_ROOM_NAME[]    = "roomName";

        static const uint64_t DEFAULT_TIMEOUT_MS = 10000; // 10 seconds
    }
}


#endif
