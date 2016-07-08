/**
* MultiSlider library
*
* The MIT License (MIT)
* Copyright (c) 2016 Alexey Gruzdev
*/

#ifndef _MULTI_SLIDER_ROOM_INFO_H_
#define _MULTI_SLIDER_ROOM_INFO_H_

#include <string>
#include <vector>

namespace jsonxx
{
    class Object;
}

namespace multislider
{
    class RoomInfo
    {
        std::string mRoomName;
        std::string mHostName;
        uint32_t mPlayersLimit;
        uint32_t mPlayersNumber;
        std::vector<std::string> mPlayers;
        //-------------------------------------------------------
    
    public:
        RoomInfo() 
            : mPlayersLimit(0), mPlayersNumber(0)
        { }

        /**
         *  Read room info from JSON string
         */
        bool deserialize(const std::string & str);

        /**
         *  Read room info from JSON 
         */
        bool deserialize(const jsonxx::Object & str);

        /**
         *  Noexcept swap
         */
        void swap(RoomInfo & other) throw();

        //-------------------------------------------------------
        // Getters

        const std::string & getName() const
        {
            return mRoomName;
        }

        const std::string & getHostName() const
        {
            return mHostName;
        }

        uint32_t getPlayersLimit() const
        {
            return mPlayersLimit;
        }

        uint32_t getPlayersNumber() const
        {
            return mPlayersNumber;
        }

        const std::vector<std::string> & getPlayers() const
        {
            return mPlayers;
        }
    };

    inline
    void swap(RoomInfo & one, RoomInfo & another)
    {
        one.swap(another);
    }
}

#endif 

