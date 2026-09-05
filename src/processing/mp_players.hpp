#pragma once

#include "prism/common.hpp"
#include "scssdk_value.h"
#include <map>

namespace ets2la_plugin
{
    // used for TruckersMP players (and eventually convoy)
    class MpPlayerProcessor
    {
    public:
        struct mp_player_vehicle_t
        {
            float3_t position;
            quat_t rotation;
            dimension_t dimensions;
        };

        struct mp_player_object_t
        {
            mp_player_vehicle_t vehicle;
            mp_player_vehicle_t trailers[3];
            uint8_t trailer_count;
            float distance;
            float speed;
            float acceleration;
            uint64_t steam_id;
            uint16_t latency;
            bool has_collision;
        };

    private:
        std::map< uint64_t, mp_player_object_t > players = {}; // steam_id as key
        class CMemoryHandler* memory_handler_;
        float3_t truck_pos;

        void write_vehicle_data() const;

    public:
        MpPlayerProcessor( CMemoryHandler* memory_handler )
            : memory_handler_( memory_handler )
        {
        }

        void tick( scs_value_dplacement_t truck_placement );
        // Will add the id to the list if it does not exist yet
        void update_player( uint64_t steam_id, mp_player_object_t vehicle );
        void remove_player( uint64_t steam_id );
    };
}
