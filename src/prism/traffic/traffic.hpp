#pragma once
#include "prism/collections/array_dyn.hpp"
#include "prism/collections/map_hashed.hpp"
#include "prism/collections/map_tiny.hpp"
#include "prism/string.hpp"
#include "prism/unit/unit.hpp"

namespace ets2la_plugin::prism
{
#pragma pack( push, 1 )

    // Size: W 0x07F0, L|A 0x0698
    class traffic_u : public /* [W|L|A 0x10] @ W|L|A 0x00 */ unit_t
    {
        class vehicle_type_info_t;
        class trailer_type_info_t;
        class lane_type_info_t;
        class pending_spawn_t;
        class cached_vehicle_t;
        class item_in_cutscene_t;
        class area_item_info_t;
        class sign_item_info_t;
        class remote_clients_jip_context_t;
        class data_rule_source_t;

    public:
        class spawned_vehicle_t // Size: 0x0010
        {
        public:
            class traffic_ai_vehicle_t* vehicle; // 0x0000 (0x08)
            char pad_0008[ 8 ];                  // 0x0008 (0x08)
        };
        static_assert( sizeof( spawned_vehicle_t ) == 0x10 );

        char pad_WLA_0x0010[ 8 ];                                    // W|L|A 0x0010 (W|L|A 0x08)
        class traffic_data_u* traffic_data;                          // W|L|A 0x0018 (W|L|A 0x08)
        class color_array_u* color_array;                            // W|L|A 0x0020 (W|L|A 0x08)
        array_dyn_t< class vehicle_type_info_t > truck_type_infos;   // W|L|A 0x0028 (W 0x28, L|A 0x20)
        array_dyn_t< class trailer_type_info_t > trailer_type_infos; // W 0x0050, L|A 0x0048 (W 0x28, L|A 0x20)
        array_dyn_t< class lane_type_info_t > lane_type_infos;       // W 0x0078, L|A 0x0068 (W 0x28, L|A 0x20)
        array_dyn_t< class pending_spawn_t* > pending_spawns;        // W 0x00A0, L|A 0x0088 (W 0x28, L|A 0x20)
        array_dyn_t< class cached_vehicle_t > cached_vehicles;       // W 0x00C8, L|A 0x00A8 (W 0x28, L|A 0x20)
        array_dyn_t< class spawned_vehicle_t > spawned_vehicles_1;   // W 0x00F0, L|A 0x00C8 (W 0x28, L|A 0x20)
        array_dyn_t< class spawned_vehicle_t > spawned_vehicles_2;   // W 0x0118, L|A 0x00E8 (W 0x28, L|A 0x20) traffic
                                                                     // vehicles from other convoy users
        array_dyn_t< class spawned_vehicle_t > spawned_vehicles_3;   // W 0x0140, L|A 0x0108 (W 0x28, L|A 0x20) out of
                                                                     // range? traffic vehicles from other convoy users
        array_dyn_t< class traffic_ai_vehicle_t* > traffic_ai_vehicles_1; // W 0x0168, L|A 0x0128 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_ai_vehicle_t* > traffic_ai_vehicles_2; // W 0x0190, L|A 0x0148 (W 0x28, L|A 0x20)
        char pad_W_0x01B8__LA_0x0168[ 40 ];                               // W 0x01B8, L|A 0x0168 (W|L|A 0x28)
        array_dyn_t< class traffic_object_t* > traffic_objects_1;         // W 0x01E0, L|A 0x0190 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_player_vehicle_t* > traffic_player_vehicles_1; // W 0x0208, L|A 0x01B0 (W 0x28, L|A
                                                                                  // 0x20)
        array_dyn_t< class traffic_player_trailer_t* > traffic_player_trailers_1; // W 0x0230, L|A 0x01D0 (W 0x28, L|A
                                                                                  // 0x20)
        array_dyn_t< class traffic_player_vehicle_t* > traffic_player_vehicles_2; // W 0x0258, L|A 0x01F0 (W 0x28, L|A
                                                                                  // 0x20)
        array_dyn_t< class traffic_player_trailer_t* > traffic_player_trailers_2; // W 0x0280, L|A 0x0210 (W 0x28, L|A
                                                                                  // 0x20)
        array_dyn_t< class traffic_world_rule_t* > traffic_world_rules_1; // W 0x02A8, L|A 0x0230 (W 0x28, L|A 0x20)
        class traffic_environment_t* traffic_environment;                 // W 0x02D0, L|A 0x0250 (W|L|A 0x08)
        array_dyn_t< class traffic_segment_t* > traffic_segments_1;       // W 0x02D8, L|A 0x0258 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_segment_t* > traffic_segments_2;       // W 0x0300, L|A 0x0278 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_segment_t* > traffic_segments_3;       // W 0x0328, L|A 0x0298 (W 0x28, L|A 0x20)
        array_dyn_t< class kdop_item_t* > kdop_items_1;                   // W 0x0350, L|A 0x02B8 (W 0x28, L|A 0x20)
        array_dyn_t< class kdop_item_t* > kdop_items_2;                   // W 0x0378, L|A 0x02D8 (W 0x28, L|A 0x20)
        array_dyn_t< class item_in_cutscene_t > items_in_cutscenes;       // W 0x03A0, L|A 0x02F8 (W 0x28, L|A 0x20)
        char pad_W_0x03C8__LA_0x0318[ 56 ];                               // W 0x03C8, L|A 0x0318 (W|L|A 0x38)
        array_dyn_t< class area_item_info_t > area_item_infos_1;          // W 0x0400, L|A 0x0350 (W 0x28, L|A 0x20)
        array_dyn_t< class area_item_info_t > area_item_infos_2;          // W 0x0428, L|A 0x0370 (W 0x28, L|A 0x20)
        char pad_W_0x0450__LA_0x0390[ 24 ];                               // W 0x0450, L|A 0x0390 (W|L|A 0x18)
        array_dyn_t< class sign_item_info_t* > sign_item_infos;           // W 0x0468, L|A 0x03A8 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_gate_t* > traffic_gates;               // W 0x0490, L|A 0x03C8 (W 0x28, L|A 0x20)
        char pad_W_0x04B8__LA_0x03E8[ 16 ];                               // W 0x04B8, L|A 0x03E8 (W|L|A 0x10)
        array_dyn_t< class kdop_item_t* > kdop_items_3;                   // W 0x04C8, L|A 0x03F8 (W 0x28, L|A 0x20)
        char pad_W_0x04F0__LA_0x0418[ 16 ];                               // W 0x04F0, L|A 0x0418 (W|L|A 0x10)
        array_dyn_t< class kdop_item_t* > kdop_items_4;                   // W 0x0500, L|A 0x0428 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_origin_t* > traffic_origins;           // W 0x0528, L|A 0x0448 (W 0x28, L|A 0x20)
        char pad_W_0x0550__LA_0x0468[ 16 ];                               // W 0x0550, L|A 0x0468 (W|L|A 0x10)
        array_dyn_t< class traffic_spawn_point_t* > traffic_spawn_points; // W 0x0560, L|A 0x0478 (W 0x28, L|A 0x20)
        char pad_W_0x0588__LA_0x0498[ 24 ];                               // W 0x0588, L|A 0x0498 (W|L|A 0x18)
        class city_data_u* city_data;                                     // W 0x05A0, L|A 0x04B0 (W|L|A 0x08)
        array_dyn_t< uint32_t > N00001DA9;                                // W 0x05A8, L|A 0x04B8 (W 0x28, L|A 0x20)
        char pad_W_0x05D0__LA_0x04D8[ 8 ];                                // W 0x05D0, L|A 0x04D8 (W|L|A 0x08)
        array_dyn_t< class traffic_object_t* > traffic_objects_2;         // W 0x05D8, L|A 0x04E0 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_object_t* > traffic_objects_3;         // W 0x0600, L|A 0x0500 (W 0x28, L|A 0x20)
        array_dyn_t< void* > N00001DB6;                                   // W 0x0628, L|A 0x0520 (W 0x28, L|A 0x20)
        char pad_W_0x0650__LA_0x0540[ 8 ];                                // W 0x0650, L|A 0x0540 (W|L|A 0x08)
        map_tiny_t< class traffic_rule_data_u*, class traffic_rule_data_u* > N00001DBC; // W 0x0658, L|A 0x0548 (W 0x60,
                                                                                        // L|A 0x48)
        char pad_W_0x06B8__LA_0x0590[ 8 ];                           // W 0x06B8, L|A 0x0590 (W|L|A 0x08)
        array_dyn_t< class remote_clients_jip_context_t > N00001DC5; // W 0x06C0, L|A 0x0598 (W 0x28, L|A 0x20)
        char pad_W_0x06E8__LA_0x05B8[ 104 ];                         // W 0x06E8, L|A 0x05B8 (W|L|A 0x68)
        string_dyn_t N00001DD6;                                      // W 0x0750, L|A 0x0620 (W|L|A 0x18)
        map_hashed_t< class traffic_segment_t*, void* > N0000EDEA;   // W 0x0770, L|A 0x0638 (W 0x80, L|A 0x60)
    };

#if defined( _WIN32 )
    static_assert( sizeof( traffic_u ) == 0x7F0 ); // W
#elif defined( __linux__ ) || defined( __APPLE__ )
    static_assert( sizeof( traffic_u ) == 0x698 ); // L|A
#endif

#pragma pack( pop )
}
