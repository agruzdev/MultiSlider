
#include "../../../client/source/MultiSlider.h"

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

static const char SERVER_ADDRESS[] = "127.0.0.1";
static const uint16_t SERVER_FRONTEND_PORT = 8800;
static const uint16_t SERVER_BACKEND_PORT  = 8700;

class SessionCallbackSample
    : public SessionCallback
{
public:
    void onStart(const std::string & sessionName, const std::string & playerName) override
    {
        std::cout << std::string("SessionCallback[") + playerName + "]: Started session " + sessionName + "\n";
    }

    void onUpdate(const std::string & sessionName, const std::string & playerName, const SessionData & data, const std::string & sharedData) override
    {
        std::string msg = std::string("SessionCallback[") + playerName + "]: Got session state (" + sessionName + ")\n";
        for (auto & entry : data) {
            msg += entry.first + " -> " + entry.second + "\n";
        }
        msg += std::string("_shared_ -> ") + sharedData + "\n";
        std::cout << msg;
    }

    void onSync(const std::string & sessionName, const std::string & playerName, uint32_t syncId) override
    {
        std::cout << std::string("SessionCallback[") + playerName + "]: Got sync " + std::to_string(syncId) + "\n";
    }
};

SessionPtr gHostSession;
SessionPtr gClientSession;

class CallbackSample
    : public Lobby::Callback
{
public:
    void onJoined(Lobby* lobby, const RoomInfo & room, const std::string & playerName) override
    {
        std::cout << std::string("[") + playerName + "]: I joined the room \"" + room.getName() + "\"!\n";
    }

    void onLeft(Lobby* lobby, const RoomInfo & room, const std::string & playerName, uint8_t flags) override
    {
        std::cout << std::string("[") + playerName + "]: I left the room \"" + room.getName() + "\"!\n";
    }

    void onBroadcast(Lobby* lobby, const RoomInfo & room, const std::string & playerName, const std::string & message) override
    {
        std::cout << std::string("[") + playerName + "]: got broadcast message \"" + message + "\"\n";
    }

    void onSessionStart(Lobby* lobby, const RoomInfo & room, const std::string & playerName, SessionPtr session) override
    {
        std::cout << std::string("[") + playerName + "]: session is started!\n";
        if (playerName == room.getHostName()) {
            gHostSession = session;
        }
        else {
            gClientSession = session;
        }
    }
};


class HostSample
{
    std::mutex mMutex;
public:
    void run()
    {
        CallbackSample callback;
        SessionCallbackSample sessionCallback;
        {
            Lobby lobby(SERVER_ADDRESS, SERVER_FRONTEND_PORT);
            //Host* host = lobby.createRoom("Player1", "Room1", 2, &callback);
            if (Lobby::SUCCESS != lobby.createRoom("Player1", "Room1", 2, &callback)) {
                throw std::runtime_error("Failed to create a room!");
            }

            gFlagJoin = true;
            gCvJoin.notify_one();

            while (0 == lobby.receive()) 
            { }
            std::this_thread::sleep_for(std::chrono::seconds(2));

            lobby.broadcast("TestMessage1", false);
            std::cout << "Server sent broadcast\n";

            {
                std::unique_lock<std::mutex> lock(mMutex);
                gCvSession.wait(lock, []() {return gFlagSession.load(); });
            }

            std::vector<RoomInfo> rooms = lobby.getRooms();
            for (auto & info : rooms) {
                std::cout << info.getName() + " by " + info.getHostName() + "   players: " + std::to_string(info.getPlayersNumber()) + "/" + std::to_string(info.getPlayersLimit()) + "\n";
            }

            lobby.startSession();
            while (0 == lobby.receive()) 
            { }

            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (gHostSession != nullptr) {
                gHostSession->startup(&sessionCallback, 1000 * 60);


                std::this_thread::sleep_for(std::chrono::seconds(1));

                gHostSession->broadcast("ServerDataHere", "OverwrittenShared", false);

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
            lobby.closeRoom();
        }
    }
};

class ClientSample
{
    std::mutex mMutex;
public:
    void run()
    {
        CallbackSample callback;
        SessionCallbackSample sessionCallback;
        {
            std::unique_lock<std::mutex> lock(mMutex);
            gCvJoin.wait(lock, []() {return gFlagJoin.load(); });

            Lobby lobby(SERVER_ADDRESS, SERVER_FRONTEND_PORT);
            std::vector<RoomInfo> rooms = lobby.getRooms();
            for (auto & info : rooms) {
                std::cout << info.getName() + " by " + info.getHostName() + "   players: " + std::to_string(info.getPlayersNumber()) + "/" + std::to_string(info.getPlayersLimit()) + "\n";
            }
            if (!rooms.empty()) {
                if (Lobby::SUCCESS != lobby.joinRoom("Player2", rooms[0], &callback)) {
                    throw std::runtime_error("Failed to join the room!");
                }

                while (0 == lobby.receive())
                { }

                gFlagSession = true;
                gCvSession.notify_one();

                while (0 == lobby.receive())
                { }

                std::this_thread::sleep_for(std::chrono::seconds(1));
                lobby.leaveRoom();
            }

            if (gClientSession != nullptr) {
                gClientSession->startup(&sessionCallback, 1000 * 60);

                gClientSession->broadcast("ClientDataHere", "SharedHere", false);

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
