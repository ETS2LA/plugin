#include "base_ctrl.hpp"

#include "core.hpp"
#include "memory/memory_utils.hpp"
#include "patterns.hpp"
#include "prism/physics/physics_scene_physx.hpp"
#include <stdexcept>

namespace ets2la_plugin::prism
{

    uint64_t base_ctrl_u::instance_ptr_address               = 0;
    uint32_t base_ctrl_u::game_actor_offset                  = 0;
    uint32_t base_ctrl_u::some_nearby_kdop_items_offset      = 0;
    uint32_t base_ctrl_u::some_nearby_non_ai_vehicles_offset = 0;
    uint32_t base_ctrl_u::physics_scene_1_offset             = 0;
    uint32_t base_ctrl_u::physics_scene_2_offset             = 0;

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

        const auto tmp_vehicles_addr = memory::get_address_for_pattern(
            patterns::base_ctrl::nearby_non_ai_vehicles::pattern, patterns::base_ctrl::nearby_non_ai_vehicles::offset
        );

        if ( tmp_vehicles_addr == 0 )
        {
            throw std::runtime_error( "Failed to find TMP vehicle list offset" );
        }
        base_ctrl_u::some_nearby_non_ai_vehicles_offset = *reinterpret_cast< uint32_t* >( tmp_vehicles_addr );

        CCore::g_instance->debug(
            "Found base_ctrl_u::some_nearby_non_ai_vehicles_offset {:x}",
            base_ctrl_u::some_nearby_non_ai_vehicles_offset
        );

        const auto scenes_addr = memory::get_address_for_pattern(
            patterns::base_ctrl::physics_scenes::pattern_scenes
        );

        if ( scenes_addr != 0 )
        {
            base_ctrl_u::physics_scene_1_offset = *reinterpret_cast< uint32_t* >(
                scenes_addr + patterns::base_ctrl::physics_scenes::offset_scene_1
            );
            base_ctrl_u::physics_scene_2_offset = *reinterpret_cast< uint32_t* >(
                scenes_addr + patterns::base_ctrl::physics_scenes::offset_scene_2
            );
            CCore::g_instance->debug(
                "Found base_ctrl_u physics scenes: scene_1={:x}, scene_2={:x}",
                base_ctrl_u::physics_scene_1_offset,
                base_ctrl_u::physics_scene_2_offset
            );
        }
        else
        {
            CCore::g_instance->warning( "Failed to find physics_scenes pattern, using fallback" );
            base_ctrl_u::physics_scene_1_offset = 0x2da0;
            base_ctrl_u::physics_scene_2_offset = 0x2da8;
        }

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

    // contains TruckerMP nearby players' trucks and trailers
    // also contains trucks and trailers in menus (background/service center/truck dealer/etc...)
    list_dyn_t< class vehicle_shared_u* >* base_ctrl_u::get_some_nearby_non_ai_vehicles_list() const
    {
        if ( base_ctrl_u::some_nearby_non_ai_vehicles_offset != 0 )
        {
            return reinterpret_cast< list_dyn_t< class vehicle_shared_u* >* >(
                reinterpret_cast< uint64_t >( this ) + base_ctrl_u::some_nearby_non_ai_vehicles_offset
            );
        }

        return nullptr;
    }

    physics_scene_physx_t* base_ctrl_u::get_physics_scene_1() const
    {
        if ( !this )
        {
            return nullptr;
        }

        uint32_t sceneOff = base_ctrl_u::physics_scene_1_offset ? base_ctrl_u::physics_scene_1_offset : 0x2da0;
        return *reinterpret_cast< physics_scene_physx_t** >( reinterpret_cast< uint64_t >( this ) + sceneOff );
    }

    physics_scene_physx_t* base_ctrl_u::get_physics_scene_2() const
    {
        if ( !this )
        {
            return nullptr;
        }

        uint32_t sceneOff = base_ctrl_u::physics_scene_2_offset ? base_ctrl_u::physics_scene_2_offset : 0x2da8;
        return *reinterpret_cast< physics_scene_physx_t** >( reinterpret_cast< uint64_t >( this ) + sceneOff );
    }
}
