#pragma once

#include <cstdint>

#pragma pack(push, 1)

namespace ets2la_plugin
{
    struct float2_t // size: 0x0008
    {
        float x; // 0x0000 (0x04)
        float y; // 0x0004 (0x04)
    };

    static_assert(sizeof( float2_t ) == 0x08);

    struct float3_t // size: 0x000c
    {
        float x; // 0x0000 (0x04)
        float y; // 0x0004 (0x04)
        float z; // 0x0008 (0x04)

        float3_t rotate( const struct quat_t& ) const;
        float3_t operator+( const float3_t& b ) const;
    };

    static_assert(sizeof( float3_t ) == 0x0c);

    struct quat_t // size: 0x0010
    {
        float w; // 0x0000 (0x04)
        float x; // 0x0004 (0x04)
        float y; // 0x0008 (0x04)
        float z; // 0x000c (0x04)
    };

    struct float4x4_t // size: 0x0040
    {
        float m11; // 0x0000 (0x04)
        float m12; // 0x0004 (0x04)
        float m13; // 0x0008 (0x04)
        float m14; // 0x000c (0x04)

        float m21; // 0x0010 (0x04)
        float m22; // 0x0014 (0x04)
        float m23; // 0x0018 (0x04)
        float m24; // 0x001c (0x04)

        float m31; // 0x0020 (0x04)
        float m32; // 0x0024 (0x04)
        float m33; // 0x0028 (0x04)
        float m34; // 0x002c (0x04)

        float m41; // 0x0030 (0x04)
        float m42; // 0x0034 (0x04)
        float m43; // 0x0038 (0x04)
        float m44; // 0x003c (0x04)
    };

    static_assert(sizeof( quat_t ) == 0x10);

    struct int3_t // size: 0x000c
    {
        int x; // 0x0000 (0x04)
        int y; // 0x0004 (0x04)
        int z; // 0x0008 (0x04)
    };

    static_assert(sizeof( int3_t ) == 0x0c);

    struct dimension_t // size: 0x000c
    {
        float width; // 0x0000 (0x04)
        float height; // 0x0004 (0x04)
        float length; // 0x0008 (0x04)
    };

    namespace prism
    {
        class placement_t // Size: 0x0020
        {
        public:
            float3_t pos; //0x0000 (0x0c)
            int16_t cx; //0x000C (0x02)
            int16_t cz; //0x000E (0x02)
            quat_t rot; //0x0010 (0x10)

            float3_t to_global_position() const;
            float get_distance_to(const float3_t position) const;

        };
        static_assert(sizeof(placement_t) == 0x20);

        class aabox_t // Size: 0x0018 1.54
        {
        public:
            float3_t start; // 0x0000 (0x0c)
            float3_t end; // 0x000C (0x0c)

            dimension_t get_dimensions() const;
        };
        static_assert( sizeof( aabox_t ) == 0x18 );

    }
}
#pragma pack(pop)
