/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#include "RoomInfo.h"
#include "Utility.h"

#include <jsonxx.h>

using namespace jsonxx;

namespace multislider
{
    static const char FIELD_ROOM_NAME[] = "name";
    static const char FIELD_HOST_NAME[] = "host";
    static const char FIELD_PLAYERS_LIMIT[]  = "playersLimit";
    static const char FIELD_PLAYERS_NUMBER[] = "playersNumber";
    static const char FIELD_PLAYERS_ARRAY[]  = "players";

    void RoomInfo::swap(RoomInfo & other)
    {
        if (this != &other) {
            using std::swap;
            swap(mRoomName, other.mRoomName);
            swap(mHostName, other.mHostName);
            swap(mPlayersLimit, other.mPlayersLimit);
            swap(mPlayersNumber, other.mPlayersNumber);
            swap(mPlayers, other.mPlayers);
        }
    }

    bool RoomInfo::deserialize(const std::string & str)
    {
        Object infoJson;
        infoJson.parse(std::string(str.c_str(), str.size()));
        return deserialize(infoJson);
    }

    bool RoomInfo::deserialize(const Object & infoJson)
    {
        RoomInfo info;
        bool success = true;
        do {
            if (infoJson.has<jsonxx::String>(FIELD_ROOM_NAME)) {
                info.mRoomName = infoJson.get<jsonxx::String>(FIELD_ROOM_NAME);
            }
            else {
                success = false;
                break;
            }
            if (infoJson.has<jsonxx::String>(FIELD_HOST_NAME)) {
                info.mHostName = infoJson.get<jsonxx::String>(FIELD_HOST_NAME);
            }
            else {
                success = false;
                break;
            }
            if (infoJson.has<jsonxx::Number>(FIELD_PLAYERS_LIMIT)) {
                info.mPlayersLimit = narrow_cast<uint32_t>(infoJson.get<jsonxx::Number>(FIELD_PLAYERS_LIMIT));
            }
            else {
                success = false;
                break;
            }
            if (infoJson.has<jsonxx::Number>(FIELD_PLAYERS_NUMBER)) {
                info.mPlayersNumber = narrow_cast<uint32_t>(infoJson.get<jsonxx::Number>(FIELD_PLAYERS_NUMBER));
            }
            else {
                success = false;
                break;
            }
            if (infoJson.has<Array>(FIELD_PLAYERS_ARRAY)) {
                Array playersJsonArray = infoJson.get<Array>(FIELD_PLAYERS_ARRAY);
                for (size_t idx = 0; idx < playersJsonArray.size(); ++idx) {
                    info.mPlayers.push_back(playersJsonArray.get<jsonxx::String>(idx));
                }
                if (info.mPlayersNumber != narrow_cast<uint32_t>(info.mPlayers.size())) {
                    success = false;
                    break;
                }
            }
        } while (false);
        if (success) {
            info.swap(*this);
        }
        return success;
    }

}


