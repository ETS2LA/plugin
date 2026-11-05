#pragma once
#include "prism/collections/array_dyn.hpp"
#include "prism/collections/map_hashed.hpp"
#include "prism/collections/map_tiny.hpp"
#include "prism/string.hpp"
#include "prism/unit/unit.hpp"

namespace ets2la_plugin::prism
{
#pragma pack( push, 1 )

    // Size: W 0x0828, L|A 0x06D0 1.62
    class traffic_u : public /* [W|L|A 0x10] @ W|L|A 0x00 */ unit_t
    {
    public:
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

        class spawned_vehicle_t // Size: 0x0010
        {
        public:
            class traffic_ai_vehicle_t* vehicle; // 0x0000 (0x08)
            char pad_0008[ 8 ];                  // 0x0008 (0x08)
        };
        static_assert( sizeof( spawned_vehicle_t ) == 0x10 );

        char pad_WLA_0x0010[ 8 ];                                         // W|L|A 0x0010 (W|L|A 0x08)
        class traffic_data_u* traffic_data;                               // W|L|A 0x0018 (W|L|A 0x08)
        class color_array_u* color_array;                                 // W|L|A 0x0020 (W|L|A 0x08)
        array_dyn_t< class vehicle_type_info_t > truck_type_infos;        // W|L|A 0x0028 (W 0x28, L|A 0x20)
        array_dyn_t< class trailer_type_info_t > trailer_type_infos;      // W 0x0050, L|A 0x0048 (W 0x28, L|A 0x20)
        array_dyn_t< class lane_type_info_t > lane_type_infos;            // W 0x0078, L|A 0x0068 (W 0x28, L|A 0x20)
        array_dyn_t< class pending_spawn_t* > pending_spawns;             // W 0x00A0, L|A 0x0088 (W 0x28, L|A 0x20)
        array_dyn_t< class cached_vehicle_t > cached_vehicles;            // W 0x00C8, L|A 0x00A8 (W 0x28, L|A 0x20)
        array_dyn_t< class spawned_vehicle_t > spawned_vehicles_1;        // W 0x00F0, L|A 0x00C8 (W 0x28, L|A 0x20)
        class traffic_history_t* traffic_history;                         // W 0x0118, L|A 0x00E8 (W|L|A 0x08)
        array_dyn_t< class spawned_vehicle_t > spawned_vehicles_2;        // W 0x0120, L|A 0x00F0 (W 0x28, L|A
                                                                          // 0x20) traffic vehicles from other
                                                                          // convoy users
        array_dyn_t< class spawned_vehicle_t > spawned_vehicles_3;        // W 0x0148, L|A 0x0110 (W 0x28, L|A
                                                                          // 0x20) out of range? traffic
                                                                          // vehicles from other convoy users
        array_dyn_t< class traffic_ai_vehicle_t* > traffic_ai_vehicles_1; // W 0x0170, L|A 0x0130 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_ai_vehicle_t* > traffic_ai_vehicles_2; // W 0x0198, L|A 0x0150 (W 0x28, L|A 0x20)
        char pad_W_0x01C0__LA_0x0170[ 40 ];                               // W 0x01C0, L|A 0x0170 (W|L|A 0x28)
        array_dyn_t< class traffic_object_t* > traffic_objects_1;         // W 0x01E8, L|A 0x0198 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_player_vehicle_t* > traffic_player_vehicles_1; // W 0x0210, L|A 0x01B8 (W 0x28,
                                                                                  // L|A 0x20)
        array_dyn_t< class traffic_player_trailer_t* > traffic_player_trailers_1; // W 0x0238, L|A 0x01D8 (W 0x28,
                                                                                  // L|A 0x20)
        array_dyn_t< class traffic_player_vehicle_t* > traffic_player_vehicles_2; // W 0x0260, L|A 0x01F8 (W 0x28,
                                                                                  // L|A 0x20)
        array_dyn_t< class traffic_player_trailer_t* > traffic_player_trailers_2; // W 0x0288, L|A 0x0218 (W 0x28,
                                                                                  // L|A 0x20)
        array_dyn_t< class traffic_world_rule_t* > traffic_world_rules_1; // W 0x02B0, L|A 0x0238 (W 0x28, L|A 0x20)
        class traffic_environment_t* traffic_environment;                 // W 0x02D8, L|A 0x0258 (W|L|A 0x08)
        array_dyn_t< class traffic_segment_t* > traffic_segments_1;       // W 0x02E0, L|A 0x0260 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_segment_t* > traffic_segments_2;       // W 0x0308, L|A 0x0280 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_segment_t* > traffic_segments_3;       // W 0x0330, L|A 0x02A0 (W 0x28, L|A 0x20)
        array_dyn_t< class kdop_item_t* > kdop_items_1;                   // W 0x0358, L|A 0x02C0 (W 0x28, L|A 0x20)
        array_dyn_t< class kdop_item_t* > kdop_items_2;                   // W 0x0380, L|A 0x02E0 (W 0x28, L|A 0x20)
        array_dyn_t< class item_in_cutscene_t > items_in_cutscenes;       // W 0x03A8, L|A 0x0300 (W 0x28, L|A 0x20)
        char pad_W_0x03D0__LA_0x0320[ 56 ];                               // W 0x03D0, L|A 0x0320 (W|L|A 0x38)
        array_dyn_t< class area_item_info_t > area_item_infos_1;          // W 0x0408, L|A 0x0358 (W 0x28, L|A 0x20)
        array_dyn_t< class area_item_info_t > area_item_infos_2;          // W 0x0430, L|A 0x0378 (W 0x28, L|A 0x20)
        char pad_W_0x0458__LA_0x0398[ 24 ];                               // W 0x0458, L|A 0x0398 (W|L|A 0x18)
        array_dyn_t< class sign_item_info_t* > sign_item_infos;           // W 0x0470, L|A 0x03B0 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_gate_t* > traffic_gates;               // W 0x0498, L|A 0x03D0 (W 0x28, L|A 0x20)
        char pad_W_0x04C0__LA_0x03F0[ 16 ];                               // W 0x04C0, L|A 0x03F0 (W|L|A 0x10)
        array_dyn_t< class kdop_item_t* > kdop_items_3;                   // W 0x04D0, L|A 0x0400 (W 0x28, L|A 0x20)
        char pad_W_0x04F8__LA_0x0420[ 16 ];                               // W 0x04F8, L|A 0x0420 (W|L|A 0x10)
        array_dyn_t< class kdop_item_t* > kdop_items_4;                   // W 0x0508, L|A 0x0430 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_origin_t* > traffic_origins;           // W 0x0530, L|A 0x0450 (W 0x28, L|A 0x20)
        char pad_W_0x0558__LA_0x0470[ 16 ];                               // W 0x0558, L|A 0x0470 (W|L|A 0x10)
        array_dyn_t< class traffic_spawn_point_t* > traffic_spawn_points; // W 0x0568, L|A 0x0480 (W 0x28, L|A 0x20)
        char pad_W_0x0590__LA_0x04A0[ 24 ];                               // W 0x0590, L|A 0x04A0 (W|L|A 0x18)
        class city_data_u* city_data;                                     // W 0x05A8, L|A 0x04B8 (W|L|A 0x08)
        array_dyn_t< uint32_t > N00001DA9;                                // W 0x05B0, L|A 0x04C0 (W 0x28, L|A 0x20)
        char pad_W_0x05D8__LA_0x04E0[ 8 ];                                // W 0x05D8, L|A 0x04E0 (W|L|A 0x08)
        array_dyn_t< class traffic_object_t* > traffic_objects_2;         // W 0x05E0, L|A 0x04E8 (W 0x28, L|A 0x20)
        array_dyn_t< class traffic_object_t* > traffic_objects_3;         // W 0x0608, L|A 0x0508 (W 0x28, L|A 0x20)
        array_dyn_t< void* > N00001DB6;                                   // W 0x0630, L|A 0x0528 (W 0x28, L|A 0x20)
        char pad_W_0x0658__LA_0x0548[ 8 ];                                // W 0x0658, L|A 0x0548 (W|L|A 0x08)
        map_tiny_t< class traffic_rule_data_u*, class traffic_rule_data_u* > N00001DBC; // W 0x0660, L|A 0x0550 (W
                                                                                        // 0x60, L|A 0x48)
        char pad_W_0x06C0__LA_0x0598[ 8 ];                           // W 0x06C0, L|A 0x0598 (W|L|A 0x08)
        array_dyn_t< class remote_clients_jip_context_t > N00001DC5; // W 0x06C8, L|A 0x05A0 (W 0x28, L|A 0x20)
        char pad_W_0x06F0__LA_0x05C0[ 104 ];                         // W 0x06F0, L|A 0x05C0 (W|L|A 0x68)
        string_dyn_t N00001DD6;                                      // W 0x0758, L|A 0x0628 (W 0x20, L|A 0x18)
        map_hashed_t< class traffic_segment_t*, void* > N0000EDEA;    // W 0x0778, L|A 0x0640 (W 0x80, L|A 0x60)
        char pad_W_0x07F8__LA_0x06A0[ 48 ];                          // W 0x07F8, L|A 0x06A0 (W|L|A 0x30)
    };

#if defined( _WIN32 )
    static_assert( sizeof( traffic_u ) == 0x828 ); // W
#elif defined( __linux__ ) || defined( __APPLE__ )
    static_assert( sizeof( traffic_u ) == 0x6D0 ); // L|A
#endif

#pragma pack( pop )
}
