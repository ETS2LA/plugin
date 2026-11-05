#pragma once

#include "./traffic.hpp"

namespace ets2la_plugin::prism
{
#pragma pack( push, 1 )

    // Size: W 0x0850, L|A 0x06F0 1.62
    class game_traffic_u : public /* [W 0x828, L|A 0x6D0] @ W|L|A 0x00 */ traffic_u
    {
    public:
        array_dyn_t< class traffic_world_rule_t* > traffic_world_rules_2; // W 0x07F0, L|A 0x0698 (W 0x28, L|A 0x20)

        static uint64_t instance_ptr_address;

        static bool scan_patterns();
        static game_traffic_u* get();
    };

#if defined( _WIN32 )
    static_assert( sizeof( game_traffic_u ) == 0x850 ); // W
#elif defined( __linux__ ) || defined( __APPLE__ )
    static_assert( sizeof( game_traffic_u ) == 0x6F0 ); // L|A
#endif

#pragma pack( pop )
}
