
#include "../../client/source/Lobby.h"

#include <iostream>
#include <mutex>
#include <future>
#include <atomic>
#include <condition_variable>

using namespace multislider;

std::atomic_bool gFlagJoin  { false };
std::atomic_bool gFlagFinish{ false };

std::condition_variable gCvJoin;
std::condition_variable gCvFinish;

class HostCallbackSample
    : public HostCallback
{
public:
    void onCreated(RoomInfo & room) override
    {
        std::cout << "Room \"" << room.roomName << "\" is created!" << std::endl;
    }

    void onClosed(RoomInfo & room) override
    {
        std::cout << "Room \"" << room.roomName << "\" is closed!" << std::endl;
    }
};


class HostSample
{
    std::mutex mMutex;
public:
    void run()
    {
        HostCallbackSample callback;
        {
            Lobby lobby;
            Host* host = lobby.createRoom("Player1", "Room1", &callback);

            gFlagJoin = true;
            gCvJoin.notify_one();

            std::unique_lock<std::mutex> lock(mMutex);
            gCvFinish.wait(lock, []() {return gFlagFinish.load(); });
            //host->closeRoom();
        }
    }
};

class ClientSample
{
    std::mutex mMutex;
public:
    void run()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        gCvJoin.wait(lock, []() {return gFlagJoin.load(); });

        Lobby lobby;
        std::vector<RoomInfo> rooms = lobby.getRooms();
        for (auto & info : rooms) {
            std::cout << info.roomName << " by " << info.hostName << std::endl;
        }

        gFlagFinish = true;
        gCvFinish.notify_one();
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
