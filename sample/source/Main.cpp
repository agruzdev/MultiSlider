
#include "../../client/source/Lobby.h"
#include "../../client/source/Exception.h"

#include <iostream>
#include <mutex>
#include <future>
#include <atomic>
#include <condition_variable>

using namespace multislider;

std::atomic_bool gFlagJoin  { false };
std::atomic_bool gFlagFinish{ false };
std::atomic_bool gFlagSession{ false };

std::condition_variable gCvJoin;
std::condition_variable gCvFinish;
std::condition_variable gCvSession;

class SessionCallbackSample
    : public SessionCallback
{
public:
    void onStart()
    {
        std::cout << "SessionCallback: onStart()" << std::endl;
    }
};

SessionPtr gHostSession;
SessionPtr gClientSession;

class HostCallbackSample
    : public HostCallback
{
public:
    void onCreated(const RoomInfo & room) override
    {
        std::cout << "Room \"" << room.roomName << "\" is created!" << std::endl;
    }

    void onClosed(const RoomInfo & room) override
    {
        std::cout << "Room \"" << room.roomName << "\" is closed!" << std::endl;
    }

    void onSessionStart(const RoomInfo & room, SessionPtr session) override
    {
        std::cout << "Room \"" << room.roomName << "\" session is started!" << std::endl;
        gHostSession = session;
    }
};


class HostSample
{
    std::mutex mMutex;
public:
    void run()
    {
        HostCallbackSample callback;
        SessionCallbackSample sessionCallback;
        {
            Lobby lobby;
            Host* host = lobby.createRoom("Player1", "Room1", &callback);

            gFlagJoin = true;
            gCvJoin.notify_one();

            std::this_thread::sleep_for(std::chrono::seconds(2));

            host->broadcast("TestMessage1");
            std::cout << "Server sent broadcast" << std::endl;

            {
                std::unique_lock<std::mutex> lock(mMutex);
                gCvSession.wait(lock, []() {return gFlagSession.load(); });
            }

            host->startSession();
            while (0 == host->receive()) 
            { }

            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (gHostSession != nullptr) {
                gHostSession->Startup(&sessionCallback);
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));

            {
                std::unique_lock<std::mutex> lock(mMutex);
                gCvFinish.wait(lock, []() {return gFlagFinish.load(); });
            }
            host->closeRoom();
        }
    }
};


class ClientCallbackSample
    : public ClientCallback
{
public:

    void onJoined(const std::string & playerName, const RoomInfo & room) override
    {
        std::cout << "[" << playerName << "]: I joined \"" << room.roomName << "\"" << std::endl;
    }

    void onLeft(const std::string & playerName, const RoomInfo & room) override
    {
        std::cout << "[" << playerName << "]: I left \"" << room.roomName << "\"" << std::endl;
    }

    void onBroadcast(const std::string & playerName, const std::string & message) override
    {
        std::cout << "[" << playerName << "]: I got broadcast message \"" << message << "\"" << std::endl;
    }

    void onSessionStart(const std::string & playerName, SessionPtr session) override
    {
        std::cout << "[" << playerName << "]: I got SessionStarted message!" << std::endl;
        gClientSession = session;
    }
};


class ClientSample
{
    std::mutex mMutex;
public:
    void run()
    {
        ClientCallbackSample callback;
        {
            std::unique_lock<std::mutex> lock(mMutex);
            gCvJoin.wait(lock, []() {return gFlagJoin.load(); });

            Lobby lobby;
            std::vector<RoomInfo> rooms = lobby.getRooms();
            for (auto & info : rooms) {
                std::cout << info.roomName << " by " << info.hostName << std::endl;
            }
            if (!rooms.empty()) {
                Client* client = lobby.joinRoom("Player2", rooms[0], &callback);

                while (0 == client->receive())
                { }

                gFlagSession = true;
                gCvSession.notify_one();

                while (0 == client->receive()) 
                { }

                std::this_thread::sleep_for(std::chrono::seconds(1));
                client->leaveRoom();
            }
            gFlagFinish = true;
            gCvFinish.notify_one();
        }
    }
};

int main()
{
    auto host   = std::async(std::launch::async, []() { 
        try {
            HostSample().run();
        }
        catch (RuntimeError & e) {
            std::cout << e.what() << std::endl;
        }
    });
    auto client = std::async(std::launch::async, []() { 
        try {
            ClientSample().run();
        }
        catch (RuntimeError & e) {
            std::cout << e.what() << std::endl;
        }
    });

    client.wait();
    host.wait();

    return 0;
}
