#include "mp_players.hpp"
#include "core.hpp"
#include "memory/virtual/memory_handler.hpp"
#include "prism/common.hpp"
#include <algorithm>
#include <vector>

namespace ets2la_plugin
{
    void MpPlayerProcessor::tick( scs_value_dplacement_t truck_placement )
    {
        this->truck_pos = float3_t{
            ( float )truck_placement.position.x,
            ( float )truck_placement.position.y,
            ( float )truck_placement.position.z
        };

        this->write_vehicle_data();
    }

    void MpPlayerProcessor::update_player( uint64_t steam_id, mp_player_object_t player )
    {
        this->players[steam_id] = player;
    }

    void MpPlayerProcessor::remove_player( uint64_t steam_id ) { this->players.erase( steam_id ); }

    void MpPlayerProcessor::write_vehicle_data() const
    {
        std::vector< mp_player_object_t > sorted_players = {};

        // get all values from the map into a sortable vector
        transform(
            this->players.begin(),
            this->players.end(),
            back_inserter( sorted_players ),
            []( const auto& val ) { return val.second; }
        );

        std::sort(
            sorted_players.begin(),
            sorted_players.end(),
            []( const mp_player_object_t& a, const mp_player_object_t& b ) { return a.distance < b.distance; }
        );

        std::array< MpPlayer, 40 > mp_players_objects = {};
        int count                                     = 0;

        for ( const auto& player : sorted_players )
        {
            if ( count >= 40 )
            {
                break;
            }

            MpPlayer player_object      = {};
            player_object.steam_id      = player.steam_id;
            player_object.latency       = player.latency;
            player_object.has_collision = player.has_collision;

            player_object.vehicle_data.vehicle = TrafficVehicle{
                player.vehicle.position.x,
                player.vehicle.position.y,
                player.vehicle.position.z,
                player.vehicle.rotation.w,
                player.vehicle.rotation.x,
                player.vehicle.rotation.y,
                player.vehicle.rotation.z,
                player.vehicle.dimensions.width,
                player.vehicle.dimensions.height,
                player.vehicle.dimensions.length,
                player.speed,        // speed
                player.acceleration, // acceleration (n/a for TruckersMP)
                0,                   // trailer_count
                0                   // id (use steamid on client-side?)
            };

            int i;
            for ( i = 0; i < player.trailer_count; ++i )
            {
                auto& trailer                            = player.trailers[ i ];
                player_object.vehicle_data.trailers[ i ] = TrafficTrailer{
                    trailer.position.x,
                    trailer.position.y,
                    trailer.position.z,
                    trailer.rotation.w,
                    trailer.rotation.x,
                    trailer.rotation.y,
                    trailer.rotation.z,
                    trailer.dimensions.width,
                    trailer.dimensions.height,
                    trailer.dimensions.length,
                };
            }

            player_object.vehicle_data.vehicle.trailer_count = i;

            if ( player_object.vehicle_data.vehicle.id == -1 )
            {
                continue;
            }

            mp_players_objects[ count ] = player_object;
            ++count;
        }

        MpPlayersMemData data = { mp_players_objects };

        this->memory_handler_->write_mp_players_mem( data );
    }

}
