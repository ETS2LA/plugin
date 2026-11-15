#include "physics_scene_physx.hpp"

#include "core.hpp"
#include "memory/memory_utils.hpp"
#include "patterns.hpp"
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ets2la_plugin::prism
{
#ifdef _WIN32
    uint32_t physics_scene_physx_t::px_scene_offset           = 0x0178;
    uint32_t physics_scene_physx_t::raycast_vtable_offset     = 0x02F0;
    uint32_t physics_scene_physx_t::lock_read_vtable_offset   = 0x0348;
    uint32_t physics_scene_physx_t::unlock_read_vtable_offset = 0x0350;
#else
    uint32_t physics_scene_physx_t::px_scene_offset           = 0x0160;
    uint32_t physics_scene_physx_t::raycast_vtable_offset     = 0x02F8;
    uint32_t physics_scene_physx_t::lock_read_vtable_offset   = 0x0350;
    uint32_t physics_scene_physx_t::unlock_read_vtable_offset = 0x0358;
#endif

    bool physics_scene_physx_t::scan_patterns()
    {
        const auto pxSceneAddr = memory::get_address_for_pattern(
            patterns::physics_scene_physx::px_scene::pattern,
            patterns::physics_scene_physx::px_scene::offset
        );

        if ( pxSceneAddr != 0 )
        {
            px_scene_offset = *reinterpret_cast< const uint32_t* >( pxSceneAddr );
            CCore::g_instance->debug( "Found physics_scene_physx px_scene_offset {:x}", px_scene_offset );
        }

        const auto raycastAddr = memory::get_address_for_pattern(
            patterns::physics_scene_physx::raycast::pattern,
            patterns::physics_scene_physx::raycast::offset
        );

        if ( raycastAddr != 0 )
        {
            raycast_vtable_offset = *reinterpret_cast< const uint32_t* >( raycastAddr );
            CCore::g_instance->debug( "Found PxScene raycast_vtable_offset {:x}", raycast_vtable_offset );
        }

        return true;
    }

    physx::PxScene* physics_scene_physx_t::get_px_scene() const
    {
        if ( this == nullptr || px_scene_offset == 0 )
        {
            return nullptr;
        }

        const auto thisAddr = reinterpret_cast< uintptr_t >( this );
        if ( thisAddr < 0x10000 || thisAddr > 0x7FFFFFFFFFFF )
        {
            return nullptr;
        }

#if defined(_MSC_VER) && defined(_WIN32)
        __try
        {
            const auto vtable = *reinterpret_cast< const uintptr_t* >( this );
            if ( vtable < 0x10000 || vtable > 0x7FFFFFFFFFFF )
            {
                return nullptr;
            }

            auto* const pxPtr = *reinterpret_cast< physx::PxScene* const* >(
                reinterpret_cast< const char* >( this ) + px_scene_offset
            );

            const auto pxAddr = reinterpret_cast< uintptr_t >( pxPtr );
            if ( pxAddr < 0x10000 || pxAddr > 0x7FFFFFFFFFFF )
            {
                return nullptr;
            }

            const auto pxVtable = *reinterpret_cast< const uintptr_t* >( pxPtr );
            if ( pxVtable < 0x10000 || pxVtable > 0x7FFFFFFFFFFF )
            {
                return nullptr;
            }

            return pxPtr;
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            return nullptr;
        }
#else
        auto* const pxPtr = *reinterpret_cast< physx::PxScene* const* >(
            reinterpret_cast< const char* >( this ) + px_scene_offset
        );

        const auto pxAddr = reinterpret_cast< uintptr_t >( pxPtr );
        if ( pxAddr < 0x10000 || pxAddr > 0x7FFFFFFFFFFF )
        {
            return nullptr;
        }

        return pxPtr;
#endif
    }

    physx::PxVec3 physics_scene_physx_t::to_physx_pos( const placement_t& placement ) const
    {
        const float dx = static_cast< float >( placement.cx - scene_sector_cx ) * 512.0f
                       + ( placement.pos.x - scene_origin.x );
        const float dy = placement.pos.y - scene_origin.y;
        const float dz = static_cast< float >( placement.cz - scene_sector_cz ) * 512.0f
                       + ( placement.pos.z - scene_origin.z );

        return physx::PxVec3( dx, dy, dz );
    }

    placement_t physics_scene_physx_t::to_placement( const physx::PxVec3& pxPos ) const
    {
        placement_t placement{};
        const float worldX = pxPos.x + scene_origin.x;
        const float worldZ = pxPos.z + scene_origin.z;

        const auto sectorX = static_cast< int32_t >( std::floor( worldX / 512.0f ) );
        const auto sectorZ = static_cast< int32_t >( std::floor( worldZ / 512.0f ) );

        placement.cx = static_cast< int16_t >( scene_sector_cx + sectorX );
        placement.cz = static_cast< int16_t >( scene_sector_cz + sectorZ );
        placement.pos.x = worldX - static_cast< float >( sectorX ) * 512.0f;
        placement.pos.y = pxPos.y + scene_origin.y;
        placement.pos.z = worldZ - static_cast< float >( sectorZ ) * 512.0f;

        return placement;
    }
}
