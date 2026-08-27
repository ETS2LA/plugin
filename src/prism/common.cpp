#include "common.hpp"

#include <cmath>

namespace ets2la_plugin
{
    float3_t prism::placement_t::to_global_position() const
    {
        return float3_t{ this->cx * 512 + this->pos.x, this->pos.y, this->cz * 512 + this->pos.z };
    }

    float prism::placement_t::get_distance_to( const float3_t dest ) const
    {
        const auto position = this->to_global_position();
        const float dx      = position.x - dest.x;
        const float dy      = position.y - dest.y;
        const float dz      = position.z - dest.z;
        return std::sqrt( dx * dx + dy * dy + dz * dz );
    }

    float3_t float3_t::rotate( const quat_t& rotation ) const
    {
        float x2 = rotation.x + rotation.x;
        float y2 = rotation.y + rotation.y;
        float z2 = rotation.z + rotation.z;

        float wx2 = rotation.w * x2;
        float wy2 = rotation.w * y2;
        float wz2 = rotation.w * z2;
        float xx2 = rotation.x * x2;
        float xy2 = rotation.x * y2;
        float xz2 = rotation.x * z2;
        float yy2 = rotation.y * y2;
        float yz2 = rotation.y * z2;
        float zz2 = rotation.z * z2;

        return {
            this->x * ( 1.0f - yy2 - zz2 ) + this->y * ( xy2 - wz2 ) + this->z * ( xz2 + wy2 ),
            this->x * ( xy2 + wz2 ) + this->y * ( 1.0f - xx2 - zz2 ) + this->z * ( yz2 - wx2 ),
            this->x * ( xz2 - wy2 ) + this->y * ( yz2 + wx2 ) + this->z * ( 1.0f - xx2 - yy2 ),
        };
    }

    float3_t float3_t::operator+( const float3_t& b ) const
    {
        return {
            this->x + b.x,
            this->y + b.y,
            this->z + b.z,
        };
    }
    float3_t float3_t::operator-( const float3_t& b ) const
    {
        return {
            this->x - b.x,
            this->y - b.y,
            this->z - b.z,
        };
    }

    quat_t quat_t::conjugate() const
    {
        return {
            this->w,
            -this->x,
            -this->y,
            -this->z,
        };
    }
}
