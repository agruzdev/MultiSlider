/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_LOBBY_H_
#define _MULTI_SLIDER_LOBBY_H_

#include <vector>

#include "LobbyCallback.h"
#include "RoomInfo.h"

namespace multislider
{

    class Lobby
    {
    public:
        typedef LobbyCallback Callback;
        //-------------------------------------------------------

    private:
        shared_ptr<RakNet::TCPInterface> mTcp;
        shared_ptr<RakNet::SystemAddress> mServerAddress;

        RoomInfo mMyRoom;
        std::string mPlayerName;
        LobbyCallback* mCallback;
        bool mIsJoined;
        bool mIsHost;

        //shared_ptr<Host>   mHostInstance;
        shared_ptr<Client> mClientInstance;

        Lobby(const Lobby &);
        Lobby & operator=(const Lobby &);

    public:
        MULTISLIDER_EXPORT
        Lobby(const std::string & serverIp, uint16_t serverPort);

        MULTISLIDER_EXPORT
        ~Lobby();

        /**
         *  Create new room on the server
         *  @param playerName name of the current player. It will be assigned as host of the created room
         *  @param roomName name of the room
         *  @param palyersLimit maximum number of players in the room (should be >= 1)
         *  @param callback callback for host events, can't be null
         *  @return true on success
         */
        MULTISLIDER_EXPORT
        bool createRoom(const std::string & playerName, const std::string & roomName, uint32_t playersLimit, LobbyCallback* callback);

        /**
         *  Get a list of all opened rooms on the server
         */
        MULTISLIDER_EXPORT
        std::vector<RoomInfo> getRooms() const;

        /**
         *  Join a room
         *  @param playerName name of the current player
         *  @param room reference to the room info to join
         *  @param callback callback for client events, can't be null
         */
        MULTISLIDER_EXPORT
        Client* joinRoom(const std::string & playerName, const RoomInfo & room, LobbyCallback* callback, bool & isFull);

        //-------------------------------------------------------
        // Host specific functions

        /**
         *  Close the current room
         */
        MULTISLIDER_EXPORT
        void closeRoom();

        /**
         *  Start game session for all joined players
         */
        MULTISLIDER_EXPORT
        void startSession();

        //-------------------------------------------------------
        // COmmon functions for both host and client

        /**
         *  Broadcast data to all players in the room
         *  Not blocking call
         */
        MULTISLIDER_EXPORT
        void broadcast(const std::string & data, bool toSelf);

        /**
         *  Check incoming broadcast messages and call callback for the each message
         *  @return number of processed messages
         */
        MULTISLIDER_EXPORT
        uint32_t receive();


    };

}

#endif
