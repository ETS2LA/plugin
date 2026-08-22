#include "base_ctrl.hpp"

#include "core.hpp"
#include "memory/memory_utils.hpp"
#include "patterns.hpp"
#include <stdexcept>

namespace ets2la_plugin::prism
{

    uint64_t base_ctrl_u::instance_ptr_address               = 0;
    uint32_t base_ctrl_u::game_actor_offset                  = 0;
    uint32_t base_ctrl_u::some_nearby_kdop_items_offset      = 0;

    bool base_ctrl_u::scan_patterns()
    {
        auto addr = memory::get_address_for_pattern( patterns::base_ctrl::pattern );

        if ( addr == 0 )
        {
            throw std::runtime_error( "Failed to find base_ctrl instance pattern" );
        }
        base_ctrl_u::instance_ptr_address =
            memory::get_absolute_address_from_offset( addr, patterns::base_ctrl::offset_instance );
        base_ctrl_u::game_actor_offset =
            *reinterpret_cast< uint32_t* >( addr + patterns::base_ctrl::offset_game_actor );

        CCore::g_instance->debug(
            "Found base_ctrl @ +{:x} and game actor offset {:x}",
            memory::as_offset( base_ctrl_u::instance_ptr_address ),
            base_ctrl_u::game_actor_offset
        );

        addr = memory::get_address_for_pattern(
            patterns::base_ctrl::nearby_kdop_items::pattern, patterns::base_ctrl::nearby_kdop_items::offset
        );

        if ( addr == 0 )
        {
            throw std::runtime_error( "Failed to find base_ctrl::some_nearby_kdop_items pattern" );
        }

        base_ctrl_u::some_nearby_kdop_items_offset = *reinterpret_cast< uint32_t* >( addr );

        CCore::g_instance->debug(
            "Found base_ctrl::some_nearby_kdop_items_offset {:x}", base_ctrl_u::some_nearby_kdop_items_offset
        );

        return true;
    }

    base_ctrl_u* base_ctrl_u::get()
    {
        if ( base_ctrl_u::instance_ptr_address != 0 )
        {
            return *reinterpret_cast< base_ctrl_u** >( base_ctrl_u::instance_ptr_address );
        }
        return nullptr;
    }

    array_dyn_t< class kdop_item_t* >* base_ctrl_u::get_nearby_kdop_items() const
    {
        if ( base_ctrl_u::some_nearby_kdop_items_offset != 0 )
        {
            return reinterpret_cast< array_dyn_t< class kdop_item_t* >* >(
                reinterpret_cast< uint64_t >( this ) + base_ctrl_u::some_nearby_kdop_items_offset
            );
        }
        return nullptr;
    }
}
