#include "traffic.hpp"

#include "imgui.h"

#include "gui/utils.hpp"
#include "prism/camera/core_camera.hpp"
#include "prism/common.hpp"

#include "prism/camera/camera_manager.hpp"
#include "prism/traffic/game_traffic.hpp"
#include "prism/traffic/objects/traffic_ai_trailer.hpp"
#include "prism/traffic/objects/traffic_ai_vehicle.hpp"
#include "prism/traffic/objects/traffic_player_trailer.hpp"
#include "prism/traffic/objects/traffic_player_vehicle.hpp"

#include "prism/traffic/objects/traffic_parked_actor.hpp"
#include "prism/traffic/objects/traffic_parked_trailer.hpp"

namespace ets2la_plugin
{

    inline constexpr auto CAM_TRIANGLE_COLOR      = 0xAAff00ff; // = ABGR | #ff00ffaa = RGBA
    inline constexpr auto AI_VEHICLE_COLOR        = 0xAAe27777; // = ABGR | #7777e2aa = RGBA
    inline constexpr auto AI_TRAILER_COLOR        = 0xAAde4949; // = ABGR | #4949deaa = RGBA
    inline constexpr auto PLAYER_VEHICLE_COLOR    = 0xAA7bcf7b; // = ABGR | #7bcf7baa = RGBA
    inline constexpr auto PLAYER_TRAILER_COLOR    = 0xAA3dca3d; // = ABGR | #3dca3daa = RGBA
    inline constexpr auto PARKED_VEHICLE_COLOR    = 0xAAc3c3c3; // = ABGR | #c3c3c3aa = RGBA
    inline constexpr auto PARKED_TRAILER_COLOR    = 0xAA898989; // = ABGR | #898989aa = RGBA
    inline constexpr auto OBJECT_AI_VEHICLE_COLOR = 0xAAe2cd77; // = ABGR | #77cde2aa = RGBA
    inline constexpr auto OBJECT_AI_TRAILER_COLOR = 0xAAdebb31; // = ABGR | #31bbdeaa = RGBA
    inline constexpr ImVec4 RED                   = { 0xFF, 0x00, 0x00, 0xFF };
    float ui_scale                                = 3.f;
    float2_t window_center;

    float3_t get_corner( const float3_t& pos, const quat_t& rot, const float3_t& offset )
    {
        return pos + offset.rotate( rot );
    }

    uint32_t get_box_color( const uint32_t color ) { return ( color | 0xFF000000 ) & 0x55FFFFFF; }

    void draw_vehicle_world_box(
        const prism::core_camera_u* cam,
        const prism::placement_t& veh_placement,
        const prism::aabox_t& veh_aabox,
        const uint32_t color
    )
    {
        auto* draw = ImGui::GetBackgroundDrawList();

        const float width  = veh_aabox.end.x - veh_aabox.start.x;
        const float height = veh_aabox.end.y - veh_aabox.start.y;
        const float length = veh_aabox.end.z - veh_aabox.start.z;

        float3_t center = {
            veh_aabox.start.x + ( width / 2.f ),
            veh_aabox.start.y + ( height / 2.f ),
            veh_aabox.start.z + ( length / 2.f ),
        };

        const auto centered_veh_pos = veh_placement.to_global_position() + center.rotate( veh_placement.rot );

        // FL
        auto ai_fl_b = drawing::world_to_screen(
            cam,
            get_corner(
                centered_veh_pos, veh_placement.rot, { -( width / 2.f ), -( height / 2.f ), -( length / 2.f ) }
            )
        );
        auto ai_fl_t = drawing::world_to_screen(
            cam,
            get_corner( centered_veh_pos, veh_placement.rot, { -( width / 2.f ), ( height / 2.f ), -( length / 2.f ) } )
        );

        // FR
        auto ai_fr_b = drawing::world_to_screen(
            cam,
            get_corner( centered_veh_pos, veh_placement.rot, { ( width / 2.f ), -( height / 2.f ), -( length / 2.f ) } )
        );
        auto ai_fr_t = drawing::world_to_screen(
            cam,
            get_corner( centered_veh_pos, veh_placement.rot, { ( width / 2.f ), ( height / 2.f ), -( length / 2.f ) } )
        );

        // RR
        auto ai_rr_b = drawing::world_to_screen(
            cam,
            get_corner( centered_veh_pos, veh_placement.rot, { ( width / 2.f ), -( height / 2.f ), ( length / 2.f ) } )
        );
        auto ai_rr_t = drawing::world_to_screen(
            cam,
            get_corner( centered_veh_pos, veh_placement.rot, { ( width / 2.f ), ( height / 2.f ), ( length / 2.f ) } )
        );

        // RL
        auto ai_rl_b = drawing::world_to_screen(
            cam,
            get_corner( centered_veh_pos, veh_placement.rot, { -( width / 2.f ), -( height / 2.f ), ( length / 2.f ) } )
        );
        auto ai_rl_t = drawing::world_to_screen(
            cam,
            get_corner( centered_veh_pos, veh_placement.rot, { -( width / 2.f ), ( height / 2.f ), ( length / 2.f ) } )
        );

        if ( ai_fl_b.z < 0 && ai_fl_t.z < 0 && ai_fr_b.z < 0 && ai_fr_t.z < 0 && ai_rr_b.z < 0 && ai_rr_t.z < 0 &&
             ai_rl_b.z < 0 && ai_rl_t.z < 0 )
        {
            // bottom
            draw->AddQuad(
                { ai_fl_b.x, ai_fl_b.y },
                { ai_fr_b.x, ai_fr_b.y },
                { ai_rr_b.x, ai_rr_b.y },
                { ai_rl_b.x, ai_rl_b.y },
                color
            );
            // top
            draw->AddQuad(
                { ai_fl_t.x, ai_fl_t.y },
                { ai_fr_t.x, ai_fr_t.y },
                { ai_rr_t.x, ai_rr_t.y },
                { ai_rl_t.x, ai_rl_t.y },
                color
            );
            // left
            draw->AddQuad(
                { ai_fl_b.x, ai_fl_b.y },
                { ai_fl_t.x, ai_fl_t.y },
                { ai_rl_t.x, ai_rl_t.y },
                { ai_rl_b.x, ai_rl_b.y },
                color
            );
            // right
            draw->AddQuad(
                { ai_fr_b.x, ai_fr_b.y },
                { ai_fr_t.x, ai_fr_t.y },
                { ai_rr_t.x, ai_rr_t.y },
                { ai_rr_b.x, ai_rr_b.y },
                color
            );
            // front
            draw->AddQuad(
                { ai_fl_t.x, ai_fl_t.y },
                { ai_fr_t.x, ai_fr_t.y },
                { ai_fr_b.x, ai_fr_b.y },
                { ai_fl_b.x, ai_fl_b.y },
                color
            );
            // back
            draw->AddQuad(
                { ai_rl_t.x, ai_rl_t.y },
                { ai_rr_t.x, ai_rr_t.y },
                { ai_rr_b.x, ai_rr_b.y },
                { ai_rl_b.x, ai_rl_b.y },
                color
            );
        }
    }

