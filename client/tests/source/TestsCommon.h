#include <future>
#include <stdexcept>
#include <thread>
#include <chrono>

#include <gtest/gtest.h>

#include "../../../client/source/MultiSlider.h"

using namespace multislider;

class TestUser
    : public LobbyCallback, public SessionCallback
{
protected:
    shared_ptr<Lobby> mLobby;
    shared_ptr<Session> mSession;

    std::mutex mStartMutex;
    std::condition_variable mStartCv;

public:

    void onSessionStart(Lobby* lobby, const RoomInfo & /*room*/, SessionPtr session, const std::string & /*sessionData*/) override
    {
        std::cout << "[" + lobby->getPlayerName() + "]: Started session\n";
        mSession = session;
        mStartCv.notify_all();
    }
    //-------------------------------------------------------

    void initHost(const std::string & ip, uint16_t port)
    {
        mLobby.reset(new Lobby(ip, port));
        if (Lobby::SUCCESS != mLobby->createRoom("host", "test", "", 2, 0, this)) {
            throw std::runtime_error("Filed to create");
        }
    }

    void initClient(const std::string & ip, uint16_t port)
    {
        mLobby.reset(new Lobby(ip, port));
        std::vector<RoomInfo> rooms = mLobby->getRooms();
        if (rooms.empty()) {
            throw std::runtime_error("No rooms");
        }
        if (Lobby::SUCCESS != mLobby->joinRoom("client", rooms[0], this)) {
            throw std::runtime_error("Filed to join");
        }
    }

    void startSessionHost()
    {
        mLobby->startSession();
        InSesion();
    }

    void startSessionClient()
    {
        InSesion();
    }

    void receiveAll()
    {
        if (nullptr != mLobby) {
            mLobby->receive();
        }
        if (nullptr != mSession) {
            if (mSession->started()) {
                mSession->keepAlive();
                mSession->receive(1);
            }
        }
    }

    void InSesion()
    {
        std::unique_lock<std::mutex> lock(mStartMutex);
        mStartCv.wait(lock, [this]() {
            return mSession != nullptr;
        });
        mSession->startup(this, 1000000);
        DoInSession();
    }
    //-------------------------------------------------------

    virtual void DoInSession()
    {
    }
};

void RunSessionTest(TestUser* host, TestUser* client, const std::string & ip, uint16_t port)
{
    host->initHost(ip, port);
    client->initClient(ip, port);

    std::atomic<bool> finish;
    std::atomic_init(&finish, false);

    std::mutex errorsMutex;
    std::vector<std::exception_ptr> errors;

    auto receiverThread = std::async(std::launch::async, [&errorsMutex, &errors, &finish, &host, &client] {
        try {
            while (!finish.load()) {
                host->receiveAll();
                client->receiveAll();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        catch (std::runtime_error & err) {
            std::unique_lock<std::mutex> lock(errorsMutex);
            std::cout << err.what() << std::endl;
            errors.push_back(std::make_exception_ptr(err));
        }
    });

    auto hostThread = std::async(std::launch::async, [&errorsMutex, &errors, &host]() {
        try {
            host->startSessionHost();
        }
        catch (std::runtime_error & err) {
            std::unique_lock<std::mutex> lock(errorsMutex);
            std::cout << err.what() << std::endl;
            errors.push_back(std::make_exception_ptr(err));
        }
    });

    auto clientThread = std::async(std::launch::async, [&errorsMutex, &errors, &client]() {
        try {
            client->startSessionClient();
        }
        catch (std::runtime_error & err) {
            std::unique_lock<std::mutex> lock(errorsMutex);
            std::cout << err.what() << std::endl;
            errors.push_back(std::make_exception_ptr(err));
        }
    });

    auto checkErrors = [&errors, &errorsMutex]() {
        if (!errors.empty()) {
            std::rethrow_exception(errors.front());
        }
    };

    hostThread.wait();
    checkErrors();
    clientThread.wait();
    checkErrors();
    finish = true;
    receiverThread.wait();
    checkErrors();
}


