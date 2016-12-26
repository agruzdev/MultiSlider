#include <memory>
#include "TestsCommon.h"

namespace
{
    static const uint32_t MESSAGES_NUM = 50;

    class ReconfigureHost
        : public TestUser
    {
        std::atomic<bool>* mFinish;
        std::atomic<bool> mSuccess;
    public:

        ReconfigureHost(std::atomic<bool>* finish)
            : mFinish(finish)
        {
            mSuccess = true;
        }

        void onRoomUpdate(Lobby* /*lobby*/, const RoomInfo & room, const std::string & /*sender*/, uint8_t flags)
        {
            if (0 != (flags & LobbyCallback::FLAG_RECONFIGURE_FAIL)) {
                std::cout << "[" + mLobby->getPlayerName() + "] Failed to reconfigure!\n";
                mSuccess = false;
            }
            else {
                std::cout << "[" + mLobby->getPlayerName() + "] Room was updated: limit " + std::to_string(room.getPlayersNumber()) + "  reserved " + std::to_string(room.getReservedPlayersNumber()) + "\n";
            }
        }

        void DoInSession() override
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            mLobby->reconfigure(10, 0);

            std::this_thread::sleep_for(std::chrono::milliseconds(250));

            mLobby->reconfigure(5, 3);

            std::this_thread::sleep_for(std::chrono::milliseconds(250));

            mLobby->reconfigure(4, 2);

            std::this_thread::sleep_for(std::chrono::milliseconds(250));

            mLobby->reconfigure(2, 0);

            std::this_thread::sleep_for(std::chrono::milliseconds(250));

            mLobby->reconfigure(7, 1);

            std::this_thread::sleep_for(std::chrono::milliseconds(250));

            mFinish->store(true);
        }

        bool success() const
        {
            return mSuccess;
        }
    };

    class ReconfigureClient
        : public TestUser
    {
        std::atomic<bool>* mFinish;
    public:

        ReconfigureClient(std::atomic<bool>* finish)
            : mFinish(finish)
        { }

        void onRoomUpdate(Lobby* /*lobby*/, const RoomInfo & room, const std::string & /*sender*/, uint8_t /*flags*/)
        {
            std::cout << "[" + mLobby->getPlayerName() + "] Room was updated: limit " + std::to_string(room.getPlayersNumber()) + "  reserved " + std::to_string(room.getReservedPlayersNumber()) + "\n";
        }

        void DoInSession() override
        {
            while (!mFinish->load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        bool success() const
        {
            return true;
        }
    };
}

TEST(Api, reconfigure)
{
    const std::string ip = "127.0.0.1";
    const uint16_t port = 8800;
    try {
        std::atomic<bool> finish;
        finish = false;
        auto host   = std::make_unique<ReconfigureHost>(&finish);
        auto client = std::make_unique<ReconfigureClient>(&finish);
        RunSessionTest(host.get(), client.get(), ip, port);
        EXPECT_TRUE(host->success());
        EXPECT_TRUE(client->success());
    }
    catch (std::runtime_error & err) {
        std::cout << err.what() << std::endl;
        GTEST_FAIL();
    }
}
