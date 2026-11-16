#include "base_ctrl.hpp"

#include "core.hpp"
#include "memory/memory_utils.hpp"
#include "patterns.hpp"
#include "prism/management/item/kdop.hpp"
#include "prism/management/item/kdop_item.hpp"
#include "prism/physics/physics_scene_physx.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

#if defined( _MSC_VER ) && defined( _WIN32 )
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace ets2la_plugin::prism
{
    namespace
    {
#if defined( _MSC_VER ) && defined( _WIN32 )
        void safe_call_ensure_colliders(
            base_ctrl_u::ensure_colliders_fn_t* fn,
            base_ctrl_u* base_ctrl,
            const kdop_t* box,
            bool filter_type_mask
        )
        {
            __try
            {
                fn( base_ctrl, box, filter_type_mask );
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
            }
        }

        void safe_call_item_ensure_physics( kdop_item_t* item )
        {
            __try
            {
                const auto vtable = *reinterpret_cast< uint64_t* >( item );
                if ( vtable != 0 )
                {
                    using ensure_physics_vfn_t = void( kdop_item_t* );
                    auto* vfn = memory::get_vtable_function< ensure_physics_vfn_t >( vtable, 0x1B8 / 8 );
                    if ( vfn != nullptr )
                    {
                        vfn( item );
                    }
                }
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
            }
        }
#endif
    }

    uint64_t base_ctrl_u::instance_ptr_address                         = 0;
    uint32_t base_ctrl_u::game_actor_offset                            = 0;
    uint32_t base_ctrl_u::some_nearby_kdop_items_offset                = 0;
    uint32_t base_ctrl_u::some_nearby_non_ai_vehicles_offset           = 0;
    uint32_t base_ctrl_u::physics_scene_1_offset                       = 0;
    uint32_t base_ctrl_u::physics_scene_2_offset                       = 0;
    base_ctrl_u::ensure_colliders_fn_t* base_ctrl_u::ensure_colliders_fn = nullptr;

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
#ifdef _WIN32
            base_ctrl_u::physics_scene_1_offset = 0x2da0;
            base_ctrl_u::physics_scene_2_offset = 0x2da8;
#else
            base_ctrl_u::physics_scene_1_offset = 0x23e8;
            base_ctrl_u::physics_scene_2_offset = 0x23f0;
#endif
        }

        if ( patterns::base_ctrl::ensure_colliders::pattern[ 0 ] != '\0' )
        {
            base_ctrl_u::ensure_colliders_fn = memory::get_function_from_pattern< ensure_colliders_fn_t >(
                patterns::base_ctrl::ensure_colliders::pattern, patterns::base_ctrl::ensure_colliders::offset
            );
            if ( base_ctrl_u::ensure_colliders_fn != nullptr )
            {
                CCore::g_instance->debug(
                    "Found base_ctrl_u::ensure_colliders_fn @ +{:x}",
                    memory::as_offset( reinterpret_cast< uint64_t >( base_ctrl_u::ensure_colliders_fn ) )
                );
            }
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

#ifdef _WIN32
        constexpr uint32_t defaultSceneOffset = 0x2da0;
#else
        constexpr uint32_t defaultSceneOffset = 0x23e8;
#endif
        uint32_t sceneOff = base_ctrl_u::physics_scene_1_offset ? base_ctrl_u::physics_scene_1_offset : defaultSceneOffset;
        return *reinterpret_cast< physics_scene_physx_t** >( reinterpret_cast< uint64_t >( this ) + sceneOff );
    }

    physics_scene_physx_t* base_ctrl_u::get_physics_scene_2() const
    {
        if ( !this )
        {
            return nullptr;
        }

#ifdef _WIN32
        constexpr uint32_t defaultSceneOffset = 0x2da8;
#else
        constexpr uint32_t defaultSceneOffset = 0x23f0;
#endif
        uint32_t sceneOff = base_ctrl_u::physics_scene_2_offset ? base_ctrl_u::physics_scene_2_offset : defaultSceneOffset;
        return *reinterpret_cast< physics_scene_physx_t** >( reinterpret_cast< uint64_t >( this ) + sceneOff );
    }

    void base_ctrl_u::ensure_colliders_in_oriented_box(
        const float3_t& global_pos,
        const quat_t& rot,
        float forward_dist,
        float backward_dist,
        float side_dist,
        float vertical_dist
    )
    {
        if ( !this )
        {
            return;
        }

        // In ETS2 local vehicle coordinates: -Z is forward, +Z is backward, +/-X is right/left.
        const float3_t local_corners[ 4 ] = {
            { -side_dist, 0.0f, -forward_dist },
            {  side_dist, 0.0f, -forward_dist },
            { -side_dist, 0.0f,  backward_dist },
            {  side_dist, 0.0f,  backward_dist },
        };

        kdop_t query_box{};
        query_box.minimums[ 0 ] =  1e30f;
        query_box.minimums[ 1 ] = global_pos.y - vertical_dist;
        query_box.minimums[ 2 ] =  1e30f;
        query_box.minimums[ 3 ] =  1e30f;
        query_box.minimums[ 4 ] =  1e30f;

        query_box.maximums[ 0 ] = -1e30f;
        query_box.maximums[ 1 ] = global_pos.y + vertical_dist;
        query_box.maximums[ 2 ] = -1e30f;
        query_box.maximums[ 3 ] = -1e30f;
        query_box.maximums[ 4 ] = -1e30f;

        for ( const auto& lc : local_corners )
        {
            const float3_t wc = global_pos + lc.rotate( rot );
            const float d3    = ( wc.x + wc.z ) * 0.5f;
            const float d4    = ( wc.x - wc.z ) * 0.5f;

            query_box.minimums[ 0 ] = ( std::min )( query_box.minimums[ 0 ], wc.x );
            query_box.maximums[ 0 ] = ( std::max )( query_box.maximums[ 0 ], wc.x );

            query_box.minimums[ 2 ] = ( std::min )( query_box.minimums[ 2 ], wc.z );
            query_box.maximums[ 2 ] = ( std::max )( query_box.maximums[ 2 ], wc.z );

            query_box.minimums[ 3 ] = ( std::min )( query_box.minimums[ 3 ], d3 );
            query_box.maximums[ 3 ] = ( std::max )( query_box.maximums[ 3 ], d3 );

            query_box.minimums[ 4 ] = ( std::min )( query_box.minimums[ 4 ], d4 );
            query_box.maximums[ 4 ] = ( std::max )( query_box.maximums[ 4 ], d4 );
        }

        if ( base_ctrl_u::ensure_colliders_fn != nullptr )
        {
#if defined( _MSC_VER ) && defined( _WIN32 )
            safe_call_ensure_colliders( base_ctrl_u::ensure_colliders_fn, this, &query_box, true );
#else
            base_ctrl_u::ensure_colliders_fn( this, &query_box, true );
#endif
            return;
        }

        // Fallback: iterate over nearby_kdop_items if the direct KDOP tree query function pattern was not found
        auto* nearby_items = this->get_nearby_kdop_items();
        if ( nearby_items == nullptr )
        {
            return;
        }

        for ( auto* item : *nearby_items )
        {
            if ( item == nullptr )
            {
                continue;
            }

            if ( item->kdop.maximums[ 0 ] < query_box.minimums[ 0 ] || item->kdop.minimums[ 0 ] > query_box.maximums[ 0 ] ||
                 item->kdop.maximums[ 1 ] < query_box.minimums[ 1 ] || item->kdop.minimums[ 1 ] > query_box.maximums[ 1 ] ||
                 item->kdop.maximums[ 2 ] < query_box.minimums[ 2 ] || item->kdop.minimums[ 2 ] > query_box.maximums[ 2 ] ||
                 item->kdop.maximums[ 3 ] < query_box.minimums[ 3 ] || item->kdop.minimums[ 3 ] > query_box.maximums[ 3 ] ||
                 item->kdop.maximums[ 4 ] < query_box.minimums[ 4 ] || item->kdop.minimums[ 4 ] > query_box.maximums[ 4 ] )
            {
                continue;
            }

#if defined( _MSC_VER ) && defined( _WIN32 )
            safe_call_item_ensure_physics( item );
#endif
        }
    }
}
