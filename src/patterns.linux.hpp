#pragma once

namespace ets2la_plugin::patterns
{

    namespace base_ctrl
    {
        inline constexpr auto pattern           = "48 8b 05 ? ? ? ? 48 8b 7f ? 48 8b 80 ? ? ? ? 4c";
        inline constexpr auto offset_instance   = 3;
        inline constexpr auto offset_game_actor = 14;

        namespace nearby_non_ai_vehicles
        {
            inline constexpr auto pattern = "48 8d bb ? ? ? ? e8 ? ? ? ? 48 8b bb ? ? ? ? e8 ? ? ? ? 48 8b bd";
            inline constexpr auto offset  = 3;
        }

        namespace nearby_kdop_items
        {
            inline constexpr auto pattern = "48 8d b7 ? ? ? ? 48 8d 97 ? ? ? ? 48 89 e5";
            inline constexpr auto offset  = 10;
        }

    }

    namespace game_ctrl
    {
        namespace gps_manager
        {
            inline constexpr auto pattern = "48 8d 55 ? be 02 00 00 00 48 8d b8";
            inline constexpr auto offset  = 12;
        }
    }

    namespace camera_manager
    {
        inline constexpr auto pattern         = "48 8b 1d ? ? ? ? 8b 43 ? 85 c0";
        inline constexpr auto offset_instance = 3;
    }

    namespace game_traffic
    {
        inline constexpr auto pattern         = "48 8b 0d ? ? ? ? 48 85 f6 4c 8b 81";
        inline constexpr auto offset_instance = 3;
    }

    namespace vehicle_shared
    {
        namespace steering
        {
            inline constexpr auto pattern = "f3 0f 11 83 ? ? ? ? 48 8b bb ? ? ? ? 48 85 ff 74 ? e8";
            inline constexpr auto offset  = 4;
        }
    }

    namespace game_physics_vehicle
    {
        namespace trailer
        {
            inline constexpr auto pattern = "48 8b 83 ? ? ? ? 66 0f ef c0 48 85 c0 74 ? f3 0f 10 80 ? ? ? ? f3";
            inline constexpr auto offset  = 3;
        }
    }

    namespace game_trailer_actor
    {
        namespace slave_trailer
        {
            inline constexpr auto pattern = "72 ? 49 8b bc 24 ? ? ? ? 48 85 ff 74 ? 48 8b 07 48 8b 80";
            inline constexpr auto offset  = 6;
        }
    }

    namespace inp_get_mix
    {
        inline constexpr auto pattern      = "e8 ? ? ? ? f3 0f 10 ? ? ? ? ? f3 0f 10 88";
        inline constexpr auto fn_offset    = 1;
        inline constexpr auto value_offset = 17;
    }
}
