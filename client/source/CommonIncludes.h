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

namespace RakNet
{
    class TCPInterface;
}

#endif