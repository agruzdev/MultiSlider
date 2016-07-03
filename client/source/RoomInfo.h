/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_ROOM_INFO_H_
#define _MULTI_SLIDER_ROOM_INFO_H_

#include <string>

namespace multislider
{
    struct RoomInfo
    {
        std::string hostName;
        std::string roomName;
        uint32_t playersLimit;
        uint32_t playersNumber;
    };
}

#endif 

