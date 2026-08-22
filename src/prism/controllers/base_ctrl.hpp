#pragma once
#include "prism/collections/array_dyn.hpp"
#include "prism/collections/list_dyn.hpp"
#include "prism/unit/unit.hpp"

namespace ets2la_plugin::prism
{
#pragma pack( push, 1 )

    // commented out because maintaining this is too annoying now, we use patterns to find the values we want anyway
    // Size: 0x3088 1.58
    class base_ctrl_u : public /* [0x10] @ 0x00 */ unit_t
    {
        //     class item_reg_t;

    public:
        //     array_dyn_t< class kdop_item_t* > map_kdop_items;                            // 0x0010 (0x20)
        //     array_dyn_t< class node_item_t* > map_nodes;                                 // 0x0030 (0x20)
        //     char pad_0050[ 16 ];                                                         // 0x0050 (0x10)
        //     class game_data_u* game_data;                                                // 0x0060 (0x08)
        //     class game_traffic_u* game_traffic;                                          // 0x0068 (0x08)
        //     class remote_vehicle_ctrl_t* remote_vehicle_ctrl;                            // 0x0070 (0x08)
        //     class core_interior_glass_config_u* interior_glass_config_rain;              // 0x0078 (0x08)
        //     class core_interior_glass_config_u* interior_glass_config_snow;              // 0x0080 (0x08)
        //     class map_data_u* map_data;                                                  // 0x0088 (0x08)
        //     class mirror_data_u* mirror_data;                                            // 0x0090 (0x08)
        //     array_dyn_t< class road_sing_area_u* > road_sing_areas;                      // 0x0098 (0x20)
        //     char pad_00B8[ 8 ];                                                          // 0x00B8 (0x08)
        //     string_dyn_t timezone;                                                       // 0x00C0 (0x18)
        //     char pad_00D8[ 344 ];                                                        // 0x00D8 (0x158)
        //     array_dyn_t< class services_item_t* > services_items;                        // 0x0230 (0x20)
        //     array_dyn_t< class garage_item_t* > garage_items;                            // 0x0250 (0x20)
        //     char pad_0270[ 24 ];                                                         // 0x0270 (0x18)
        //     array_dyn_t< class core_camera_u* > N000000C0;                               // 0x0288 (0x20)
        //     array_dyn_t< class trailer_u* > N000000C4;                                   // 0x02A8 (0x20)
        //     array_dyn_t< class trailer_def_u* > N000000C8;                               // 0x02C8 (0x20)
        //     array_dyn_t< class traffic_vehicle_type_u* > N000000CC;                      // 0x02E8 (0x20)
        //     array_dyn_t< class traffic_trailer_type_u* > N000000D0;                      // 0x0308 (0x20)
        //     array_dyn_t< class accessory_hookup_data_u* > N000000D4;                     // 0x0328 (0x20)
        //     array_dyn_t< class core_camera_u* > cameras;                                 // 0x0348 (0x20)
        //     array_dyn_t< class core_vehicle_u* > trailers;                               // 0x0368 (0x20)
        //     array_dyn_t< class trailer_def_u* > unk_trailer_defs;                        // 0x0388 (0x20)
        //     array_dyn_t< class traffic_vehicle_type_u* > traffic_vehicle_types;          // 0x03A8 (0x20)
        //     array_dyn_t< class traffic_trailer_type_u* > traffic_trailer_types;          // 0x03C8 (0x20)
        //     array_dyn_t< class accessory_hookup_data_u* > accessory_hookup_datas;        // 0x03E8 (0x20)
        //     class physics_data_u* physics_data;                                          // 0x0408 (0x08)
        //     class physics_interior_camera_data_u* physics_interior_camera_data;          // 0x0410 (0x08)
        //     class force_feedback_data_u* force_feedback_data;                            // 0x0418 (0x08)
        //     class vehicle_damage_data_u* vehicle_damage_data;                            // 0x0420 (0x08)
        //     array_dyn_t< class trailer_category_def_u* > N000000F4;                      // 0x0428 (0x20)
        //     char pad_0448[ 40 ];                                                         // 0x0448 (0x28)
        //     array_dyn_t< class token_t > N000000FD;                                      // 0x0470 (0x20)
        //     char pad_0490[ 24 ];                                                         // 0x0490 (0x18)
        //     class vehicle_service_data_u* vehicle_service_data;                          // 0x04A8 (0x08)
        //     class license_plate_manager_u* license_plate_manager;                        // 0x04B0 (0x08)
        //     class driver_plate_manager_u* driver_plate_manager;                          // 0x04B8 (0x08)
        //     class player_tag_manager_u* player_tag_manager;                              // 0x04C0 (0x08)
        //     class traffic_sign_manager_u* traffic_sign_manager;                          // 0x04C8 (0x08)
        //     char pad_04D0[ 16 ];                                                         // 0x04D0 (0x10)
        //     array_dyn_t< class kdop_item_t* > kdop_items_unk;                            // 0x04E0 (0x20)
        //     array_dyn_t< class kdop_item_t* > kdop_items_unk2;                           // 0x0500 (0x20)
        //     array_dyn_t< class base_ctrl_u::item_reg_t > N00000116;                      // 0x0520 (0x20)
        //     array_dyn_t< class kdop_item_t* > some_nearby_kdop_items__dont_use;          // 0x0540 (0x20) I think
        //     array_dyn_t< class base_ctrl_u::item_reg_t > N0000011E;                      // 0x0560 (0x20)
        //     char pad_0580[ 88 ];                                                         // 0x0580 (0x58)
        //     list_dyn_t< class kdop_item_t* > N0000832F;                                  // 0x05D8 (0x58)
        //     list_dyn_t< class city_item_t* > N00000138;                                  // 0x0630 (0x58)
        //     list_dyn_t< class model_hookup_u* > N00000143;                               // 0x0688 (0x58)
        //     char pad_06E0[ 136 ];                                                        // 0x06E0 (0x88)
        //     array_dyn_t< class kdop_item_t* > N0000015F;                                 // 0x0768 (0x20)
        //     array_dyn_t< class kdop_item_t* > N00000163;                                 // 0x0788 (0x20)
        //     char pad_07A8[ 16 ];                                                         // 0x07A8 (0x10)
        //     class trigger_manager_u* trigger_manager_u;                                  // 0x07B8 (0x08)
        //     class camera_manager_u* camera_manager_u;                                    // 0x07C0 (0x08)
        //     class cutscene_manager_u* cutscene_manager_u;                                // 0x07C8 (0x08)
        //     class journey_events_manager_u* journey_events_manager_u;                    // 0x07D0 (0x08)
        //     char pad_07D8[ 16 ];                                                         // 0x07D8 (0x10)
        //     class core_camera_u* core_camera_u;                                          // 0x07E8 (0x08)
        //     class environment_u* environment_u;                                          // 0x07F0 (0x08)
        //     class oversize_manager_u* oversize_manager_u;                                // 0x07F8 (0x08)
        //     char pad_0800[ 72 ];                                                         // 0x0800 (0x48)
        //     class eye_state_u* eye_state_u;                                              // 0x0848 (0x08)
        //     char pad_0850[ 8 ];                                                          // 0x0850 (0x08)
        //     array_dyn_t< class navigation_ferry_data_t > navigation_ferry_data;          // 0x0858 (0x20)
        //     char pad_0878[ 8 ];                                                          // 0x0878 (0x08)
        //     array_dyn_t< class token_t > N00000182;                                      // 0x0880 (0x20)
        //     char pad_08A0[ 8392 ];                                                       // 0x08A0 (0x20c8)
        //     class local_gameplay_mode_u* local_gameplay_mode_u;                          // 0x2968 (0x08)
        //     char pad_2970[ 32 ];                                                         // 0x2970 (0x20)
        //     list_dyn_t< class vehicle_shared_u* > N000005A5;                             // 0x2990 (0x58)
        //     char pad_29E8[ 240 ];                                                        // 0x29E8 (0xf0)
        //     array_dyn_t< class kdop_item_t* > N000005CE;                                 // 0x2AD8 (0x20)
        //     char pad_2AF8[ 192 ];                                                        // 0x2AF8 (0xc0)
        //     list_dyn_t< class vehicle_shared_u* > some_nearby_non_ai_vehicles__dont_use; // 0x2BB8 (0x58)
        //     class sector_manager_u* sector_manager;                                      // 0x2C10 (0x08)
        //     list_dyn_t< class sprite_queue_t* > N000005F6;                               // 0x2C18 (0x58)
        //     char pad_2C70[ 88 ];                                                         // 0x2C70 (0x58)
        //     char pad_2CC8[ 48 ];                                                         // 0x2CC8 (0x30)
        //     array_dyn_t< class segment_t* > N00000607;                                   // 0x2CF8 (0x20)
        //     array_dyn_t< class kdop_item_t* > N0000060B;                                 // 0x2D18 (0x20)
        //     array_dyn_t< class kdop_item_t* > N0000060F;                                 // 0x2D38 (0x20)
        //     array_dyn_t< class kdop_item_t* > N00000613;                                 // 0x2D58 (0x20)
        //     char pad_2D78[ 8 ];                                                          // 0x2D78 (0x08)
        //     class asset_catalog_u* asset_catalog;                                        // 0x2D80 (0x08)
        //     char pad_2D88[ 96 ];                                                         // 0x2D88 (0x60)
        //     char pad_2DE8[ 272 ];                                                        // 0x2DE8 (0x110)
        //     array_dyn_t< class shadowmap_t > N00000647;                                  // 0x2EF8 (0x20)
        //     char pad_2F18[ 24 ];                                                         // 0x2F18 (0x18)
        //     class game_actor_u* game_actor__dont_use;                                    // 0x2F30 (0x08)
        //     char pad_2F38[ 336 ];                                                        // 0x2F38 (0x150)

        static uint64_t instance_ptr_address;
        static uint32_t game_actor_offset;
        static uint32_t some_nearby_kdop_items_offset;

        static bool scan_patterns();
        static base_ctrl_u* get();

        array_dyn_t< class kdop_item_t* >* get_nearby_kdop_items() const;
    };
    // static_assert( sizeof( base_ctrl_u ) == 0x3088 );

#pragma pack( pop )
}
