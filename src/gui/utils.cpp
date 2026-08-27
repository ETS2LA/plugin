#include "utils.hpp"

#include "imgui.h"
#include "prism/camera/core_camera.hpp"
#include "prism/common.hpp"
#include <cmath>

namespace ets2la_plugin::drawing
{
    ImVec2 world_to_window( const float3_t& position, const float2_t& window_center, const float ui_scale )
    {
        return { window_center.x + position.x * ui_scale, window_center.y + position.z * ui_scale };
    }

    float3_t world_to_screen( const prism::core_camera_u* cam, const float3_t& pos )
    {
        const auto io = ImGui::GetIO();
        auto p        = pos - cam->placement.to_global_position();
        p             = p.rotate( cam->placement.rot.conjugate() );

        float f = 1.f / tanf( cam->v_fov * ( acosf( -1 ) * 2 ) / 360 * 0.5f );
        f /= p.z;
        p.x *= f / ( io.DisplaySize.x / io.DisplaySize.y );
        p.y *= f;

        p.x = p.x * 0.5f + 0.5f;
        p.y = p.y * 0.5f + 0.5f;

        p.x *= io.DisplaySize.x;
        p.y *= io.DisplaySize.y;

        return float3_t{ io.DisplaySize.x - p.x, p.y, p.z };
    }
}
