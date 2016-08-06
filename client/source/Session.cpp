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

#include "UdpInterface.h"

#include <jsonxx.h>
#include <GetTime.h> // From RakNet

using namespace RakNet;
using namespace jsonxx;

namespace multislider
{
    using namespace constants;

    struct MsgInfo
    {
        uint32_t seqIdx;
        uint64_t sentTime;
        std::string msgData;

        MsgInfo(uint32_t seqIdx, uint64_t sentTime, const std::string & msgData)
            : seqIdx(seqIdx), sentTime(sentTime), msgData(msgData)
        { }
    };


    //-------------------------------------------------------
    static const size_t RECEIVE_BIFFER_SIZE = 4096;
    static const size_t MESSSAGES_BUFFER_SIZE = 32;
    static const size_t IDX_WRAP_MODULO = 1024;

    //bool Session::msEnetInited = false;

    void Session::destoyInstance(Session* ptr)
    {
        if (ptr != NULL) {
            delete ptr;
        }
    }
    //-------------------------------------------------------

    Session::Session(std::string ip, uint16_t port, const std::string & playerName, const std::string & sessionName, uint32_t sessionId)
        : mServerIp(ip), mServerPort(port), mPlayerName(playerName), mSessionName(sessionName), mSessionId(sessionId), mLastPing(0), mStarted(false),
          mLocalSeqIdx(1), mRemoteSeqIdx(0), mPreviousIdxBits(0)
    {
        assert(!mServerIp.empty());
        assert(!mPlayerName.empty());
        assert(!mSessionName.empty());
        mReceiveBuffer.resize(RECEIVE_BIFFER_SIZE);
    }
    //-------------------------------------------------------

    Session::~Session()
    {
        if (mStarted) {
            Object quitJson;
            quitJson << MESSAGE_KEY_CLASS << backend::QUIT;
            quitJson << MESSAGE_KEY_PLAYER_NAME << mPlayerName;
            sendUpdDatagram(makeEnvelop(quitJson).write(JSON));
            //mCallback->onQuit(mSessionName, mPlayerName, false);
        }
    }
    //-------------------------------------------------------

    uint32_t Session::getNextSeqIdx()
    {
        const uint32_t idx = mLocalSeqIdx;
        mLocalSeqIdx = (mLocalSeqIdx < IDX_WRAP_MODULO - 1) ? mLocalSeqIdx + 1 : 0;
        return idx;
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
        if (!UdpInterface::Instance().sendUpdDatagram(*mSocket, mServerIp, mServerPort, message)) {
            throw RuntimeError("Session[Startup]: Failed to send UDP datagram");
        }
    }
    //-------------------------------------------------------

    size_t Session::awaitUdpDatagram(uint64_t timeoutMilliseconds, uint32_t attemptsTimeoutMilliseconds /* = 100 */) 
    {
        return UdpInterface::Instance().awaitUdpDatagram(*mSocket, mReceiveBuffer, timeoutMilliseconds, attemptsTimeoutMilliseconds);
    }
    //-------------------------------------------------------

