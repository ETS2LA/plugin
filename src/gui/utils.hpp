#pragma once

struct ImVec2;

namespace ets2la_plugin
{
    struct float2_t;
    struct float3_t;

    namespace prism
    {
        class core_camera_u;
    }

    namespace drawing
    {
        ImVec2 world_to_window( const float3_t& position, const float2_t& window_center, const float scale );
        float3_t world_to_screen( const prism::core_camera_u* cam, const float3_t& pos );
    }
}
