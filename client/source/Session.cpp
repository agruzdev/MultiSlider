/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#include "Session.h"
#include "Exception.h"
#include "Utility.h"
#include "Constants.h"

#include <enet/enet.h>
#include <RakSleep.h>

#include <jsonxx.h>

using namespace RakNet;
using namespace jsonxx;


namespace multislider
{
    struct SocketImpl
    {
        ENetSocket enetSocket;
        bool valid;

        SocketImpl()
            : valid(false)
        { }
    };
}

namespace
{
    using namespace multislider;

    struct EnetSocketDeleter
    {
        void operator()(SocketImpl* ptr) const
        {
            if (ptr != NULL) {
                if (ptr->valid) {
                    enet_socket_destroy(ptr->enetSocket);
                }
            }
        }
    };


}

namespace multislider
{
    using namespace constants;

    //-------------------------------------------------------
    static const size_t RECEIVE_BIFFER_SIZE = 4096;

    bool Session::msEnetInited = false;

    void Session::destoyInstance(Session* ptr)
    {
        if (ptr != NULL) {
            delete ptr;
        }
    }
    //-------------------------------------------------------

    Session::Session(std::string ip, uint16_t port, const std::string & playerName, const std::string & sessionName, uint32_t sessionId)
        : mServerIp(ip), mServerPort(port), mPlayerName(playerName), mSessionName(sessionName), mSessionId(sessionId), mStarted(false)
    {
        assert(!mServerIp.empty());
        assert(!mPlayerName.empty());
        assert(!mSessionName.empty());
        if (!msEnetInited) {
            if (!enet_initialize()) {
                // ToDo: Inited by RaNet. Fix after getting rid of RakNet?
                //throw RuntimeError("Session[Session]: Failed to init ENet");
            }
            enet_time_set(0);
            msEnetInited = true;
        }
        mReceiveBuffer.resize(RECEIVE_BIFFER_SIZE);
    }
    //-------------------------------------------------------

    Session::~Session()
    {

    }
    //-------------------------------------------------------

    Object Session::makeEnvelop(const jsonxx::Object & obj) const
    {
        Object envelop;
        envelop << MESSAGE_KEY_CLASS << backend::ENVELOP;
        envelop << MESSAGE_KEY_SESSION_ID << mSessionId;
        envelop << MESSAGE_KEY_DATA << obj.write(JSON);
        return envelop;
    }
    //-------------------------------------------------------

    void Session::sendUpdDatagram(const std::string & message) const
    {
        ENetAddress address;
        enet_address_set_host(&address, mServerIp.c_str());
        address.port = mServerPort;

        ENetBuffer buffer;
        buffer.data = const_cast<void*>(pointer_cast<const void*>(message.c_str()));
        buffer.dataLength = message.size();
        if (!enet_socket_send(mSocket->enetSocket, &address, &buffer, 1)) {
            throw RuntimeError("Session[Startup]: Failed to send UDP datagram");
        }
    }
    //-------------------------------------------------------

    size_t Session::awaitUdpDatagram(uint64_t timeoutMilliseconds, uint32_t attemptsTimeoutMilliseconds /* = 100 */) 
    {
        ENetAddress address;
        ENetBuffer buffer;
        buffer.data = &mReceiveBuffer[0];
        buffer.dataLength = mReceiveBuffer.size();
        uint64_t time = 0;
        int len = 0;
        while ((time < timeoutMilliseconds) && (0 == (len = enet_socket_receive(mSocket->enetSocket, &address, &buffer, 1)))) {
            RakSleep(attemptsTimeoutMilliseconds);
            time += attemptsTimeoutMilliseconds;
        }
        if (len < 0) {
            return 0;
        }
        else {
            return static_cast<size_t>(len);
        }
    }
    //-------------------------------------------------------

