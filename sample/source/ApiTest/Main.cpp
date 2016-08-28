
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
//static const char SERVER_ADDRESS[] = "95.213.199.37";
static const uint16_t SERVER_FRONTEND_PORT = 8800;
static const uint16_t SERVER_BACKEND_PORT  = 8700;

class SessionCallbackSample
    : public SessionCallback
{
public:
    void onStart(Session* session) override
    {
        std::cout << std::string("SessionCallback[") + session->getPlayerName() + "]: Started session " + session->getSessionName() + "\n";
    }

    void onUpdate(Session* session, const SessionData & data, const PlayerData & sharedData) override
    {
        std::string msg = std::string("SessionCallback[") + session->getPlayerName() + "]: Got session state (" + session->getSessionName() + ")\n";
        for (auto & entry : data) {
            if (entry.second.alive) {
                msg += entry.first + " -> " + entry.second.data + " at time " + std::to_string(entry.second.timestamp) + "\n";
            }
            else {
                msg += entry.first + " -> disconnected \n";
            }
        }
        msg += std::string("_shared_ -> ") + sharedData.data + " at time " + std::to_string(sharedData.timestamp) + "\n";
        std::cout << msg;
    }

    void onSync(Session* session, uint32_t syncId, bool wasLost) override
    {
        if (!wasLost) {
            std::cout << std::string("SessionCallback[") + session->getPlayerName() + "]: Got sync " + std::to_string(syncId) + "\n";
        }
        else {
            std::cout << std::string("SessionCallback[") + session->getPlayerName() + "]: Sync " + std::to_string(syncId) + " was lost\n";
        }
    }

    void onQuit(Session* session, const std::string & playerName, bool byTimeout) throw() override
    {
        std::cout << std::string("SessionCallback[") + session->getPlayerName() + "]: Player " + playerName + " has quited the session " + session->getSessionName() + " (by timeout =" + std::to_string(byTimeout) + ")\n";
    }
};

SessionPtr gHostSession;
SessionPtr gClientSession;

class CallbackSample
    : public Lobby::Callback
{
public:
    void onJoined(Lobby* lobby, const RoomInfo & room) override
    {
        std::cout << std::string("[") + lobby->getPlayerName() + "]: I joined the room \"" + room.getName() + "\"!\n";
    }

    void onLeft(Lobby* lobby, const RoomInfo & room, uint8_t flags) override
    {
        std::cout << std::string("[") + lobby->getPlayerName() + "]: I left the room \"" + room.getName() + "\"!\n";
    }

    void onBroadcast(Lobby* lobby, const RoomInfo & room, const std::string & sender, const std::string & message, uint8_t flags) override
    {
        std::cout << std::string("[") + lobby->getPlayerName() + "]: got broadcast message \"" + message + "\" [from \"" + sender + "\" flags = " + std::to_string(flags) + "]\n";
    }

    void onSessionStart(Lobby* lobby, const RoomInfo & room, SessionPtr session, const std::string & sessionData) override
    {
        std::cout << std::string("[") + lobby->getPlayerName() + "]: session is started! " + sessionData + "\n";
        if (lobby->getPlayerName() == room.getHostName()) {
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
            if (Lobby::SUCCESS != lobby.createRoom("Player1", "Room1", "TestRoom", 2, 0, &callback)) {
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

            lobby.startSession("<initial data here>");
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

                gHostSession->sync(7, 1, true);

                uint32_t counter = 0;
                while (counter < 2) {
                    counter += gHostSession->receive();
                }
                gHostSession.reset();
            }

            {
                std::unique_lock<std::mutex> lock(mMutex);
                gCvFinish.wait(lock, []() {return gFlagFinish.load(); });
            }
            lobby.closeRoom();

            std::this_thread::sleep_for(std::chrono::seconds(1));
            lobby.receive();
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

                std::cout << "ping = " + std::to_string(gClientSession->getLastPing()) + "\n";

                gClientSession->broadcast("ForcedData", true);
                while (0 == gClientSession->receive()) 
                { }

                std::cout << "ping = " + std::to_string(gClientSession->getLastPing()) + "\n";

                gClientSession.reset();
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
