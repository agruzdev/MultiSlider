#include <memory>
#include "TestsCommon.h"

namespace
{
    static const uint32_t MESSAGES_NUM = 10;

    class ReliabilitySyncHost
        : public TestUser
    {
        std::atomic<bool> mSuccess;
    public:

        ReliabilitySyncHost()
        {
            mSuccess = true;
        }

        void onSync(Session* /*session*/, uint32_t id, bool wasLost) override
        {
            if (wasLost) {
                std::cout << "[host] Sync message " + std::to_string(id) + " was lost!\n";
                mSuccess = false;
            }
        }

        void DoInSession() override
        {
            for (uint32_t i = 0; i < MESSAGES_NUM; ++i) {
                while (!mSession->sync(i, 0, true)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }

        bool success() const
        {
            return mSuccess;
        }
    };

    class ReliabilitySyncClient
        : public TestUser
    {
        std::atomic<uint32_t> mCounter;
    public:

        ReliabilitySyncClient()
        {
            mCounter = 0;
        }

        void onSync(Session* /*session*/, uint32_t id, bool /*wasLost*/) override
        {
            std::cout << "[host] Got sync message " + std::to_string(id) + "\n";
            ++mCounter;
        }

        void DoInSession() override
        {
            uint32_t attempts = 0;
            while (mCounter < MESSAGES_NUM && attempts++ < 100) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        bool success() const
        {
            return mCounter == MESSAGES_NUM;
        }
    };
}

TEST(Reliability, Sync)
{
    const std::string ip = "127.0.0.1";
    const uint16_t port = 8800;
    try {
        auto host   = std::make_unique<ReliabilitySyncHost>();
        auto client = std::make_unique<ReliabilitySyncClient>();
        RunSessionTest(host.get(), client.get(), ip, port);
        EXPECT_TRUE(host->success());
        EXPECT_TRUE(client->success());
    }
    catch (std::runtime_error & err) {
        std::cout << err.what() << std::endl;
        GTEST_FAIL();
    }
}
