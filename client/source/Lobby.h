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
    class UdpSocket;

    class Lobby
    {
    public:
        typedef LobbyCallback Callback;

        enum Status
        {
            SUCCESS,        ///< Returned on success operation
            FAIL,           ///< Returned on failed operation; no specific fail reason
            ROOM_IS_FULL,   ///< Trying to join a full room
            NAME_EXISTS,    ///< Player with such name is already in the room
            ROOM_EXISTS     ///< Trying to create a room with existing name
        };

        //-------------------------------------------------------

    private:
        shared_ptr<RakNet::TCPInterface> mTcp;
        shared_ptr<RakNet::SystemAddress> mServerAddress;
        shared_ptr<UdpSocket> mUpdSocket;

        std::string mServerIp;
        uint16_t mServerPort;

        RoomInfo mMyRoom;
        std::string mPlayerName;
        LobbyCallback* mCallback;
        bool mIsJoined;
        bool mIsHost;

        mutable std::vector<uint8_t> mReceiveBuffer;
        //-------------------------------------------------------

        jsonxx::Object makeEnvelop(const jsonxx::Object & obj) const;

        Lobby(const Lobby &);
        Lobby & operator=(const Lobby &);
        //-------------------------------------------------------

    public:
        MULTISLIDER_EXPORT
        Lobby(const std::string & serverIp, uint16_t serverPort);

        MULTISLIDER_EXPORT
        ~Lobby();

        /**
         *  Create new room on the server
         *  @param playerName name of the current player. It will be assigned as host of the created room
         *  @param roomName name of the room
         *  @param description description of the room
         *  @param playersLimit maximum number of players in the room (should be >= 1)
         *  @param playersReserved number of reserved players slots in the room (0 <= playersReserved < playersLimit)
         *  @param callback callback for host events, can't be null
         */
        MULTISLIDER_EXPORT
        Status createRoom(const std::string & playerName, const std::string & roomName, const std::string & description, uint32_t playersLimit, uint32_t playersReserved, LobbyCallback* callback);

        /**
         *  Create new room on the server
         *  @param playerName name of the current player. It will be assigned as host of the created room
         *  @param roomName name of the room
         *  @param description description of the room
         *  @param playersLimit maximum number of players in the room (should be >= 1)
         *  @param playersReserved number of reserved players slots in the room (0 <= playersReserved < playersLimit)
         *  @param callback callback for host events, can't be null
         *  @param userParameter arbitrary user data
         */
        MULTISLIDER_EXPORT
        Status createRoom(const std::string & playerName, const std::string & roomName, const std::string & description, uint32_t playersLimit, uint32_t playersReserved, const std::string & userParameter, LobbyCallback* callback);
         

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
        Status joinRoom(const std::string & playerName, const RoomInfo & room, LobbyCallback* callback);

        //-------------------------------------------------------
        // Host specific functions

        /**
         *  Eject player from the room
         */
        MULTISLIDER_EXPORT
        void eject(const std::string & playerName);

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

        /**
         *  Start game session for all joined players
         *  @param sessionData user data passed to all players on session start
         */
        MULTISLIDER_EXPORT
        void startSession(const std::string & sessionData);

        //-------------------------------------------------------
        // Common functions for both host and client

        /**
         *  Leave the current room
         *  After leaving Client instance can't be reused for another room
         */
        MULTISLIDER_EXPORT
        void leaveRoom();

        /**
         *  Broadcast data to all players in the room. 
         *  Not blocking call
         */
        MULTISLIDER_DEPRECATED(Will be changed to room reconfiguration later. Use the method say() instead.)
        MULTISLIDER_EXPORT
        void broadcast(const std::string & data, bool toSelf);

        /**
         *  Send message to all players
         *  @param message User's message
         */
        MULTISLIDER_EXPORT
        void say(const std::string & message);

        /**
         *  Check incoming broadcast messages and call callback for the each message
         *  @return number of processed messages
         */
        MULTISLIDER_EXPORT
        uint32_t receive();

        //-------------------------------------------------------

        /**
         *  Return true if player is joined a room
         */
        bool isJoined() const
        {
            return mIsJoined;
        }

        /**
         *  Return true if player is host in the current room
         *  Is valid only if isJoined() returned true
         */
        bool isHost() const
        {
            return mIsHost;
        }

        /**
         * Get name of the current player
         */
        const std::string & getPlayerName() const 
        {
            return mPlayerName;
        }
    };

}

#endif
