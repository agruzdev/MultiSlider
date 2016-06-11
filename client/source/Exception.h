/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_EXCEPTION_H_
#define _MULTI_SLIDER_EXCEPTION_H_

#include <stdexcept>

namespace multislider
{

    class Fail
        : public std::runtime_error
    {
    public:
        Fail(const std::string & msg) 
            : std::runtime_error(msg)
        { }

        Fail(const char* msg)
            : std::runtime_error(msg)
        { }
    };

}

#endif
