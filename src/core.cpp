#include "core.hpp"
#include "consts.hpp"

#ifdef INCLUDE_GUI
#include "gui/gui.hpp"
#endif

#include "memory/memory_utils.hpp"
#include "sdk/stores.hpp"

#include <common/scssdk_telemetry_truck_common_channels.h>

#include "prism/controllers/base_ctrl.hpp"
#include "prism/controllers/game_ctrl.hpp"
#include "prism/camera/core_camera.hpp"
#include "prism/camera/camera_manager.hpp"
#include "prism/navigation/gps_manager.hpp"
#include "prism/traffic/game_traffic.hpp"

#include "prism/management/item/node_item.hpp"
#include "prism/navigation/route_task.hpp"

#include "prism/game_actor.hpp"

#include "processing/traffic.hpp"

#include <ctime>
#include <cmath>

#include <vector>
#include <chrono>
#include <algorithm>
#include <array>
#include <map>

namespace ets2la_plugin
{
    CCore *CCore::g_instance = nullptr;

    SCSAPI_VOID telemetry_tick(const scs_event_t event, const void *const event_info, scs_context_t context)
    {
        CCore::g_instance->tick();
    }

    CCore::CCore(const scs_telemetry_init_params_v101_t *init_params) : init_params_(init_params)
    {
        scs_log_ = init_params->common.log;
        memory_manager_ = new CMemoryHandler(scs_log_);
        g_instance = this;
    }

    CCore::~CCore()
    {
        this->destroy();
    }

    void CCore::get_camera_data() const
    {
        const auto camera_manager = prism::camera_manager_u::get();

        if (camera_manager == nullptr)
            return;

        // make sure index is in the list
        if (camera_manager->current_camera >= camera_manager->cameras.size)
            return;

        const auto *current_camera = camera_manager->cameras[camera_manager->current_camera];
        if (current_camera != nullptr)
        {
            CameraMemData data;
            data.fov = current_camera->camera_fov;
            data.pos_x = current_camera->placement.pos.x;
            data.pos_y = current_camera->placement.pos.y;
            data.pos_z = current_camera->placement.pos.z;
            data.cx = current_camera->placement.cx;
            data.cz = current_camera->placement.cz;
            data.qw = current_camera->placement.rot.w;
            data.qx = current_camera->placement.rot.x;
            data.qy = current_camera->placement.rot.y;
            data.qz = current_camera->placement.rot.z;

            data.m11 = current_camera->projection_matrix.m11;
            data.m12 = current_camera->projection_matrix.m12;
            data.m13 = current_camera->projection_matrix.m13;
            data.m14 = current_camera->projection_matrix.m14;
            data.m21 = current_camera->projection_matrix.m21;
            data.m22 = current_camera->projection_matrix.m22;
            data.m23 = current_camera->projection_matrix.m23;
            data.m24 = current_camera->projection_matrix.m24;
            data.m31 = current_camera->projection_matrix.m31;
            data.m32 = current_camera->projection_matrix.m32;
            data.m33 = current_camera->projection_matrix.m33;
            data.m34 = current_camera->projection_matrix.m34;
            data.m41 = current_camera->projection_matrix.m41;
            data.m42 = current_camera->projection_matrix.m42;
            data.m43 = current_camera->projection_matrix.m43;
            data.m44 = current_camera->projection_matrix.m44;

            // We add the truck position to the camera data to avoid jitter on the ETS2LA side.
            // To render at an offset to the truck (e.g. the HUD), it's position has to be synced with the camera timestamp.
            // Sending it along with the camera data is the most reliable way to achieve this (to my knowledge!).
            const auto game_actor = prism::game_actor_u::get();
            if (game_actor != nullptr)
            {
                const auto* our_truck = game_actor->game_physics_vehicle;
                prism::placement_t truck_placement;
                our_truck->get_interpolated_placement(&truck_placement);
                const auto truck_position = traffic_processor_->get_center_coords(truck_placement, our_truck->aabox);

                data.truck_pos_x = truck_position.x;
                data.truck_pos_y = truck_position.y;
                data.truck_pos_z = truck_position.z;

                data.truck_rot_w = truck_placement.rot.w;
                data.truck_rot_x = truck_placement.rot.x;
                data.truck_rot_y = truck_placement.rot.y;
                data.truck_rot_z = truck_placement.rot.z;
            } else {
                data.truck_pos_x = 0;
                data.truck_pos_y = 0;
                data.truck_pos_z = 0;

                data.truck_rot_w = 1;
                data.truck_rot_x = 0;
                data.truck_rot_y = 0;
                data.truck_rot_z = 0;
            }

            this->memory_manager_->write_camera_mem(data);
        }
    }

