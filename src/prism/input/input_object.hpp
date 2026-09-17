#pragma once

#include "prism/token.hpp"

namespace ets2la_plugin::prism
{
#pragma pack( push, 1 )

    struct input_object_name_t
    {
        token_t token1;
        token_t token2;
        token_t token3;
    };

    // Size: W|L|A 0x0028
    class input_object_t
    {
    public:
        input_object_name_t name; // W|L|A 0x0008 (W|L|A 0x08)
        uint64_t ext_ref;         // W|L|A 0x0020 (W|L|A 0x08)

        virtual void destructor();
#if defined( __linux__ )
        virtual void destructor2();
#endif
    };

    static_assert( sizeof( input_object_t ) == 0x28 ); // W|L|A

    // Size: W|L|A 0x0010
    template < class T >
    class input_object_ref_base_t
    {
        T* object; // W|L|A 0x0008 (W|L|A 0x08)

        virtual void destructor();
#if defined( __linux__ )
        virtual void destructor2();
#endif
    };
    static_assert( sizeof( input_object_ref_base_t< input_object_t > ) == 0x10 );

    // Size: W|L|A 0x0010
    template < class T >
    class input_object_ref_t : public input_object_ref_base_t< T >
    {
    };
    static_assert( sizeof( input_object_ref_t< input_object_t > ) == 0x10 );

    // Size: W|L|A 0x0010
    template < class T >
    class mix_ref_base_t : public input_object_ref_t< T >
    {
    };
    static_assert( sizeof( mix_ref_base_t< input_object_ref_t< class input_mix_t > > ) == 0x10 );

    // Size: W|L|A 0x0010
    template < class T, uint64_t TKey1, uint64_t TKey2, uint64_t TKey3 >
    class fixed_ref_base_t : public input_object_ref_t< T >
    {
    };
    static_assert(
        sizeof( fixed_ref_base_t< mix_ref_base_t< input_object_ref_t< input_object_t > >, 0, 0, 0 > ) == 0x10
    );

    // Size: W|L|A 0x0010
    template < uint64_t TKey1, uint64_t TKey2, uint64_t TKey3 >
    class mix_fixed_ref_t
        : public fixed_ref_base_t< mix_ref_base_t< input_object_ref_t< class input_mix_t > >, TKey1, TKey2, TKey3 >
    {
    };
    static_assert( sizeof( mix_fixed_ref_t< 0, 0, 0 > ) == 0x10 );

#pragma pack( pop )
}
