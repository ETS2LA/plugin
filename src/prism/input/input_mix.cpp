#include "input_mix.hpp"

#include "core.hpp"
#include "memory/memory_utils.hpp"
#include "patterns.hpp"
#include <stdexcept>

namespace ets2la_plugin::prism
{
#if defined( _WIN32 )
    uint64_t input_mix_t::mixes_hash_table = 0;
    uint8_t input_mix_t::vfunc_offset      = 0;
#else
    input_mix_t* ( *input_mix_t::inp_get_mix_fn )( const input_object_name_t& ) = nullptr;
#endif
    uint64_t input_mix_t::value_offset = 0;

    bool input_mix_t::scan_patterns()
    {
#if defined( _WIN32 )
        auto addr = memory::get_address_for_pattern( patterns::inp_get_mix::hash_table_pattern );

        if ( addr == 0 )
        {
            throw std::runtime_error( "Failed to find inp_get_mix hash_table_pattern" );
        }
        mixes_hash_table = memory::get_absolute_address_from_offset( addr, patterns::inp_get_mix::hash_table_offset );

        addr = memory::get_address_for_pattern( patterns::inp_get_mix::vfunc_value_pattern );

        if ( addr == 0 )
        {
            throw std::runtime_error( "Failed to find inp_get_mix vfunc & value pattern" );
        }

        vfunc_offset = *reinterpret_cast< uint8_t* >( addr + patterns::inp_get_mix::vfunc_offset );
        value_offset = *reinterpret_cast< uint32_t* >( addr + patterns::inp_get_mix::value_offset );

        CCore::g_instance->debug(
            "Found inp_get_mix vtable @ +{:x} vfunc[{}] with value offset 0x{:x}",
            mixes_hash_table,
            vfunc_offset,
            value_offset
        );
#else
        auto addr = memory::get_address_for_pattern( patterns::inp_get_mix::pattern );

        if ( addr == 0 )
        {
            throw std::runtime_error( "Failed to find inp_get_mix pattern" );
        }
        inp_get_mix_fn = reinterpret_cast< input_mix_t* ( * )( const input_object_name_t& ) >(
            memory::get_absolute_address_from_offset( addr, patterns::inp_get_mix::fn_offset )
        );
        value_offset = *reinterpret_cast< uint32_t* >( addr + patterns::inp_get_mix::value_offset );

        CCore::g_instance->debug(
            "Found inp_get_mix function @ +{:x} with value offset 0x{:x}",
            reinterpret_cast< uint64_t >( inp_get_mix_fn ),
            value_offset
        );
#endif

        return true;
    }

    input_mix_t* input_mix_t::get( const token_t token )
    {
        input_object_name_t query_data{ token, token_t( 0 ), token_t( 0 ) };
#if defined( _WIN32 )
        if ( mixes_hash_table == 0 || vfunc_offset == 0 || value_offset == 0 )
        {
            return false;
        }
        auto hash_table_find_fn = reinterpret_cast< input_mix_t** ( * )( uint64_t, const input_object_name_t& ) >(
            *reinterpret_cast< uint64_t* >( *reinterpret_cast< uint64_t* >( mixes_hash_table ) + vfunc_offset )
        );

        return *hash_table_find_fn( mixes_hash_table, query_data );
#else
        return inp_get_mix_fn( query_data );
#endif
    }

    float input_mix_t::get_value()
    {
        if ( value_offset == 0 )
        {
            return false;
        }

        return *reinterpret_cast< float* >( reinterpret_cast< char* >( this ) + value_offset );
    }

    bool input_mix_t::set_value( const float value )
    {
        if ( value_offset == 0 )
        {
            return false;
        }

        *reinterpret_cast< float* >( reinterpret_cast< char* >( this ) + value_offset ) = value;

        return true;
    }
}
