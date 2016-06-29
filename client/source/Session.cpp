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

#include <RakPeerInterface.h>
#include <RakNetTypes.h>
#include <BitStream.h>

#include <jsonxx.h>

using namespace RakNet;
using namespace jsonxx;

namespace
{
    struct PeerDeleter
    {
        void operator()(RakPeerInterface* ptr)
        {
            RakPeerInterface::DestroyInstance(ptr);
        }
    };
}

namespace multislider
{
    using namespace constants;

    void Session::DestoyInstance(Session* ptr)
    {
        if (ptr != nullptr) {
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
        mSocket = shared_ptr<SocketDescriptor>(new SocketDescriptor(8800, 0));
        //std::strncpy(mSocket->hostAddress, ip.c_str(), arrayLengh(mSocket->hostAddress));
        //mSocket->port = port;
        mPeer = shared_ptr<RakPeerInterface>(RakPeerInterface::GetInstance(), PeerDeleter());
    }
    //-------------------------------------------------------

    Session::~Session()
    {
        if (mStarted) {
            mPeer->Shutdown(0);
        }
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

    void Session::Startup(SessionCallback* callback)
    {
        if (callback == NULL) {
            throw RuntimeError("Session[Startup]: callback can't be null!");
        }
        if (RAKNET_STARTED != mPeer->Startup(8, mSocket.get(), 1)) {
            throw RuntimeError("Session[Startup]: failed to start RakPeer!");
        }
        mStarted = true;
        Object readyJson;
        readyJson << MESSAGE_KEY_CLASS << backend::READY;
        readyJson << MESSAGE_KEY_PLAYER_NAME << mPlayerName;
        std::string readyMessage = makeEnvelop(readyJson).write(JSON);

        DataStructures::List<RakNetSocket2*> sockets;
        mPeer->GetSockets(sockets);
        RakNetSocket2* socket = sockets.Get(0);
        RNS2_SendParameters message;
        message.data = pointer_cast<char*>(&readyMessage[0]);
        message.length = readyMessage.size();
        message.systemAddress.FromStringExplicitPort(mServerIp.c_str(), mServerPort);
        socket->Send(&message, __FILE__, __LINE__);
    }
    //-------------------------------------------------------

    uint32_t Session::receive()
    {
        return 0;
    }
}



