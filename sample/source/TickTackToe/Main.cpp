
#include "../../../client/source/Lobby.h"
#include "../../../client/source/Exception.h"

#include <cassert>
#include <iostream>
#include <future>
#include <thread>

#include <windows.h>

using namespace multislider;

static const std::string CMD_CREATE   = "create";
static const std::string CMD_JOIN     = "join";
static const std::string CMD_UPDATE   = "update";
static const std::string CMD_EXIT     = "exit";
static const std::string CMD_PLAY_SYM = "play";
static const std::string CMD_START    = "start";

class Controller
    : public HostCallback, public ClientCallback, public SessionCallback
{
    Host*      mHost    = nullptr;
    Client*    mClient  = nullptr;
    SessionPtr mSession = nullptr;

    bool mHostReady   = false;
    bool mClientReady = false;
    bool mSetupFinish = false;
    bool mGameFinish  = false;

    bool mHostPlaysCrosses = true;

    uint32_t mTurn = 0;

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
        Lobby lobby(ip, port);
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
                std::cout << "  [" << idx << "]  " << info.getName() << "\t  by " << info.getHostName() << "\t        " << info.getPlayersNumber() << " / " << info.getPlayersLimit() << std::endl;
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
                    roomname = rooms[roomIdx].getName();
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

        //-------------------------------------------------------
        // Game loop

        assert(mSession != nullptr);
        gameScreen(roomname, mHost != nullptr);
    }

    void setupScreen(const std::string & roomname, bool isHost)
    {
        auto inputHandler = std::async(std::launch::async, [this]() {
            while(!mSetupFinish) {
                std::string line;
                std::getline(std::cin, line);
                auto cmd = splitCommand(line);
                if (cmd.size() == 2 && cmd[0] == CMD_PLAY_SYM) {
                    if ((mHost != nullptr) && (cmd[1] == "X" || cmd[1] == "O")) {
                        if (mHost != nullptr) {
                            mHost->broadcast(cmd[1], true);
                        }
                    }
                }
                else if (cmd.size() == 1 && cmd[0] == CMD_START) {
                    if (mHost != nullptr) {
                        mHost->broadcast("HR", true);
                    }
                    else {
                        mClient->broadcast("CR", true);
                    }
                    std::cout << "Waiting for all players..." << std::endl;
                    break;
                }
                else {
                    std::cout << "Unknown command!" << std::endl;
                }
            }
        });
        while (!mSetupFinish) {
            if (mHost != nullptr) {
                mHost->receive();
            }
            else {
                mClient->receive();
            }
            pause(100);
        }
        inputHandler.wait();
    }

    void drawRoomScreen(const RoomInfo & room, bool isHost)
    {
        clear();
        std::cin.clear();

        // Header 
        std::cout << "Room " << room.getName() << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << std::endl;

        if (room.getPlayersNumber() < 2) {
            std::cout << "Waiting for the second player..." << std::endl;
            std::cout << std::endl;
        }
        else {
            // Info
            std::cout << "Game options:" << std::endl;
            std::cout << "    " << room.getPlayers()[0] << "\t plays: " << (mHostPlaysCrosses ? 'X' : 'O') << "\t \t" << (mHostReady ? " ready!" : "") << std::endl;
            std::cout << "    " << room.getPlayers()[1] << "\t plays: " << (mHostPlaysCrosses ? 'O' : 'X') << "\t \t" << (mClientReady ? " ready!" : "") << std::endl;
            std::cout << std::endl;

            // Options
            std::cout << "Options: " << std::endl;
            if (isHost) {
                std::cout << "  " << CMD_PLAY_SYM << " [X or O] - change your symbol" << std::endl;
            }
            std::cout << "  " << CMD_START << " - start the game!" << std::endl;
            std::cout << std::endl;
        }
    }


    void gameScreen(const std::string & roomname, bool isHost)
    {
        while (!mGameFinish) {
            mSession->receive();
            pause(100);
        }
    }

    void drawGameScreen(const std::string & roomname, const std::string & fieldData, bool myTurn, char mySymbol, int winCode)
    {
        clear();
        std::cin.sync();

        std::cout << "Room " << roomname << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
        std::cout << std::endl;
        std::cout << "You play as " << mySymbol << std::endl;
        std::cout << std::endl;

        if (fieldData.size() == 9) {
            std::cout << std::endl;
            std::cout << "            " << " 1   2   3 " << std::endl;
            std::cout << std::endl;
            std::cout << "        a   " << " " << fieldData[0] << " | " << fieldData[1] << " | " << fieldData[2] << std::endl;
            std::cout << "            " << "---|---|---" << std::endl;
            std::cout << "        b   " << " " << fieldData[3] << " | " << fieldData[4] << " | " << fieldData[5] << std::endl;
            std::cout << "            " << "---|---|---" << std::endl;
            std::cout << "        c   " << " " << fieldData[6] << " | " << fieldData[7] << " | " << fieldData[8] << std::endl;
        }
        std::cout << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  <letter><number> - to make turn into the position <letter><number>" << std::endl;
        std::cout << std::endl;

        switch (winCode) {
        default:
        case 0:
            if (myTurn) {
                std::cout << "Your turn!" << std::endl;
            }
            else {
                std::cout << "Please wait..." << std::endl;
            }
            break;
        case 1:
            std::cout << "You win!" << std::endl;
            break;
        case 2:
            std::cout << "You lost!" << std::endl;
            break;
        case 3:
            std::cout << "Draw!" << std::endl;
            break;
        }
        std::cout << std::endl;
    }

    void unblockInput()
    {
        std::cin.putback('\n');
    }

    std::string makeMessage(const std::string & field, uint32_t turn)
    {
        return field + std::to_string(turn);
    }

    std::tuple<std::string, uint32_t> parseMessage(const std::string & msg)
    {
        return std::make_tuple(msg.substr(0, 9), static_cast<uint32_t>(std::stoul(msg.substr(9))));
    }

    char checkWin(const std::string & field)
    {
        auto equal = [](char x, char y, char z) -> bool { return (x == y) && (y == z); };
        // rows
        for (uint8_t j = 0; j < 3; ++j) {
            if (equal(field[j * 3], field[j * 3 + 1], field[j * 3 + 2])) {
                return field[j * 3];
            }
        }
        // columns
        for (uint8_t i = 0; i < 3; ++i) {
            if (equal(field[i], field[3 + i], field[6 + i])) {
                return field[i];
            }
        }
        // diagonals
        if (equal(field[0], field[4], field[8])) {
            return field[0];
        }
        if (equal(field[2], field[4], field[6])) {
            return field[2];
        }
        return ' ';
    }

    //-------------------------------------------------------
    // Host callback

    void onCreated(Host* host, const RoomInfo & room) override
    {
        (void)room;
        drawRoomScreen(room, true);
    }

    void onBroadcast(Host* host, const RoomInfo & room, const std::string & message) override
    {
        if (!message.empty()) {
            if (message.size() == 1) {
                mHostPlaysCrosses = (message[0] == 'X');
            }
            else if(message.size() == 2 && message[1] == 'R') {
                if (message[0] == 'H') {
                    mHostReady = true;
                }
                else if (message[0] == 'C') {
                    mClientReady = true;
                }
            }
        }
        drawRoomScreen(room, true);
        if (mHostReady && mClientReady) {
            host->startSession();
        }
    }

    void onClosed(Host* host, const RoomInfo & room) override
    {
        //std::cout << std::string("Room \"") + room.getName() + "\" is closed!\n";
        //mFinish = true;
    }

    void onSessionStart(Host* host, const RoomInfo & room, SessionPtr session) override
    {
        //std::cout << std::string("Room \"") + room.getName() + "\" session is started!\n";
        mSession = session;
        mSession->startup(this, 5 * 1000);
        mSetupFinish = true;
    }


    //-------------------------------------------------------
    // Client callback

    void onJoined(Client* client, const std::string & playerName, const RoomInfo & room) override
    {
        drawRoomScreen(room, false);
    }

    void onLeft(Client* client, const std::string & playerName, const RoomInfo & room) override
    {
        //std::cout << std::string("[") + playerName + "]: I left \"" + room.getName() + "\"\n";
    }

    void onBroadcast(Client* client, const std::string & playerName, const RoomInfo & room, const std::string & message) override
    {
        if (!message.empty()) {
            if (message.size() == 1) {
                mHostPlaysCrosses = (message[0] == 'X');
            }
            else if (message.size() == 2 && message[1] == 'R') {
                if (message[0] == 'H') {
                    mHostReady = true;
                }
                else if (message[0] == 'C') {
                    mClientReady = true;
                }
            }
        }
        drawRoomScreen(room, false);
    }

    void onSessionStart(Client* client, const std::string & playerName, const RoomInfo & room, SessionPtr session) override
    {
        //std::cout << std::string("[") + playerName + "]: I got SessionStarted message!\n";
        mSession = session;
        mSession->startup(this, 5 * 1000);
        mSetupFinish = true;
    }

    //-------------------------------------------------------
    // Session callback

    void onStart(const std::string & sessionName, const std::string & playerName) override
    {
        std::cout << std::string("SessionCallback[") + playerName + "]: Started session " + sessionName + "\n";
        if (mHost != nullptr) {
            mSession->broadcast("", makeMessage("         ", 0), true);
        }
    }

    void onUpdate(const std::string & sessionName, const std::string & playerName, const SessionData & data, const std::string & sharedData) override
    {
        std::string field;
        uint32_t turn;
        std::tie(field, turn) = parseMessage(sharedData);

        const bool isHost   = (mHost != nullptr);
        const bool hostTurn = (turn % 2 == 0);
        const bool myTurn   = (isHost && hostTurn) || (!mHost && !hostTurn);
        const char mySymbol = ((isHost && mHostPlaysCrosses) || (!mHost && !mHostPlaysCrosses)) ? 'X' : 'O';

        // Check win
        const char winSym = checkWin(field);
        int winCode = 0; // 0 - nothing, 1 - win, 2 - loose, 3 - draw
        if (winSym != ' ') {
            winCode = (winSym == mySymbol) ? 1 : 2;
        }
        if (0 == winCode) {
            if (field.find(' ') == std::string::npos) {
                winCode = 3;
            }
        }
        if (winCode != 0) {
            mGameFinish = true;
        }

        for (;;) {
            // Draw screen
            drawGameScreen(sessionName, field, myTurn, mySymbol, winCode);
            // Make turn
            if (!mGameFinish && myTurn) {
                std::string s;
                std::getline(std::cin, s);
                if (s.size() == 2) {
                    const char letter = s[0];
                    const int  number = s[1] - '0';
                    if (std::string("abc").find(letter) != std::string::npos && number >= 1 && number <= 3) {
                        const int  idx = (letter - 'a') * 3 + number - 1;
                        if (field[idx] == ' ') {
                            field[idx] = mySymbol;
                            mSession->broadcast(s, makeMessage(field, turn + 1), true);
                            break;
                        }
                    }
                }
                std::cout << "Invalid turn" << std::endl;
                pause(1000);
            }
            else {
                break;
            }
        }
    }

    void onSync(const std::string & sessionName, const std::string & playerName, uint32_t syncId) override
    {
        //std::cout << std::string("SessionCallback[") + playerName + "]: Got sync " + std::to_string(syncId) + "\n";
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

    try {
        Controller ctrl;
        ctrl.run(ip, port);
    }
    catch (RuntimeError & err) {
        std::cout << err.what() << std::endl;
    }
    return 0;
}
