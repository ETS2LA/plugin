#pragma once

#include "prism/common.hpp"
#include "prism/unit/unit.hpp"

namespace ets2la_plugin::prism
{
#pragma pack( push, 1 )

    class vehicle_shared_u : public unit_t // Size: 0x0D68 1.54
    {
    public:
        char pad_0010[ 8 ]; // 0x0010 (0x08)
        aabox_t aabox;      // 0x0018 (0x18)
        // class physics_actor_physx_t* dynamic_physics_actor; // 0x0030 (0x08)
        // char pad_0038[ 8 ]; // 0x0038 (0x08)
        // class array_dyn_t< class physics_shape_t* > physics_shapes; // 0x0040 (0x20)
        // char pad_0060[ 8 ]; // 0x0060 (0x08)
        // class game_trailer_actor_u* game_physics_vehicle; // 0x0068 (0x08)
        // char pad_0070[ 80 ]; // 0x0070 (0x50)
        // class array_dyn_t< class placement_interface_t* > wheel_placement_interfaces; // 0x00C0 (0x20)
        // class array_dyn_t< class wheel_instance_t > wheel_instances; // 0x00E0 (0x20)
        // class physics_powertrain_u* physics_powertrain; // 0x0100 (0x08)
        // class array_dyn_t< class model_object_u* > chassis_model_objects; // 0x0108 (0x20)
        // char pad_0128[ 32 ]; // 0x0128 (0x20)
        // class core_vehicle_u* vehicle; // 0x0148 (0x08)
        // class accessory_chassis_data_u* accessory_chassis_data; // 0x0150 (0x08)
        // class array_dyn_t< class physics_integrator_t* > physics_integrators; // 0x0158 (0x20)
        // char pad_0178[ 32 ]; // 0x0178 (0x20)
        // bool moving_air_suspension_1; // 0x0198 (0x01)
        // bool moving_air_suspension_2; // 0x0199 (0x01)
        // bool resetting_air_suspension; // 0x019A (0x01)
        // char pad_019B[ 5 ]; // 0x019B (0x05)
        // class fake_shadow_u* fake_shadow; // 0x01A0 (0x08)
        // char pad_01A8[ 40 ]; // 0x01A8 (0x28)
        // class kdop_item_t* current_kdop_item; // 0x01D0 (0x08)
        // uint8_t light_state; // 0x01D8 (0x01)
        // char pad_01D9[ 15 ]; // 0x01D9 (0x0f)
        // class array_dyn_t< uint32_t > wheels_not_on_ground; // 0x01E8 (0x20)
        // char pad_0208[ 32 ]; // 0x0208 (0x20)
        // class array_dyn_t< float3_t > wheel_offsets; // 0x0228 (0x20)
        // class array_dyn_t< float3_t > wheel_locations; // 0x0248 (0x20)
        // class array_dyn_t< float > tire_shifts; // 0x0268 (0x20)
        // class array_dyn_t< float > tire_sizes; // 0x0288 (0x20)
        // class array_dyn_t< float > N00002B76; // 0x02A8 (0x20)
        // class array_dyn_t< float > N00002B7A; // 0x02C8 (0x20)
        // class array_dyn_t< float3_t > N00002B7E; // 0x02E8 (0x20)
        // char pad_0308[ 44 ]; // 0x0308 (0x2c)
        // float N000060E9; // 0x0334 (0x04)
        // float N00002B88; // 0x0338 (0x04)
        // float3_t hook_locator; // 0x033C (0x0c)
        // float N0000520A; // 0x0348 (0x04)
        // float N00006014; // 0x034C (0x04)
        // float N00002B8A; // 0x0350 (0x04)
        // char pad_0354[ 20 ]; // 0x0354 (0x14)
        // class array_dyn_t< float > precalculated_axle_weight; // 0x0368 (0x20)
        // char pad_0388[ 8 ]; // 0x0388 (0x08)
        // float trailer_connecting_state; // 0x0390 (0x04) 1 = connected, 0 = disconnected
        // char pad_0394[ 8 ]; // 0x0394 (0x08)
        // float front_air_suspension; // 0x039C (0x04) 0.5 - 2
        // float rear_air_suspension; // 0x03A0 (0x04) 0.5 - 2
        // char pad_03A4[ 4 ]; // 0x03A4 (0x04)
        // class array_dyn_t< class sagging_probe_result_t > sagging_probe_results; // 0x03A8 (0x20)
        // char pad_03C8[ 64 ]; // 0x03C8 (0x40)
        // class array_dyn_t< class physics_suspension_t* > physics_suspensions; // 0x0408 (0x20)
        // char pad_0428[ 32 ]; // 0x0428 (0x20)
        // class array_dyn_t< float > N00002BB0; // 0x0448 (0x20)
        // class array_dyn_t< float > wheel_weights; // 0x0468 (0x20)
        // class array_dyn_t< float > N00002BB4; // 0x0488 (0x20)
        // class array_dyn_t< float > N00002BB8; // 0x04A8 (0x20)
        // float steering__dont_use; // 0x04C8 (0x04)
        // char pad_04CC[ 4 ]; // 0x04CC (0x04)
        // class vehicle_wheel_steering_data_t* wheel_steering_stuff; // 0x04D0 (0x08)
        // char pad_04D8[ 8 ]; // 0x04D8 (0x08)
        // class array_dyn_t< float3_t > N00002BC4; // 0x04E0 (0x20)
        // char pad_0500[ 56 ]; // 0x0500 (0x38)
        // class array_dyn_t< float > brake_input; // 0x0538 (0x20)
        // char pad_0558[ 32 ]; // 0x0558 (0x20)
        // class array_dyn_t< class addon_instance_t > addon_instances; // 0x0578 (0x20)
        // class array_dyn_t< class addon_hookup_instance_t > addon_hookup_instances; // 0x0598 (0x20)
        // char pad_05B8[ 40 ]; // 0x05B8 (0x28)
        // class history_stream_set_u* N00002BE0; // 0x05E0 (0x08)
        // char pad_05E8[ 1344 ]; // 0x05E8 (0x540)
        // float3_t linear_acceleration; // 0x0B28 (0x0c)
        // char pad_0B34[ 156 ]; // 0x0B34 (0x9c)
        // float3_t angular_acceleration; // 0x0BD0 (0x0c)
        // char pad_0BDC[ 4 ]; // 0x0BDC (0x04)
        // class array_dyn_t< float > wheels_20_steering; // 0x0BE0 (0x20)
        // char pad_0C00[ 88 ]; // 0x0C00 (0x58)
        // class array_dyn_t< physics_shape_t* > N00005533; // 0x0C58 (0x20)
        // char pad_0C78[ 48 ]; // 0x0C78 (0x30)
        // class array_dyn_t< physics_shape_t* > N0000553A; // 0x0CA8 (0x20)
        // char pad_0CC8[ 8 ]; // 0x0CC8 (0x08)
        // class array_dyn_t< physics_shape_t* > N0000553F; // 0x0CD0 (0x20)
        // char pad_0CF0[ 120 ]; // 0x0CF0 (0x78)

        virtual void vfunc_12();
        virtual void vfunc_13();
        virtual void vfunc_14();
        virtual void vfunc_15();
        virtual void vfunc_16();
        virtual void vfunc_17();
        virtual void vfunc_18();
        virtual void vfunc_19();
        virtual void vfunc_20();
        virtual void vfunc_21();
        virtual void vfunc_22();
        virtual void vfunc_23();
        virtual void vfunc_24();
        virtual void vfunc_25();
        virtual void vfunc_26();
        virtual void vfunc_27();
        virtual void vfunc_28();
        virtual void get_interpolated_placement( placement_t* ) const;
        virtual void get_interpolated_rotation( quat_t* ) const;
        virtual void get_physics_placement( placement_t* ) const;

        static uint32_t steering_angle_offset;
        static uint32_t* wheel_input_insn_disp_ptr;
        static uint32_t wheel_input_orig_offset;

        static bool scan_patterns();
        static void set_steering_override( bool active );
        bool set_steering_angle( const float new_steering_angle );
        float get_steering_angle();
    };
    // static_assert( sizeof( vehicle_shared_u ) == 0xD68 );

#pragma pack( pop )
}
