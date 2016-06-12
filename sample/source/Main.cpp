
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

class Callback
    : public HostCallback
{ };


class HostSample
{
    std::mutex mMutex;
public:
    void run()
    {
        Lobby lobby;
        Callback callback;
        Host* host = lobby.createRoom("Player1", "Room1", &callback);

        gFlagJoin = true;
        gCvJoin.notify_one();

        std::unique_lock<std::mutex> lock(mMutex);
        gCvFinish.wait(lock, []() {return gFlagFinish.load(); });
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
    auto host   = std::async(std::launch::async, []() { HostSample().run(); });
    auto client = std::async(std::launch::async, []() { ClientSample().run(); });

    client.wait();
    host.wait();

    return 0;
}
