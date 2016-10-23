/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_UDP_INTERFACE_H_
#define _MULTI_SLIDER_UDP_INTERFACE_H_

#include <vector>

#include <RakSleep.h>

#include <boost/asio.hpp>

#include "CommonIncludes.h"
#include "Utility.h"

namespace multislider
{
    /**
     *  Singleton wrapper for ENet library
     */
    class UdpInterface
    {
    public:
        const static size_t MAX_BUFFER_SIZE;

    private:
        boost::asio::io_service mIoService;
        boost::asio::ip::udp::socket mAsioSocket;
        boost::asio::ip::udp::endpoint mEndpoint;

        boost::asio::streambuf mStreamBuffer;
        std::istream mReceiveStream;

        UdpInterface(const UdpInterface &);
        UdpInterface & operator= (const UdpInterface &);

    public:
        UdpInterface(const std::string & ip, uint16_t port)
            : mIoService(), mAsioSocket(mIoService), mStreamBuffer(MAX_BUFFER_SIZE), mReceiveStream(&mStreamBuffer)
        {
            boost::asio::ip::udp::resolver resolver(mIoService);
            boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), ip, to_string(port));
            mEndpoint = *resolver.resolve(query);
            mAsioSocket.open(boost::asio::ip::udp::v4());
        }

        bool sendUpdDatagram(const std::string & message)
        {
            boost::system::error_code err;
            size_t len = mAsioSocket.send_to(boost::asio::buffer(message), mEndpoint, 0, err);
            return !err && (len == message.size());
        }

        std::string awaitUdpDatagram(uint64_t timeoutMilliseconds, uint32_t attemptsTimeoutMilliseconds = 100)
        {
            std::string message;
            uint64_t time = 0;
            while (time <= timeoutMilliseconds) {
                size_t available = mAsioSocket.available();
                if (available > 0) {
                    boost::asio::ip::udp::endpoint sender;
                    size_t len = mAsioSocket.receive_from(mStreamBuffer.prepare(available), sender);
                    if (sender == mEndpoint) {
                        mStreamBuffer.commit(len);
                        message = std::string(std::istreambuf_iterator<char>(mReceiveStream), std::istreambuf_iterator<char>());
                        break;
                    }
                    else {
                        mStreamBuffer.consume(len);
                    }
                }
                RakSleep(attemptsTimeoutMilliseconds);
                time += attemptsTimeoutMilliseconds;
            }
            return message;
        }
    };

}

#endif
