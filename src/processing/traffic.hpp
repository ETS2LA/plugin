#pragma once

#include "prism/traffic/objects/traffic_actor.hpp"
#include "prism/traffic/objects/traffic_ai_trailer.hpp"
#include "prism/traffic/objects/traffic_ai_vehicle.hpp"
#include "prism/traffic/objects/traffic_semaphore_actor.hpp"
#include "prism/vehicles/game_physics_vehicle.hpp"
#include "prism/vehicles/game_trailer_actor.hpp"
#include "prism/management/item/semaphore_instance.hpp"
#include "memory/virtual/memory_handler.hpp"
#include <vector>
#include <map>

namespace ets2la_plugin
{
    // Used internally in the TrafficProcessor to store ai in a way I can use them easily.
    struct processor_traffic_vehicle_object_t
    {
        float truck_distance;
        float speed;
        float acceleration;
        const prism::traffic_actor_t* traffic_actor;
    };

    // Used internally in the TrafficProcessor to traffic objects (traffic lights and gates)
    // in a way I can use them easily.
    struct processor_semaphore_object_t
    {
        uint32_t id;
        float distance;
        const prism::traffic_semaphore_actor_t* traffic_object;
    };

    struct processor_actor_object_t
    {
        bool is_trailer;
        float distance;
        const prism::traffic_actor_t* traffic_actor;
    };

    struct processor_active_actor_object_t : public processor_actor_object_t
    {
        float speed;
        float acceleration;
    };

    class TrafficProcessor
    {
        private:
            mutable std::map<uintptr_t, short> vehicle_uids = {};
            CMemoryHandler* memory_handler_;
            scs_log_t scs_log_;
            float3_t truck_pos;

            template <class... T>
            void info(const char *fmt_s, T &&...args) const
            {
                scs_log_(0, fmt::vformat(std::string("[ets2la_plugin][TrafficProcessor] ") + fmt_s, fmt::make_format_args(args...)).c_str());
            }

            std::vector<processor_active_actor_object_t> active_actors = {};
            std::vector<processor_actor_object_t> parked_actors = {};
            std::vector<processor_semaphore_object_t> semaphore_objects = {};

            void clear_data();
            short get_uid_for_vehicle(uintptr_t vehicle_ptr) const;
            void process_traffic_object(const prism::traffic_object_t* traffic_object, const uint32_t id = 0);

            void get_ai_traffic_data();
            void get_traffic_objects_data();
            void get_player_traffic_data();

            void write_semaphore_data();
            /**
             * Writes all traffic actors from `this->parked_actors` to `shm/ETS2LAParkedVehicles`
             */
            void write_parked_vehicle_data();
            void write_active_traffic_data() const;

        public:
            TrafficProcessor(scs_log_t scs_log, CMemoryHandler* memory_handler)
            : scs_log_(scs_log), memory_handler_(memory_handler)
            {
            }

            void tick(scs_value_dplacement_t truck_pos);
    };
}