    void draw_vehicle(
        const prism::core_camera_u* cam,
        const prism::placement_t& veh_placement,
        const prism::aabox_t& veh_aabox,
        const uint32_t color
    )
    {
        auto* draw         = ImGui::GetWindowDrawList();
        const auto cam_pos = cam->placement.to_global_position();
        const auto rel_pos = veh_placement.to_global_position() - cam_pos;
        const auto cam_rot = cam->placement.rot.conjugate();

        float3_t ai_fl = { veh_aabox.start.x, 0, veh_aabox.start.z };
        ai_fl          = ai_fl.rotate( veh_placement.rot );
        ai_fl          = ai_fl + rel_pos;
        ai_fl          = ai_fl.rotate( cam_rot );

        float3_t ai_fr = { veh_aabox.end.x, 0, veh_aabox.start.z };
        ai_fr          = ai_fr.rotate( veh_placement.rot );
        ai_fr          = ai_fr + rel_pos;
        ai_fr          = ai_fr.rotate( cam_rot );

        float3_t ai_rr = { veh_aabox.end.x, 0, veh_aabox.end.z };
        ai_rr          = ai_rr.rotate( veh_placement.rot );
        ai_rr          = ai_rr + rel_pos;
        ai_rr          = ai_rr.rotate( cam_rot );

        float3_t ai_rl = { veh_aabox.start.x, 0, veh_aabox.end.z };
        ai_rl          = ai_rl.rotate( veh_placement.rot );
        ai_rl          = ai_rl + rel_pos;
        ai_rl          = ai_rl.rotate( cam_rot );

        draw->AddQuadFilled(
            drawing::world_to_window( ai_fl, window_center, ui_scale ),
            drawing::world_to_window( ai_fr, window_center, ui_scale ),
            drawing::world_to_window( ai_rr, window_center, ui_scale ),
            drawing::world_to_window( ai_rl, window_center, ui_scale ),
            color
        );

        draw_vehicle_world_box( cam, veh_placement, veh_aabox, get_box_color( color ) );
    }

    void draw_ai_vehicles(
        const prism::core_camera_u* cam,
        prism::array_dyn_t< prism::traffic_u::spawned_vehicle_t >& ai_vehicles,
        const uint32_t truck_color,
        const uint32_t trailer_color
    )
    {
        int i = -1;
        for ( const auto& ai_vehicle : ai_vehicles )
        {
            ++i;
            if ( ai_vehicle.vehicle == nullptr || ai_vehicle.vehicle->traffic_vehicle == nullptr )
            {
                continue;
            }

            draw_vehicle( cam, ai_vehicle.vehicle->placement, ai_vehicle.vehicle->aabox, truck_color );

            const auto* trailer = ai_vehicle.vehicle->trailer;
            int trailer_count   = 0;

            // go through all slave trailers
            while ( trailer != nullptr )
            {
                draw_vehicle( cam, trailer->placement, trailer->aabox, trailer_color );
                trailer = trailer->slave_trailer;
                ++trailer_count;
            }
        }
    }