    void CCore::get_navigation_data() const
    {
        auto *gps_manager = prism::gps_manager_t::get();

        if (gps_manager == nullptr)
        {
            return;
        }

        // route_task is nullptr when no route is set
        if (gps_manager->simple_route_source.route_task != nullptr)
        {
            if (gps_manager->simple_route_source.route_task->physical_route_items.size != last_route_length_)
            {
                last_route_length_ = gps_manager->simple_route_source.route_task->physical_route_items.size;
                std::array<RouteTaskObject, 6000> route_tasks = {};
                int i = 0;
                for (const auto &route_item : gps_manager->simple_route_source.route_task->physical_route_items)
                {
                    RouteTaskObject task = {};
                    prism::node_item_t *node = route_item.node;
                    task.uid = node->uid;

                    task.distance = route_item.total_distance_till_end;
                    task.time = route_item.total_time_till_end;

                    route_tasks[i] = task;
                    i++;
                }

                RouteMemData data = { route_tasks };
                this->memory_manager_->write_route_mem(data);
            }
        }
    }

    void CCore::override_inputs() const
    {
        InputMemData data = this->memory_manager_->read_input_mem();

        bool should_override_steering = data.override_steering;
        bool should_override_acceleration = data.override_acceleration;

        double steering_timestamp = data.steering_timestamp;
        double acceleration_timestamp = data.acceleration_timestamp;

        float custom_steering_angle = data.steering;
        float custom_acceleration = data.acceleration;

        double current_time = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (current_time - steering_timestamp > 0.2)
        {
            should_override_steering = false;
        }
        if (current_time - acceleration_timestamp > 0.2)
        {
            should_override_acceleration = false;
        }

        auto* game_actor = prism::game_actor_u::get();
        if (game_actor != nullptr) {
            if (should_override_steering && game_actor->game_physics_vehicle != nullptr)
            {
                if(!was_overriding_steering){
                    was_overriding_steering = true;
                    steering_start_time = current_time;
                    this->info("controlling steering.");
                }
                double t = (current_time - steering_start_time) / 1.5; // 1.5s to fully take control
                if (t < 1) {
                    t = sin((t * 3.1415) / 2); // easeOutSine
                    float cur = game_actor->game_physics_vehicle->get_steering_angle();
                    custom_steering_angle = cur * (1 - t) + (custom_steering_angle * t); // lerp
                }
                game_actor->game_physics_vehicle->set_steering_angle(custom_steering_angle);
            }
            else{
                if(was_overriding_steering){
                    was_overriding_steering = false;
                    this->info("stopped controlling steering.");
                }
            }

            if (should_override_acceleration && game_actor->game_physics_vehicle != nullptr)
            {
                if (!was_overriding_acceleration) {
                    was_overriding_acceleration = true;
                    this->info("controlling acceleration.");
                }

                if (custom_acceleration > 0)
                {
                    game_actor->set_throttle_input(custom_acceleration);
                    game_actor->set_brake_input(0.0f);
                }
                else
                {
                    game_actor->set_throttle_input(0.0f);
                    game_actor->set_brake_input(-custom_acceleration);
                }
            }
            else
            {
                if (was_overriding_acceleration) {
                    was_overriding_acceleration = false;
                    this->info("stopped controlling acceleration.");
                    this->info("custom_acceleration: {}", custom_acceleration);
                    this->info("timestamp: {}, current_time: {}, time_diff: {}", acceleration_timestamp, current_time, current_time - acceleration_timestamp);
                }
            }
        }
    }

    void CCore::update_plugin_state() const
    {
        // "1.58.1" -> 15801
        // It's easier to send an int than a string through shared mem.
        int version = 0;
        std::string version_str = VERSION;
        try {
            version_str.erase(std::remove(version_str.begin(), version_str.end(), '.'), version_str.end());
            version = std::stoi(version_str);
        }
        catch (const std::exception& e) {
            this->error("Error parsing plugin version: {}", e.what());
            version = 0;
        }

        this->memory_manager_->write_state_mem(
            PluginStateData{ version, was_overriding_steering, was_overriding_acceleration }
        );
    }

