#pragma once

#include "prism/collections/array_dyn.hpp"
#include "prism/common.hpp"
#include "physx_types.hpp"
#include <cstdint>

namespace ets2la_plugin::prism
{
#pragma pack( push, 1 )

    class physics_scene_physx_t
    {
    public:
        void* vptr;
        char pad_0008[ 0x0108 ];
        float3_t scene_origin;
        int16_t scene_sector_cx;
        int16_t scene_sector_cz;
        char pad_0120[ 0x0058 ];
        physx::PxScene* px_scene;

        static uint32_t px_scene_offset;
        static uint32_t raycast_vtable_offset;
        static uint32_t lock_read_vtable_offset;
        static uint32_t unlock_read_vtable_offset;

        static bool scan_patterns();

        physx::PxScene* get_px_scene() const;
        physx::PxVec3 to_physx_pos( const placement_t& placement ) const;
        placement_t to_placement( const physx::PxVec3& pxPos ) const;
    };

#pragma pack( pop )
}