    void draw_traffic_object( const prism::core_camera_u* cam, const prism::traffic_object_t* traffic_object )
    {
        const auto object_type = traffic_object->get_type();

        if ( object_type == prism::ETrafficObjectType::traffic_ai_vehicle )
        {
            const auto* ai_vehicle = static_cast< const prism::traffic_ai_vehicle_t* >( traffic_object );
            draw_vehicle( cam, ai_vehicle->placement, ai_vehicle->aabox, OBJECT_AI_VEHICLE_COLOR );
        }
        else if ( object_type == prism::ETrafficObjectType::traffic_ai_trailer )
        {
            const auto* ai_trailer = static_cast< const prism::traffic_ai_trailer_t* >( traffic_object );
            draw_vehicle( cam, ai_trailer->placement, ai_trailer->aabox, OBJECT_AI_TRAILER_COLOR );
        }
        else if ( object_type == prism::ETrafficObjectType::traffic_parked_vehicle )
        {
            const auto* parked_actor = static_cast< const prism::traffic_parked_actor_t* >( traffic_object );
            draw_vehicle( cam, parked_actor->placement, parked_actor->aabox, PARKED_VEHICLE_COLOR );
            auto* trailer = parked_actor->slave_trailer;

            while ( trailer != nullptr )
            {
                draw_vehicle( cam, trailer->placement, trailer->aabox, PARKED_TRAILER_COLOR );
                trailer = trailer->slave_trailer;
            }
        }
        else if ( object_type == prism::ETrafficObjectType::traffic_parked_trailer )
        {
            const auto* parked_actor = static_cast< const prism::traffic_parked_trailer_t* >( traffic_object );
            draw_vehicle( cam, parked_actor->placement, parked_actor->aabox, PARKED_TRAILER_COLOR );
            auto* trailer = parked_actor->slave_trailer;

            while ( trailer != nullptr )
            {
                draw_vehicle( cam, trailer->placement, trailer->aabox, PARKED_TRAILER_COLOR );
                trailer = trailer->slave_trailer;
            }
        }
        else
        {
            // ImGui::Text( "%llx: %s", ( uint64_t )traffic_object, traffic_object->get_type_name() );
        }
    }

    void CTraffic::render()
    {
        ImGui::Begin( "Traffic" );

        const auto ui_position = ImGui::GetCursorScreenPos();
        const auto w_width     = ImGui::GetWindowWidth();
        const auto w_height    = ImGui::GetWindowHeight();
        window_center          = { ui_position.x + w_width / 2.f, ui_position.y + w_height / 2.f };

        ImGui::DragFloat( "UI Scale", &ui_scale, 0.1f, 0.1f, 10.0f );

        auto* game_traffic   = prism::game_traffic_u::get();
        auto* camera_manager = prism::camera_manager_u::get();

        if ( camera_manager == nullptr || camera_manager->current_camera >= camera_manager->cameras.size )
        {
            ImGui::TextColored( RED, "Could not get camera from camera manager" );
            ImGui::End();
            return;
        }

        const auto* current_camera = camera_manager->cameras[ camera_manager->current_camera ];

        if ( current_camera == nullptr )
        {
            ImGui::TextColored( RED, "Camera %d is nullptr", camera_manager->current_camera );
            ImGui::End();
            return;
        }

        auto* draw = ImGui::GetWindowDrawList();

        // our camera triangle
        draw->AddTriangleFilled(
            { window_center.x - 2 * ui_scale, window_center.y + 2 * ui_scale },
            { window_center.x, window_center.y - 2 * ui_scale },
            { window_center.x + 2 * ui_scale, window_center.y + 2 * ui_scale },
            CAM_TRIANGLE_COLOR
        );

        if ( game_traffic != nullptr )
        {
            draw_ai_vehicles( current_camera, game_traffic->spawned_vehicles_1, AI_VEHICLE_COLOR, AI_TRAILER_COLOR );
            draw_ai_vehicles( current_camera, game_traffic->spawned_vehicles_2, AI_VEHICLE_COLOR, AI_TRAILER_COLOR );

            for ( const auto* traffic_player_vehicle : game_traffic->traffic_player_vehicles_1 )
            {
                draw_vehicle(
                    current_camera,
                    traffic_player_vehicle->placement,
                    traffic_player_vehicle->aabox,
                    PLAYER_VEHICLE_COLOR
                );
            }

            for ( const auto* traffic_player_trailer : game_traffic->traffic_player_trailers_1 )
            {
                draw_vehicle(
                    current_camera,
                    traffic_player_trailer->placement,
                    traffic_player_trailer->aabox,
                    PLAYER_TRAILER_COLOR
                );
            }

            for ( const auto* traffic_object : game_traffic->traffic_objects_1 )
            {
                draw_traffic_object( current_camera, traffic_object );
            }
        }
        ImGui::End();
    }
}