    void Session::startup(SessionCallback* callback, uint64_t timeout)
    {
        if (callback == NULL) {
            throw RuntimeError("Session[Startup]: callback can't be null!");
        }
        mSocket = shared_ptr<SocketImpl>(new SocketImpl);
        mSocket->enetSocket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
        mSocket->valid = true;
        if (0 != enet_socket_set_option(mSocket->enetSocket, ENET_SOCKOPT_NONBLOCK, 1)) {
            throw RuntimeError("Session[Session]: Failed to setup socket");
        }
        mCallback = callback;

        // Now ready
        Object readyJson;
        readyJson << MESSAGE_KEY_CLASS << backend::READY;
        readyJson << MESSAGE_KEY_PLAYER_NAME << mPlayerName;
        const std::string readyMessage = makeEnvelop(readyJson).write(JSON);
        sendUpdDatagram(readyMessage);

        uint64_t time = 0;
        size_t dataLength = 0;
        while (0 == (dataLength = awaitUdpDatagram(DEFAULT_TIMEOUT_MS))) {
            time += DEFAULT_TIMEOUT_MS;
            if (time > timeout) {
                throw RuntimeError("Session[Startup]: Failed to get response from server");
            }
            // Resend
            sendUpdDatagram(readyMessage);
        }
        if(responsed(&mReceiveBuffer[0], dataLength, RESPONSE_SUCC)) {
            while (0 == (dataLength = awaitUdpDatagram(DEFAULT_TIMEOUT_MS))) {
                time += DEFAULT_TIMEOUT_MS;
                if (time > timeout) {
                    throw RuntimeError("Session[Startup]: Failed to get response from server");
                }
                // Resend
                sendUpdDatagram(readyMessage);
            }
            Object messageJson;
            messageJson.parse(std::string(pointer_cast<char*>(&mReceiveBuffer[0]), dataLength));
            std::string messageClass(messageJson.get<std::string>(MESSAGE_KEY_CLASS, ""));
            if (!messageClass.empty()) {
                if (isMessageClass(messageClass, backend::START)) {
                    mCallback->onStart(mSessionName, mPlayerName);
                    mStarted = true;
                    return;
                }
            }
            throw RuntimeError("Session[Startup]: Unexpected server response!");
        }

        Object messageJson;
        messageJson.parse(std::string(pointer_cast<char*>(&mReceiveBuffer[0]), dataLength));
        std::string messageClass(messageJson.get<std::string>(MESSAGE_KEY_CLASS, ""));
        if (!messageClass.empty()) {
            if (isMessageClass(messageClass, backend::START)) {
                mCallback->onStart(mSessionName, mPlayerName);
                mStarted = true;
                return;
            }
        }
        //else 
        throw RuntimeError("Session[Startup]: Unexpected server response!");
    }
    //-------------------------------------------------------

    void Session::broadcast(const std::string & data)
    {
        if (!mStarted) {
            throw ProtocolError("Session[broadcast]: Session was not started!");
        }
        Object updateJson;
        updateJson << MESSAGE_KEY_CLASS << backend::UPDATE;
        updateJson << MESSAGE_KEY_PLAYER_NAME << mPlayerName;
        updateJson << MESSAGE_KEY_TIMESTAMP << enet_time_get();
        updateJson << MESSAGE_KEY_DATA << data;
        sendUpdDatagram(makeEnvelop(updateJson).write(JSON));
    }
    //-------------------------------------------------------

    uint32_t Session::receive()
    {
        uint32_t counter = 0;
        size_t dataLength = 0; 
        while((dataLength = awaitUdpDatagram(1)) > 0) {
            ++counter;
            Object messageJson;
            messageJson.parse(std::string(pointer_cast<char*>(&mReceiveBuffer[0]), dataLength));
            std::string messageClass(messageJson.get<std::string>(MESSAGE_KEY_CLASS));
            if (isMessageClass(messageClass, backend::START)) {
                mCallback->onStart(mSessionName, mPlayerName);
            }
            else if (isMessageClass(messageClass, backend::STATE)) {
                Array sessionDataJson;
                std::string dataJson = messageJson.get<std::string>(MESSAGE_KEY_DATA);
                sessionDataJson.parse(dataJson);
                SessionData sessionData;
                for (size_t i = 0; i < sessionDataJson.size(); ++i) {
                    Object entry = sessionDataJson.get<Object>(i);
                    sessionData[entry.get<std::string>(MESSAGE_KEY_NAME)] = entry.get<std::string>(MESSAGE_KEY_DATA);
                }
                mCallback->onUpdate(mSessionName, mPlayerName, sessionData);
            }
            else {
                throw RuntimeError("Session[receive]: Unknown datagram type!");
            }
        }
        return counter;
    }
}



