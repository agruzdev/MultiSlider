
#include "../../../client/source/Lobby.h"
#include "../../../client/source/Exception.h"

#include <cassert>
#include <iostream>
#include <future>
#include <thread>

#include <windows.h>

using namespace multislider;

#define VK_NO_KEY 0x00
#define VK_KEY_0 0x30
#define VK_KEY_1 0x31
#define VK_KEY_2 0x32
#define VK_KEY_3 0x33
#define VK_KEY_4 0x34
#define VK_KEY_5 0x35
#define VK_KEY_6 0x36
#define VK_KEY_7 0x37
#define VK_KEY_8 0x38
#define VK_KEY_9 0x39

class Controller
    : public HostCallback, public ClientCallback
{
    Host*      mHost    = nullptr;
    Client*    mClient  = nullptr;
    SessionPtr mSession = nullptr;

    bool mFinish = false;

    bool mHostPlaysCrosses = true;

    //-------------------------------------------------------

    void clear()
    {
        std::system("cls");
    }

    void pause(size_t milliseconds)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    std::vector<std::string> splitCommand(const std::string & cmd)
    {
        std::vector<std::string> tokens;
        size_t pos = 0;
        for (;;) {
            size_t idx = cmd.find_first_of(' ', pos);
            if (idx == std::string::npos) {
                tokens.push_back(cmd.substr(pos));
                break;
            }
            tokens.push_back(cmd.substr(pos, idx - pos));
            pos = idx + 1;
        }
        return tokens;
    }

public:
    void run(std::string ip, uint16_t port)
    {
        mFinish = false;

        Lobby lobby;
        std::string username;
        std::cout << "Please introduce yourself: ";
        std::cin >> username;

        std::string _tmp;
        std::getline(std::cin, _tmp); // Quickfix


        std::string roomname;

        //-------------------------------------------------------
        //-------------------------------------------------------
        // Lobby screen
        //-------------------------------------------------------
        for (;;) {
            const std::string CMD_CREATE = "create";
            const std::string CMD_JOIN   = "join";
            const std::string CMD_UPDATE = "update";
            const std::string CMD_EXIT   = "exit";

            clear();
            std::cin.sync();

            //Header 
            std::cout << "Lobby of " << username << std::endl;
            std::cout << "--------------------------------------------------" << std::endl;
            std::cout << std::endl;

            // Rooms info
            auto rooms = lobby.getRooms();
            std::cout << "Available rooms:" << std::endl;
            size_t idx = 0;
            for (const auto & info : rooms) {
                std::cout << "  [" << idx << "]  " << info.roomName << "\t  by " << info.hostName << "\t        " << info.playersNumber << " / " << info.playersLimit << std::endl;
            }
            std::cout << std::endl;

            //Footer
            std::cout << "Options:" << std::endl;
            std::cout << "  " << CMD_CREATE << " <roomName> - to create a new room" << std::endl;
            std::cout << "  " << CMD_JOIN   << " <number> - to join a room with number <number>" << std::endl;
            std::cout << "  " << CMD_UPDATE << " - to update rooms list" << std::endl;
            std::cout << "  " << CMD_EXIT   << " - to leave" << std::endl;
            std::cout << std::endl;

            // Process command
            std::string cmdFull;
            std::getline(std::cin, cmdFull);
            auto cmd = splitCommand(cmdFull);
            if (cmd.size() == 1 && cmd[0] == CMD_UPDATE) {
                /* continue */
            }
            else if (cmd.size() == 1 && cmd[0] == CMD_EXIT) {
                std::cout << "Bye!" << std::endl;
                pause(1000);
                return;
            }
            else if (cmd.size() == 2 && cmd[0] == CMD_CREATE) {
                roomname = cmd[1];
                mHost = lobby.createRoom(username, roomname, 2, this);
                if (nullptr != mHost) {
                    break;
                }
                else {
                    std::cout << "Sorry! Failed to create the room" << std::endl;
                    pause(1000);
                }
            }
            else if (cmd.size() == 2 && cmd[0] == CMD_JOIN) {
                int roomIdx = std::stoi(cmd[1]);
                if (roomIdx >= 0 && roomIdx < static_cast<int>(rooms.size())) {
                    bool full = false;
                    roomname = rooms[roomIdx].roomName;
                    mClient = lobby.joinRoom(username, rooms[roomIdx], this, full);
                    if (nullptr != mClient) {
                        break;
                    }
                    else  if (full) {
                        std::cout << "Sorry! This room is already full" << std::endl;
                        pause(1000);
                    }
                    else {
                        std::cout << "Sorry! Failed to join the room" << std::endl;
                        pause(1000);
                    }
                }
            }
            else {
                std::cout << "Unknown command!" << std::endl;
                pause(1000);
            }
        };


        //-------------------------------------------------------
        // Setup loop
        assert(mHost != nullptr || mClient != nullptr);
        setupScreen(roomname, mHost != nullptr);

    }

    void setupScreen(const std::string & roomname, bool isHost)
    {
        //-------------------------------------------------------
        //-------------------------------------------------------
        // Setup the room
        //-------------------------------------------------------

        static const std::string CMD_PLAY_SYM = "play";
        static const std::string CMD_UPDATE = "update";
        static const std::string CMD_START = "start";
        for (;;) {
            clear();
            std::cin.sync();

            // Header 
            std::cout << "Room " << roomname << std::endl;
            std::cout << "--------------------------------------------------" << std::endl;
            std::cout << std::endl;

            // Info
            std::cout << "Game options:" << std::endl;
            std::cout << "    Host plays:   " << (mHostPlaysCrosses ? 'X' : 'O') << std::endl;
            std::cout << "    Client plays: " << (mHostPlaysCrosses ? 'O' : 'X') << std::endl;
            std::cout << std::endl;

            // Options
            std::cout << "Options: " << std::endl;
            if (isHost) {
                std::cout << "  " << CMD_PLAY_SYM << " [X or O] - change your symbol" << std::endl;
            }
            std::cout << "  " << CMD_UPDATE << " - update this screen" << std::endl;
            std::cout << "  " << CMD_START << " - start the game!" << std::endl;
            std::cout << std::endl;

            // Process command
            std::string cmdFull;
            std::getline(std::cin, cmdFull);
            auto cmd = splitCommand(cmdFull);
            if (cmd.size() == 2 && cmd[0] == CMD_PLAY_SYM) {
                if (cmd[1] == "X" || cmd[1] == "O") {
                    mHost->broadcast(cmd[1], true);
                }
            }
            else if (cmd.size() == 1 && cmd[0] == CMD_UPDATE) {
                continue;
            }
            else {
                std::cout << "Unknown command" << std::endl;
                continue;
            }
            
            uint32_t count = 0;
            do {
                if (isHost) {
                    count += mHost->receive();
                }
                else {
                    count += mClient->receive();
                }
            } while (count == 0);
        }
    }

    //-------------------------------------------------------
    // Host callback

    void onCreated(const RoomInfo & room) override
    {
        (void)room;
    }

    void onBroadcast(const RoomInfo & room, const std::string & message) override
    {
        mHostPlaysCrosses = (message[0] == 'X');
    }

    void onClosed(const RoomInfo & room) override
    {
        std::cout << std::string("Room \"") + room.roomName + "\" is closed!\n";
        mFinish = true;
    }

    void onSessionStart(const RoomInfo & room, SessionPtr session) override
    {
        std::cout << std::string("Room \"") + room.roomName + "\" session is started!\n";
        mSession = session;
    }


    //-------------------------------------------------------
    // Client callback

    void onJoined(const std::string & playerName, const RoomInfo & room) override
    {
        //setupScreen(room, false);
    }

    void onLeft(const std::string & playerName, const RoomInfo & room) override
    {
        std::cout << std::string("[") + playerName + "]: I left \"" + room.roomName + "\"\n";
        mFinish = true;
    }

    void onBroadcast(const std::string & playerName, const std::string & message) override
    {
        //std::cout << std::string("[") + playerName + "]: I got broadcast message \"" + message + "\"\n";
        mHostPlaysCrosses = (message[0] == 'X');
    }

    void onSessionStart(const std::string & playerName, SessionPtr session) override
    {
        std::cout << std::string("[") + playerName + "]: I got SessionStarted message!\n";
        mSession = session;
    }
};

int main(int argc, char** argv)
{
    std::cout << "Welcome to TickTakToe" << std::endl << std::endl;
    if (argc != 3) {
        std::cout << "Wrong arguments!" << std::endl;
        std::cout << "Usage: TickTackToe.exe <server IP> <server port>" << std::endl;
        std::cout << std::endl;
        return 1;
    }
    std::string ip = std::string(argv[1]);
    uint16_t port  = static_cast<uint16_t>(atoi(argv[2]));

    Controller ctrl;
    ctrl.run(ip, port);

    return 0;
}