    void Session::startup(SessionCallback* callback, uint64_t timeout)
    {
        if (callback == NULL) {
            throw RuntimeError("Session[Startup]: callback can't be null!");
        }
        mSocket = shared_ptr<UdpSocket>(new UdpSocket);
        if (0 != enet_socket_set_option(*mSocket, ENET_SOCKOPT_NONBLOCK, 1)) {
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
                    mStarted = true;
                    mCallback->onStart(mSessionName, mPlayerName);
                    mLastPing = RakNet::GetTime();
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
                mStarted = true;
                mCallback->onStart(mSessionName, mPlayerName);
                mLastPing = RakNet::GetTime();
                return;
            }
        }
        //else 
        throw RuntimeError("Session[Startup]: Unexpected server response!");
    }
    //-------------------------------------------------------

    void Session::broadcast(const std::string & data, const std::string & sharedData, bool forced)
    {
        if (!mStarted) {
            throw ProtocolError("Session[broadcast]: Session was not started!");
        }
        Object updateJson;
        updateJson << MESSAGE_KEY_CLASS << backend::UPDATE;
        updateJson << MESSAGE_KEY_PLAYER_NAME << mPlayerName;
        updateJson << MESSAGE_KEY_TIMESTAMP << enet_time_get();
        updateJson << MESSAGE_KEY_FORCE_BROADCAST << forced;
        updateJson << MESSAGE_KEY_DATA << data;
        updateJson << MESSAGE_KEY_SHARED_DATA << sharedData;
        sendUpdDatagram(makeEnvelop(updateJson).write(JSON));
    }
    //-------------------------------------------------------

    void Session::broadcast(const std::string & data, bool forced)
    {
        broadcast(data, std::string(), forced);
    }
    //-------------------------------------------------------

    bool Session::sync(uint32_t syncId, uint64_t delay, bool reliable)
    {
        if (!mStarted) {
            throw ProtocolError("Session[broadcast]: Session was not started!");
        }
        Object syncJson;
        syncJson << MESSAGE_KEY_CLASS << backend::SYNC_REQUEST;
        syncJson << MESSAGE_KEY_PLAYER_NAME << mPlayerName;
        syncJson << MESSAGE_KEY_DELAY << delay;
        syncJson << MESSAGE_KEY_SYNC_ID << syncId;
        if (reliable) {
            if (mOutputQueue.size() >= MESSSAGES_BUFFER_SIZE) {
                return false;
            }
            uint32_t seqIdx = getNextSeqIdx();
            syncJson << MESSAGE_KEY_SEQ_INDEX << seqIdx;
            std::string syncMessage = makeEnvelop(syncJson).write(JSON);
            mOutputQueue.push_back(shared_ptr<MsgInfo>(new MsgInfo(seqIdx, RakNet::GetTimeMS(), syncMessage)));
            sendUpdDatagram(syncMessage);
        }
        else {
            syncJson << MESSAGE_KEY_SEQ_INDEX << (IDX_WRAP_MODULO + 1);
            sendUpdDatagram(makeEnvelop(syncJson).write(JSON));
        }
        return true;
    }
    //-------------------------------------------------------

    void Session::sync(uint32_t syncId, uint64_t delay)
    {
        sync(syncId, delay, false);
    }
    //-------------------------------------------------------

    void Session::keepAlive()
    {
        if (!mStarted) {
            throw ProtocolError("Session[keepAlive]: Session was not started!");
        }
        Object keepAliveJson;
        keepAliveJson << MESSAGE_KEY_CLASS << backend::KEEP_ALIVE;
        keepAliveJson << MESSAGE_KEY_PLAYER_NAME << mPlayerName;
        sendUpdDatagram(makeEnvelop(keepAliveJson).write(JSON));
    }
    //-------------------------------------------------------

    uint32_t Session::receive()
    {
        uint32_t counter = 0;
        size_t dataLength = 0; 
        while(mStarted && (dataLength = awaitUdpDatagram(1, 1)) > 0) {
            ++counter;
            Object messageJson;
            messageJson.parse(std::string(pointer_cast<char*>(&mReceiveBuffer[0]), dataLength));
            std::string messageClass(messageJson.get<std::string>(MESSAGE_KEY_CLASS, ""));
            if (isMessageClass(messageClass, backend::START)) {
                mCallback->onStart(mSessionName, mPlayerName);
            }
            else if (isMessageClass(messageClass, backend::STATE)) {
                Array sessionDataJson;
                std::string dataJson = messageJson.get<std::string>(MESSAGE_KEY_DATA);
                sessionDataJson.parse(dataJson);
                SessionData sessionData;
                std::string shared;
                for (size_t i = 0; i < sessionDataJson.size(); ++i) {
                    Object entry = sessionDataJson.get<Object>(i);
                    sessionData[entry.get<std::string>(MESSAGE_KEY_NAME)] = entry.get<std::string>(MESSAGE_KEY_DATA);
                }
                mCallback->onUpdate(mSessionName, mPlayerName, sessionData, messageJson.get<jsonxx::String>(MESSAGE_KEY_SHARED_DATA, ""));
            }
            else if (isMessageClass(messageClass, backend::SYNC)) {
                const uint32_t seqIdx = narrow_cast<uint32_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_SEQ_INDEX, 0));
                if (seqIdx >= IDX_WRAP_MODULO || checkAcknowledgement(seqIdx)) {
                    mCallback->onSync(mSessionName, mPlayerName, narrow_cast<uint32_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_SYNC_ID)), false);
                }
            }
            else if (isMessageClass(messageClass, backend::KEEP_ALIVE)) {
                mLastPing = RakNet::GetTimeMS();
                --counter; // KeepAlive messages are 'invisible'
            }
            else if (isMessageClass(messageClass, backend::ACK)) {
                const uint32_t ackIdx = narrow_cast<uint32_t>(messageJson.get<jsonxx::Number>(MESSAGE_KEY_SEQ_INDEX, 0));
                checkAcknowledgement(ackIdx);
            }
            else {
                throw RuntimeError("Session[receive]: Unknown datagram type!");
            }
        }
        if (mStarted) {
            // Check timeout
            if ((RakNet::GetTimeMS() - mLastPing) > KEEP_ALIVE_LIMIT) {
                mCallback->onQuit(mSessionName, mPlayerName, true);
                mStarted = false;
            }
        }
        if (mStarted) {
            // Resend messages if necessary
            const uint64_t currentTime = RakNet::GetTimeMS();
            for (size_t idx = 0; idx < mOutputQueue.size(); ++idx) {
                shared_ptr<MsgInfo>& msg = mOutputQueue[idx];
                if (currentTime - msg->sentTime > RESEND_INTERVAL) {
                    sendUpdDatagram(msg->msgData);
                    msg->sentTime = currentTime;
                }
            }
        }
        return counter;
    }
    //-------------------------------------------------------

    bool Session::checkAcknowledgement(uint32_t ackIdx)
    {
        // Same message like before
        if (ackIdx == mRemoteSeqIdx) {
            return false;
        }
        // Ack is before previous message
        if ((ackIdx > mRemoteSeqIdx) && (ackIdx - mRemoteSeqIdx < MESSSAGES_BUFFER_SIZE)) {
            removeAcknowledged(ackIdx);
            mPreviousIdxBits <<= (ackIdx - mRemoteSeqIdx);
            mRemoteSeqIdx = ackIdx;
            return true;
        }
        // Ack is before previous message as well but wrapped by modulo 
        if ((IDX_WRAP_MODULO + ackIdx > mRemoteSeqIdx) && (IDX_WRAP_MODULO + ackIdx - mRemoteSeqIdx) < MESSSAGES_BUFFER_SIZE) {
            removeAcknowledged(ackIdx);
            mPreviousIdxBits <<= (IDX_WRAP_MODULO + ackIdx - mRemoteSeqIdx);
            mRemoteSeqIdx = ackIdx;
            return true;
        }
        // Ack is before previous message
        if ((ackIdx > mRemoteSeqIdx) && ackIdx - mRemoteSeqIdx < MESSSAGES_BUFFER_SIZE) {
            const uint32_t ackBit = 0x1 << (ackIdx - mRemoteSeqIdx - 1);
            if (0 == (mPreviousIdxBits & ackBit)) {
                // First time message
                removeAcknowledged(ackIdx);
                mPreviousIdxBits = mPreviousIdxBits | ackBit;
                return true;
            }
        }
        return false;
    }

    void Session::removeAcknowledged(uint32_t ackIdx) {
        size_t idx = 0;
        for (; idx < mOutputQueue.size(); ++idx) {
            if (mOutputQueue[idx]->seqIdx == ackIdx) {
                break;
            }
        }
        if (idx < mOutputQueue.size()) {
            mOutputQueue.erase(mOutputQueue.begin() + idx);
        }
    }
    //-------------------------------------------------------

    uint64_t Session::getConnectionTimeout()
    {
        return KEEP_ALIVE_LIMIT;
    }
}



