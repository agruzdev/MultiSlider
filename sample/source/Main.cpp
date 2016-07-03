
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
    void onStart(const std::string & sessionName, const std::string & playerName) override
    {
        std::cout << std::string("SessionCallback[") + playerName + "]: Started session " + sessionName + "\n";
    }

    void onUpdate(const std::string & sessionName, const std::string & playerName, const SessionData & data) override
    {
        std::string msg = std::string("SessionCallback[") + playerName + "]: Got session state (" + sessionName + ")\n";
        for (auto & entry : data) {
            msg += entry.first + " -> " + entry.second + "\n";
        }
        std::cout << msg;
    }

    void onSync(const std::string & sessionName, const std::string & playerName, uint32_t syncId) override
    {
        std::cout << std::string("SessionCallback[") + playerName + "]: Got sync " + std::to_string(syncId) + "\n";
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
        std::cout << std::string("Room \"") + room.roomName + "\" is created!\n";
    }

    void onClosed(const RoomInfo & room) override
    {
        std::cout << std::string("Room \"") + room.roomName + "\" is closed!\n";
    }

    void onSessionStart(const RoomInfo & room, SessionPtr session) override
    {
        std::cout << std::string("Room \"") + room.roomName + "\" session is started!\n";
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
            Host* host = lobby.createRoom("Player1", "Room1", 2, &callback);

            gFlagJoin = true;
            gCvJoin.notify_one();

            std::this_thread::sleep_for(std::chrono::seconds(2));

            host->broadcast("TestMessage1");
            std::cout << "Server sent broadcast\n";

            {
                std::unique_lock<std::mutex> lock(mMutex);
                gCvSession.wait(lock, []() {return gFlagSession.load(); });
            }

            std::vector<RoomInfo> rooms = lobby.getRooms();
            for (auto & info : rooms) {
                std::cout << info.roomName + " by " + info.hostName + "   players: " + std::to_string(info.playersNumber) + "/" + std::to_string(info.playersLimit) + "\n";
            }

            host->startSession();
            while (0 == host->receive()) 
            { }

            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (gHostSession != nullptr) {
                gHostSession->startup(&sessionCallback, 1000 * 60);


                std::this_thread::sleep_for(std::chrono::seconds(1));

                gHostSession->broadcast("ServerDataHere", false);

                while (0 == gHostSession->receive())
                { }

                std::this_thread::sleep_for(std::chrono::seconds(1));

                gHostSession->sync(7, 1);

                uint32_t counter = 0;
                while (counter < 2) {
                    counter += gHostSession->receive();
                }
            }

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
        std::cout << std::string("[") + playerName + "]: I joined \"" + room.roomName + "\"\n";
    }

    void onLeft(const std::string & playerName, const RoomInfo & room) override
    {
        std::cout << std::string("[") + playerName + "]: I left \"" + room.roomName + "\"\n";
    }

    void onBroadcast(const std::string & playerName, const std::string & message) override
    {
        std::cout << std::string("[") + playerName + "]: I got broadcast message \"" + message + "\"\n";
    }

    void onSessionStart(const std::string & playerName, SessionPtr session) override
    {
        std::cout << std::string("[") + playerName + "]: I got SessionStarted message!\n";
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
        SessionCallbackSample sessionCallback;
        {
            std::unique_lock<std::mutex> lock(mMutex);
            gCvJoin.wait(lock, []() {return gFlagJoin.load(); });

            Lobby lobby;
            std::vector<RoomInfo> rooms = lobby.getRooms();
            for (auto & info : rooms) {
                std::cout << info.roomName + " by " + info.hostName + "   players: " + std::to_string(info.playersNumber) + "/" + std::to_string(info.playersLimit) + "\n";
            }
            if (!rooms.empty()) {
                bool full = false;
                Client* client = lobby.joinRoom("Player2", rooms[0], &callback, full);

                while (0 == client->receive())
                { }

                gFlagSession = true;
                gCvSession.notify_one();

                while (0 == client->receive())
                { }

                std::this_thread::sleep_for(std::chrono::seconds(1));
                client->leaveRoom();
            }

            if (gClientSession != nullptr) {
                gClientSession->startup(&sessionCallback, 1000 * 60);

                gClientSession->broadcast("ClientDataHere", false);

                uint32_t counter = 0;
                while (counter < 2) {
                    counter += gClientSession->receive();
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));

                gClientSession->broadcast("ForcedData", true);
                while (0 == gClientSession->receive()) 
                { }
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