    void CCore::tick() const
    {
        // Local\ETS2LACameraProps
        this->get_camera_data();

        // Local\ETS2LATraffic, Local\ETS2LASemaphore, Local\ETS2LAParkedVehicles
        traffic_processor_->tick(this->truck_pos);

        // Local\ETS2LARoute
        this->get_navigation_data();

        // Local\ETS2LAPluginInput
        this->override_inputs();

        // Local\ETS2LAPluginStatus
        this->update_plugin_state();
    }

    bool CCore::scan_for_required_patterns()
    {
        try
        {
            prism::base_ctrl_u::scan_patterns();
        }
        catch( std::exception& e )
        {
            this->error("Error when scanning base_ctrl memory patterns: {}", e.what());
            return false;
        }

        try
        {
            prism::game_ctrl_u::scan_patterns();
        }
        catch( std::exception& e )
        {
            this->error("Error when scanning game_ctrl memory patterns: {}", e.what());
            return false;
        }

        if ( !prism::camera_manager_u::scan_patterns() )
        {
            this->error( "Could not find camera_manager patterns" );
            return false;
        }

        if ( !prism::game_traffic_u::scan_patterns() )
        {
            this->error( "Could not find game_traffic patterns" );
            return false;
        }

        if ( !prism::vehicle_shared_u::scan_patterns() )
        {
            this->error( "Could not find vehicle_shared patterns" );
            return false;
        }

        try
        {
            prism::game_actor_u::scan_patterns();
        }
        catch( std::exception& e )
        {
            this->error("Error when scanning game_actor memory patterns: {}", e.what());
            return false;
        }

        if ( !prism::game_physics_vehicle_u::scan_patterns() )
        {
            this->error( "Could not find game_physics_vehicle patterns" );
            return false;
        }
        if ( !prism::game_trailer_actor_u::scan_patterns() )
        {
            this->error( "Could not find game_trailer_actor patterns" );
            return false;
        }

        return true;
    }

    bool CCore::init()
    {
        this->info("Initializing {}", VERSION);
        this->info("Expected game version: {}", GAME_VERSION);

        // parse GAME_VERSION from for example "1.55.x" to "1.55." (removing the "x" everywhere)
        auto game_version_parsed = fmt::to_string(GAME_VERSION);
        game_version_parsed.erase(std::remove(game_version_parsed.begin(), game_version_parsed.end(), 'x'), game_version_parsed.end());

        // search for the parsed game version in the game name which is for example: "Euro Truck Simulator 2 1.55.1.0s"
        if (fmt::to_string(this->init_params_->common.game_name).find(game_version_parsed) == std::string::npos) {
            this->error("Detected unsupported game version: {}", fmt::to_string(this->init_params_->common.game_name));
            return false; // game version not supported, dont load plugin
        } else {
            this->info("Detected matching game version: {}", fmt::to_string(this->init_params_->common.game_name));
        }

        if ( !this->scan_for_required_patterns() )
        {
            this->error( "Could not find required memory patterns" );
            return false;
        }

        if (this->init_params_->register_for_channel(
                SCS_TELEMETRY_TRUCK_CHANNEL_world_placement,
                SCS_U32_NIL,
                SCS_VALUE_TYPE_dplacement,
                SCS_TELEMETRY_CHANNEL_FLAG_none,
                telemetry_store_dplacement,
                &this->truck_pos) != SCS_RESULT_ok)
        {
            this->error("Could not register for truck world_placement channel");
            return false;
        }

        this->memory_manager_ = new CMemoryHandler(scs_log_);
        this->memory_manager_->init();

        this->traffic_processor_ = new TrafficProcessor(scs_log_, this->memory_manager_);

        if (this->init_params_->register_for_event(SCS_TELEMETRY_EVENT_frame_end, telemetry_tick, nullptr) != SCS_RESULT_ok)
        {
            this->error("Could not register for frame_end event");
            return false;
        }

#ifdef INCLUDE_GUI
        this->gui_ = new CGui();
        this->gui_->init();
#endif

        return true;
    }

    void CCore::destroy()
    {
        delete this->memory_manager_;
        delete this->traffic_processor_;
#ifdef INCLUDE_GUI
        delete this->gui_;
#endif
    }
}
