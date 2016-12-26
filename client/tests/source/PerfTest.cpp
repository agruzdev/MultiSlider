#include <memory>
#include "TestsCommon.h"

namespace
{
    static const uint32_t MESSAGES_NUM = 20;

    class ReconfigureHost
        : public TestUser
    {
        std::atomic<bool>* mFinish;
    public:

        ReconfigureHost(std::atomic<bool>* finish)
            : mFinish(finish)
        { }

        void DoInSession() override
        {
            while (!*mFinish) {
                mSession->say("ping");
            }
        }

        bool success() const
        {
            return true;
        }
    };

    class ReconfigureClient
        : public TestUser
    {
        std::atomic<bool>* mFinish;
        std::chrono::high_resolution_clock::time_point mStartTime;
        std::chrono::high_resolution_clock::time_point mEndTime;
        size_t mCounter = 0;
        bool mFirst = true;
    public:

        ReconfigureClient(std::atomic<bool>* finish)
            : mFinish(finish)
        { }

        void onMessage(Session* session, const std::string & sender, const std::string & message) override
        {
            if (!(*mFinish)) {
                if (mFirst) {
                    mStartTime = std::chrono::high_resolution_clock::now();
                    mFirst = false;
                }
                else {
                    mEndTime = std::chrono::high_resolution_clock::now();
                }
            }
            ++mCounter;
            if (mCounter >= MESSAGES_NUM) {
                mFinish->store(true);
            }
        }

        void DoInSession() override
        {
            while (*mFinish) {
                mSession->receive();
            }
        }

        bool success() const
        {
            std::cout << "Messages per second = " << (static_cast<float>(MESSAGES_NUM) / (std::chrono::duration_cast<std::chrono::milliseconds>(mEndTime - mStartTime).count() / 1e3)) << std::endl;
            return true;
        }
    };
}

TEST(Functional, PerfTest)
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
