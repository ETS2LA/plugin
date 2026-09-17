#pragma once

namespace ets2la_plugin::patterns
{
    namespace base_ctrl
    {
        inline constexpr auto pattern           = "48 8b ? ? ? ? ? 48 8b ? 48 8b ? 48 8b 80";
        inline constexpr auto offset_instance   = 3;
        inline constexpr auto offset_game_actor = 16;

        namespace nearby_non_ai_vehicles
        {
            inline constexpr auto pattern = "f0 0f c1 42 ? 48 8d b9 ? ? ? ? 48 8b 07";
            inline constexpr auto offset  = 8;
        }

        namespace nearby_kdop_items
        {
            inline constexpr auto pattern = "4c 8d 81 ? ? ? ? 0f 29 ? ? 48 8d 91";
            inline constexpr auto offset  = 3;
        }
    }

    namespace game_ctrl
    {
        namespace gps_manager
        {
            inline constexpr auto pattern = "48 8d 88 ? ? ? ? 48 85 c9 74 ? 48 8b 01 48 8d 54";
            inline constexpr auto offset  = 3;
        }
    }

    namespace camera_manager
    {
        inline constexpr auto pattern         = "48 8b 05 ? ? ? ? 48 8b fa 48 85 c0 0f";
        inline constexpr auto offset_instance = 3;
    }

    namespace game_traffic
    {
        inline constexpr auto pattern         = "48 8b d9 48 8b 0d ? ? ? ? 48 85 c9 74 ? 48 8b 83 ? ? ? ? 48 85 c0";
        inline constexpr auto offset_instance = 6;
    }

    namespace vehicle_shared
    {
        namespace steering
        {
            inline constexpr auto pattern = "eb ? f3 0f 10 ? ? ? ? ? f3 0f 5c ? ? ? ? ? 0f 2f d3";
            inline constexpr auto offset  = 6;
        }
    }

    namespace game_physics_vehicle
    {
        namespace trailer
        {
            inline constexpr auto pattern = "4c 8b 71 ? 49 8b 8e ? ? ? ? 48 85 c9";
            inline constexpr auto offset  = 7;
        }
    }

    namespace game_trailer_actor
    {
        namespace slave_trailer
        {
            inline constexpr auto pattern = "48 8b 86 ? ? ? ? 48 85 c0 74 ? 48 8b fe 48 8b f0";
            inline constexpr auto offset  = 3;
        }
    }

    namespace inp_get_mix
    {
        inline constexpr auto hash_table_pattern = "48 8b 05 ? ? ? ? 48 8d 55 ? 48 8d 0d ? ? ? ? 48 c7 45 d0 0d";
        inline constexpr auto hash_table_offset  = 3;

        inline constexpr auto vfunc_value_pattern = "ff 50 ? 48 85 c0 74 ? 48 8b 18 f3 0f 10 8b";
        inline constexpr auto vfunc_offset        = 2;
        inline constexpr auto value_offset        = 15;
    }
}
