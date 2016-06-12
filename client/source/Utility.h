/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_UTILITY_H_
#define _MULTI_SLIDER_UTILITY_H_

#include "CommonIncludes.h"
#include "TCPInterface.h"
#include "RakSleep.h"

namespace multislider
{
    /**
     *  Blocking receive 
     *  http://www.raknet.com/forum/index.php?topic=587.0
     */
    template <typename _ConnectionInterfacePtr>
    inline
    RakNet::Packet* awaitResponse(const _ConnectionInterfacePtr & connection, uint64_t timeoutMilliseconds, uint32_t attemptsTimeoutMilliseconds = 100)
    {
        RakNet::Packet* packet(NULL);
        uint64_t time = 0;
        for (;;) {
            packet = connection->Receive();
            if (packet != NULL || time > timeoutMilliseconds) {
                break;
            }
            RakSleep(attemptsTimeoutMilliseconds);
            time += attemptsTimeoutMilliseconds;
        }
        return packet;
    }

    /**
     *  Safe cast for pointers
     */
    template <typename _TypeTo, typename _TypeFrom>
    inline 
    _TypeTo castPointerTo(_TypeFrom* ptr)
    {
        return static_cast<_TypeTo>(static_cast<void*>(ptr));
    }

    /**
     *  Safe cast for pointers
     */
    template <typename _TypeTo, typename _TypeFrom>
    inline 
    _TypeTo castPointerTo(const _TypeFrom* ptr)
    {
        return static_cast<_TypeTo>(static_cast<const void*>(ptr));
    }


    /**
     *  Compare default server response 
     *  @param bytes response bytes, should be at least 4 bytes
     *  @param expected expected response, should be exactly 4 bytes
     */
    inline 
    bool responsed(const RakNet::Packet* packet, const char* expected)
    {   
        assert(std::strlen(expected) == 4);
        return (packet->length == 4) && 
            (*castPointerTo<const uint32_t*>(packet->data) == *castPointerTo<const uint32_t*>(expected));
    }
}


#endif
