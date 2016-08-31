/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_COMMON_INCLUDES_H_
#define _MULTI_SLIDER_COMMON_INCLUDES_H_

#include <cassert>
#include <cstdint>
#include <string>

#include "LibInterface.h"

#if defined(_MSC_VER) && (_MSC_VER < 1800)
#include <tr1/memory>
namespace multislider
{
    using std::tr1::shared_ptr;
}
#else
#include <memory>
namespace multislider
{
    using std::shared_ptr;
}
#endif

#define QUOTE_IMPL(s) #s
#define QUOTE(s) QUOTE_IMPL(s)

#if defined (_MSC_VER)
# define MULTISLIDER_DEPRECATED(Reason) __declspec(deprecated(QUOTE(Reason)))
#else
# define MULTISLIDER_DEPRECATED(Reason) __attribute__ ((deprecated))
#endif

namespace RakNet
{
    struct SystemAddress;
    class TCPInterface;
}

namespace multislider
{
    class RoomInfo;
    class Lobby;
    class Session;

    typedef shared_ptr<Session> SessionPtr;
}

#endif
