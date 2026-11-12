#include "vehicle_shared.hpp"

#include "core.hpp"
#include "memory/memory_utils.hpp"
#include "patterns.hpp"

#if defined( _WIN32 )
#include <windows.h>
#endif

namespace ets2la_plugin::prism
{
    uint32_t vehicle_shared_u::steering_angle_offset       = 0;
    uint32_t* vehicle_shared_u::wheel_input_insn_disp_ptr  = nullptr;
    uint32_t vehicle_shared_u::wheel_input_orig_offset     = 0;

    bool vehicle_shared_u::scan_patterns()
    {
        const auto addr = memory::get_address_for_pattern(
            patterns::vehicle_shared::steering::pattern, patterns::vehicle_shared::steering::offset
        );

        if ( addr == 0 )
        {
            return false;
        }

        vehicle_shared_u::steering_angle_offset = *reinterpret_cast< uint32_t* >( addr );

#if defined( _WIN32 )
        // NOTE: This pattern was found and implemented by Gemini.
        //       I am not a reverse engineer so I cannot validate whether this is correct or not.
        //       Future support is not guaranteed, although if it found it once I would imagine it can be found again.

        // The pattern starts with `eb <rel8>` which jumps directly to `movss xmm1, dword ptr [rdi + wheel_input_offset]`.
        // When a physical steering wheel is connected, that instruction loads [rdi + 0x1470] (the physical wheel angle)
        // and pulls [rdi + steering_angle_offset] toward the physical wheel right before applying steering to the physics wheels.
        const auto jmp_addr = addr - patterns::vehicle_shared::steering::offset;
        if ( *reinterpret_cast< const uint8_t* >( jmp_addr ) == 0xEB )
        {
            const uint8_t rel8       = *reinterpret_cast< const uint8_t* >( jmp_addr + 1 );
            const auto    movss_addr = jmp_addr + 2 + rel8;
            const auto*   bytes      = reinterpret_cast< const uint8_t* >( movss_addr );
            if ( bytes[ 0 ] == 0xF3 && bytes[ 1 ] == 0x0F && bytes[ 2 ] == 0x10 )
            {
                vehicle_shared_u::wheel_input_insn_disp_ptr = reinterpret_cast< uint32_t* >( movss_addr + 4 );
                vehicle_shared_u::wheel_input_orig_offset   = *vehicle_shared_u::wheel_input_insn_disp_ptr;
                CCore::g_instance->debug(
                    "Found vehicle_shared_u::wheel_input_orig_offset {:x}", vehicle_shared_u::wheel_input_orig_offset
                );
            }
        }
#endif

        CCore::g_instance->debug(
            "Found vehicle_shared_u::steering_angle_offset {:x}", vehicle_shared_u::steering_angle_offset
        );

        return true;
    }

    // See note in above scan_patterns().
    void vehicle_shared_u::set_steering_override( const bool active )
    {
#if defined( _WIN32 )
        if ( vehicle_shared_u::wheel_input_insn_disp_ptr == nullptr || vehicle_shared_u::steering_angle_offset == 0 ||
             vehicle_shared_u::wheel_input_orig_offset == 0 )
        {
            return;
        }

        const uint32_t desired =
            active ? vehicle_shared_u::steering_angle_offset : vehicle_shared_u::wheel_input_orig_offset;

        if ( *vehicle_shared_u::wheel_input_insn_disp_ptr == desired )
        {
            return;
        }

        DWORD old_protect = 0;
        if ( VirtualProtect(
                 vehicle_shared_u::wheel_input_insn_disp_ptr, sizeof( uint32_t ), PAGE_EXECUTE_READWRITE, &old_protect
             ) )
        {
            *vehicle_shared_u::wheel_input_insn_disp_ptr = desired;
            VirtualProtect(
                vehicle_shared_u::wheel_input_insn_disp_ptr, sizeof( uint32_t ), old_protect, &old_protect
            );
            FlushInstructionCache(
                GetCurrentProcess(), vehicle_shared_u::wheel_input_insn_disp_ptr, sizeof( uint32_t )
            );
        }
#else
        ( void )active;
#endif
    }

    bool vehicle_shared_u::set_steering_angle( const float new_steering_value )
    {
        if ( vehicle_shared_u::steering_angle_offset == 0 )
        {
            return false;
        }

        *reinterpret_cast< float* >( reinterpret_cast< char* >( this ) + vehicle_shared_u::steering_angle_offset ) =
            new_steering_value;

        return true;
    }

    float vehicle_shared_u::get_steering_angle()
    {
        if ( vehicle_shared_u::steering_angle_offset == 0 )
        {
            return 0.0f;
        }

        return *reinterpret_cast< float* >(
            reinterpret_cast< char* >( this ) + vehicle_shared_u::steering_angle_offset
        );
    }
}
