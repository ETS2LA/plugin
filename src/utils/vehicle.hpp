#pragma once

#include "prism/common.hpp"

namespace ets2la_plugin::utils::vehicle
{
    // vehicle coordinates are not at the center of the vehicle, so we use the aabox to calculate the center
    inline float3_t get_center_coords( const prism::placement_t& placement, const prism::aabox_t& aabox )
    {
        const auto dimensions = aabox.get_dimensions();

        float3_t offset = {
            aabox.start.x + ( dimensions.width / 2.f ),
            aabox.start.y + ( dimensions.height / 2.f ),
            aabox.start.z + ( dimensions.length / 2.f ),
        };

        return placement.to_global_position() + offset.rotate( placement.rot );
    }

    // vehicle coordinates are not at the center of the vehicle, so we use the aabox to calculate the center
    inline float3_t get_center_coords( const float3_t& position, const quat_t& rotation, const prism::aabox_t& aabox )
    {
        const auto dimensions = aabox.get_dimensions();

        float3_t offset = {
            aabox.start.x + ( dimensions.width / 2.f ),
            aabox.start.y + ( dimensions.height / 2.f ),
            aabox.start.z + ( dimensions.length / 2.f ),
        };

        return position + offset.rotate( rotation );
    }
}
