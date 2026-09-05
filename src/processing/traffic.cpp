#include "traffic.hpp"

#include "core.hpp"

#include "prism/game_actor.hpp"
#include "prism/traffic/game_traffic.hpp"
#include "prism/traffic/objects/traffic_light.hpp"
#include "prism/traffic/objects/traffic_parked_actor.hpp"
#include "prism/traffic/objects/traffic_parked_trailer.hpp"
#include "prism/traffic/objects/traffic_player_vehicle.hpp"
#include "prism/traffic/objects/traffic_player_trailer.hpp"
#include "prism/controllers/game_ctrl.hpp"
#include "prism/management/item/kdop_item.hpp"
#include "prism/management/item/prefab_item.hpp"
#include "prism/management/item/segment.hpp"
#include "prism/management/item/semaphore_instance.hpp"
#include "utils/vehicle.hpp"

#include <math.h>
#include <algorithm>
#include <unordered_set>

namespace ets2la_plugin
{
    short TrafficProcessor::get_uid_for_vehicle(uintptr_t vehicle_ptr) const
    {
        auto it = vehicle_uids.find(vehicle_ptr);
        if (it != vehicle_uids.end())
        {
            return it->second;
        }
        else
        {
            short new_uid = static_cast<short>(vehicle_uids.size()) + 1;
            vehicle_uids[vehicle_ptr] = new_uid;
            return new_uid;
        }
    }

    void TrafficProcessor::clear_data()
    {
        this->active_actors.clear();
        this->parked_actors.clear();
        this->semaphore_objects.clear();
    }

    void TrafficProcessor::process_traffic_object(const prism::traffic_object_t* traffic_object, const uint32_t id)
    {
        const auto object_type = traffic_object->get_type();

        if (object_type == prism::ETrafficObjectType::traffic_ai_vehicle)
        {
            // special transport escort vehicles (and maybe some other vehicles, not sure)
            const auto ai_vehicle = static_cast<const prism::traffic_ai_vehicle_t*>(traffic_object);

            this->active_actors.emplace_back(
                processor_active_actor_object_t{
                    {
                        false,
                        ai_vehicle->placement.get_distance_to(this->truck_pos),
                        ai_vehicle
                    },
                    ai_vehicle->physics->speed,
                    ai_vehicle->physics->acceleration
                }
            );
        }
        else if (object_type == prism::ETrafficObjectType::traffic_ai_trailer)
        {
            // TODO: check if this is just trailers that are attached to `traffic_ai_vehicle`s or do any loose trailers exist?
        }
        else if (object_type == prism::ETrafficObjectType::traffic_parked_vehicle ||
                 object_type == prism::ETrafficObjectType::traffic_parked_trailer)
        {
            // traffic_parked_* objects are stored as `traffic_parked_actor_t`s
            const auto* traffic_actor = static_cast<const prism::traffic_parked_actor_t*>(traffic_object);

            this->parked_actors.emplace_back(
                processor_actor_object_t{
                    object_type == prism::ETrafficObjectType::traffic_parked_trailer,
                    traffic_actor->placement.get_distance_to(this->truck_pos),
                    traffic_actor
                }
            );

            auto slave_trailer = traffic_actor->slave_trailer;
            while (slave_trailer != nullptr)
            {
                this->parked_actors.emplace_back(
                    processor_actor_object_t{
                        true,
                        slave_trailer->placement.get_distance_to(this->truck_pos),
                        traffic_actor
                    }
                );
                slave_trailer = slave_trailer->slave_trailer;
            }
        }
        else if (object_type == prism::ETrafficObjectType::traffic_semaphore_actor)
        {
            auto* semaphore_actor = static_cast<const prism::traffic_semaphore_actor_t*>(traffic_object);

            semaphore_objects.push_back(processor_semaphore_object_t{
                id,
                semaphore_actor->placement.get_distance_to(this->truck_pos),
                semaphore_actor
            });
        }
    }

    void TrafficProcessor::get_ai_traffic_data()
    {
        auto* game_traffic = prism::game_traffic_u::get();
        if (game_traffic == nullptr)
        {
            return;
        }

        for (const auto& spawned_vehicle : game_traffic->spawned_vehicles_1)
        {
            if (spawned_vehicle.vehicle->physics == nullptr)
            {
                continue;
            }
            active_actors.emplace_back(processor_active_actor_object_t{
                {
                    false,
                    spawned_vehicle.vehicle->placement.get_distance_to(this->truck_pos),
                    spawned_vehicle.vehicle,
                },
                spawned_vehicle.vehicle->physics->speed,
                spawned_vehicle.vehicle->physics->acceleration
            });
        }

        for (const auto& spawned_vehicle : game_traffic->spawned_vehicles_2)
        {
            if (spawned_vehicle.vehicle->physics == nullptr)
            {
                continue;
            }
            active_actors.emplace_back(processor_active_actor_object_t{
                {
                    false,
                    spawned_vehicle.vehicle->placement.get_distance_to(this->truck_pos),
                    spawned_vehicle.vehicle,
                },
                spawned_vehicle.vehicle->physics->speed,
                spawned_vehicle.vehicle->physics->acceleration
            });
        }

        for (auto* traffic_object : game_traffic->traffic_objects_1)
        {
            const auto object_type = traffic_object->get_type();
            if (object_type != prism::ETrafficObjectType::traffic_ai_vehicle)
            {
                continue;
            }
            // special transport escort vehicles (and maybe some other vehicles, not sure)
            const auto ai_vehicle = reinterpret_cast<prism::traffic_ai_vehicle_t*>(traffic_object);
            if (ai_vehicle->physics == nullptr)
            {
                continue;
            }
            active_actors.emplace_back(processor_active_actor_object_t{
                {
                    false,
                    ai_vehicle->placement.get_distance_to(this->truck_pos),
                    ai_vehicle,
                },
                ai_vehicle->physics->speed,
                ai_vehicle->physics->acceleration
            });
        }
    }

    void TrafficProcessor::write_active_traffic_data() const
    {
        std::vector<processor_traffic_vehicle_object_t> traffic_objects;

        // Here we sort the vehicles by their distance to the truck. I've not figured
        // out a better way to do this yet, if you can think of a better sorting algorithm
        // then please ping @Tumppi066 on our Discord server or create a PR.

        for (const auto& traffic_actor : this->active_actors)
        {
            processor_traffic_vehicle_object_t traffic_object;
            traffic_object.traffic_actor = traffic_actor.traffic_actor;
            traffic_object.truck_distance = traffic_actor.distance;
            traffic_object.speed = traffic_actor.speed;
            traffic_object.acceleration = traffic_actor.acceleration;

            traffic_objects.push_back(traffic_object);
        }

        std::sort(traffic_objects.begin(), traffic_objects.end(), [](const processor_traffic_vehicle_object_t& a, const processor_traffic_vehicle_object_t& b) {
            return a.truck_distance < b.truck_distance;
        });

        // Now we need to convert this data to the format the memory uses.

        std::array<TrafficVehicleObject, 40> traffic_vehicle_objects = {};
        int count = 0;

        for (const auto& traffic_object : traffic_objects)
        {
            if (count >= 40)
                break;

            TrafficVehicleObject vehicle_object = {};
            vehicle_object.vehicle.id = -1;

            const auto* traffic_actor = traffic_object.traffic_actor;
            const auto position = utils::vehicle::get_center_coords(traffic_actor->placement, traffic_actor->aabox);
            vehicle_object.vehicle = TrafficVehicle{
                position.x,
                position.y,
                position.z,
                traffic_actor->placement.rot.w,
                traffic_actor->placement.rot.x,
                traffic_actor->placement.rot.y,
                traffic_actor->placement.rot.z,
                abs(traffic_actor->aabox.start.x - traffic_actor->aabox.end.x), // width
                abs(traffic_actor->aabox.start.y - traffic_actor->aabox.end.y), // height
                abs(traffic_actor->aabox.start.z - traffic_actor->aabox.end.z), // length
                traffic_object.speed,          // speed
                traffic_object.acceleration,   // acceleration
                0,                                        // trailer_count
                get_uid_for_vehicle(reinterpret_cast<uintptr_t>(traffic_actor)), // id
            };

            auto trailer = traffic_actor->slave;
            int i = 0;
            while (trailer != nullptr && i < 3)
            {
                const auto trailer_position = utils::vehicle::get_center_coords(trailer->placement, trailer->aabox);
                vehicle_object.trailers[i] = TrafficTrailer{
                    trailer_position.x,
                    trailer_position.y,
                    trailer_position.z,
                    trailer->placement.rot.w,
                    trailer->placement.rot.x,
                    trailer->placement.rot.y,
                    trailer->placement.rot.z,
                    abs(trailer->aabox.start.x - trailer->aabox.end.x), // width
                    abs(trailer->aabox.start.y - trailer->aabox.end.y), // height
                    abs(trailer->aabox.start.z - trailer->aabox.end.z), // length
                };

                trailer = trailer->slave;
                i++;
            }

            vehicle_object.vehicle.trailer_count = i;


            if(vehicle_object.vehicle.id == -1)
            {
                continue;
            }

            traffic_vehicle_objects[count] = vehicle_object;
            count++;
        }

        memory_handler_->write_traffic_mem(TrafficMemData{
            traffic_vehicle_objects
        });
    }

    void TrafficProcessor::get_traffic_objects_data()
    {
        auto* base_ctrl = prism::base_ctrl_u::get();
        if (base_ctrl == nullptr)
        {
            return;
        }

        auto* nearby_kdop_items = base_ctrl->get_nearby_kdop_items();
        if ( nearby_kdop_items == nullptr )
        {
            return;
        }

        // Gather traffic lights and gates from prefabs
        for (const auto *kdop_item : *nearby_kdop_items )
        {
            if (kdop_item->item_type != 4) // 4 = prefab
            {
                if (kdop_item->item_type > 4)
                    break; // items seem to be ordered by item type, so we can stop looping after going through all prefabs
                continue;
            }

            const auto prefab_kdop_item = static_cast<const prism::prefab_item_t *>(kdop_item);
            if (prefab_kdop_item->segment == nullptr)
                continue;

            for (const auto &semaphore_instance : prefab_kdop_item->segment->semaphore_instances)
            {
                if (semaphore_instance.actor == nullptr)
                {
                    continue;
                }
                this->process_traffic_object(semaphore_instance.actor, semaphore_instance.id);
            }
        }

        auto* game_traffic = prism::game_traffic_u::get();
        if (game_traffic != nullptr)
        {
            for (const auto* traffic_object : game_traffic->traffic_objects_1)
            {
                this->process_traffic_object(traffic_object);
            }
        }
    }

    void TrafficProcessor::write_semaphore_data()
    {
        std::sort(
            this->semaphore_objects.begin(),
            this->semaphore_objects.end(),
            [](const processor_semaphore_object_t& a, const processor_semaphore_object_t& b) {
                return a.distance < b.distance;
            }
        );

        std::array<SemaphoreObject, 40> semaphore_objects_data = {};
        int count = 0;
        for (const auto& obj : this->semaphore_objects)
        {
            if (count >= 40)
                break;

            auto* traffic_object = obj.traffic_object;

            SemaphoreObject return_object = {
                traffic_object->placement.pos.x,
                traffic_object->placement.pos.y,
                traffic_object->placement.pos.z,
                traffic_object->placement.cx,
                traffic_object->placement.cz,
                traffic_object->placement.rot.w,
                traffic_object->placement.rot.x,
                traffic_object->placement.rot.y,
                traffic_object->placement.rot.z,
            };
            return_object.id = obj.id;

            if (traffic_object->traffic_rule != nullptr && traffic_object->traffic_rule->get_type() == prism::ETrafficObjectType::traffic_light)
            {
                const auto* traffic_light = static_cast<const prism::traffic_light_t*>(traffic_object->traffic_rule);
                return_object.type = 1; // 1 = traffic light
                return_object.time_remaining = traffic_light->state_time_remaining;
                // 0 = off, 1 = orange_to_red, 2 = red, 4 = orange_to_green, 8 = green, 32 = sleep (blinking orange)
                return_object.state = traffic_light->state;
            }
            else // gates
            {
                // state // 0 = closing; 1 = closed; 3 = open; 2 = opening
                return_object.type = 2; // 2 = gate
                return_object.state = traffic_object->state;
                if (traffic_object->animation_mode == 2)
                {
                    return_object.time_remaining = traffic_object->open_time_remaining;
                } else {
                    return_object.time_remaining = traffic_object->animation_time_elapsed;
                }
            }

            semaphore_objects_data[count] = return_object;
            count++;
        }

        this->memory_handler_->write_semaphore_mem(SemaphoreMemData{
            semaphore_objects_data
        });
    }

    /**
     * traffic_player_vehicles and traffic_player_trailers includes:
     * - the truck and trailer(s) we're driving
     * - convoy players' truck and trailer(s)
     * - the trailers already in companies that have a job in the job market
     */
    void TrafficProcessor::get_player_traffic_data()
    {
        auto* game_traffic = prism::game_traffic_u::get();
        auto* game_actor = prism::game_actor_u::get();
        if (game_traffic == nullptr || game_actor == nullptr)
        {
            return;
        }

        std::unordered_set<uintptr_t> actors_to_ignore = {};

        // we don't want to add our own truck/trailer(s)
        actors_to_ignore.emplace(reinterpret_cast<uintptr_t>(game_actor->game_physics_vehicle));
        auto our_slave_trailer = game_actor->game_physics_vehicle->get_trailer();
        while (our_slave_trailer != nullptr)
        {
            actors_to_ignore.emplace(reinterpret_cast<uintptr_t>(our_slave_trailer));
            our_slave_trailer = our_slave_trailer->get_slave_trailer();
        }

        for (auto* player_vehicle : game_traffic->traffic_player_vehicles_1)
        {
            // TODO: pattern for traffic_player_object?
            if (actors_to_ignore.find(reinterpret_cast<uintptr_t>(player_vehicle->traffic_player_object->object)) != actors_to_ignore.end())
            {
                continue;
            }

            this->active_actors.emplace_back(
                processor_active_actor_object_t{
                    {
                        false,
                        player_vehicle->placement.get_distance_to(this->truck_pos),
                        player_vehicle,
                    },
                    player_vehicle->speed,
                    player_vehicle->acceleration,
                }
            );

            auto actor_slave = player_vehicle->slave;
            while (actor_slave != nullptr)
            {
                // we go through the trailers that are attached to the truck later, so we add them to the ignore list
                actors_to_ignore.emplace(reinterpret_cast<uintptr_t>(actor_slave));
                actor_slave = actor_slave->slave;
            }
        }

        for (auto* player_trailer : game_traffic->traffic_player_trailers_1)
        {
            // TODO: pattern for traffic_player_object?
            if (actors_to_ignore.find(reinterpret_cast<uintptr_t>(player_trailer->traffic_player_object->object)) != actors_to_ignore.end())
            {
                continue;
            }
            // any trailer that isn't in the ignore list is a trailer that isn't connected to a truck, so we add them to the parked actors list.
            this->parked_actors.emplace_back(
                processor_actor_object_t{
                    true,
                    player_trailer->placement.get_distance_to(this->truck_pos),
                    player_trailer,
                }
            );
        }
    }

    void TrafficProcessor::write_parked_vehicle_data()
    {
        std::sort(
            this->parked_actors.begin(),
            this->parked_actors.end(),
            [](const processor_actor_object_t& a, const processor_actor_object_t& b) { return a.distance < b.distance; }
        );

        std::array<ParkedVehicle, 40> parked_vehicles_data = { 0 };
        int count = 0;
        for (const auto& actor_data : this->parked_actors)
        {
            if (count >= 40)
            {
                break;
            }

            const auto position = utils::vehicle::get_center_coords(actor_data.traffic_actor->placement, actor_data.traffic_actor->aabox);
            ParkedVehicle vehicle_object = {
                position.x,
                position.y,
                position.z,
                actor_data.traffic_actor->placement.rot.w,
                actor_data.traffic_actor->placement.rot.x,
                actor_data.traffic_actor->placement.rot.y,
                actor_data.traffic_actor->placement.rot.z,
                abs(actor_data.traffic_actor->aabox.start.x - actor_data.traffic_actor->aabox.end.x), // width
                abs(actor_data.traffic_actor->aabox.start.y - actor_data.traffic_actor->aabox.end.y), // height
                abs(actor_data.traffic_actor->aabox.start.z - actor_data.traffic_actor->aabox.end.z),  // length
                get_uid_for_vehicle(reinterpret_cast<uintptr_t>(actor_data.traffic_actor)), // id
                actor_data.is_trailer
            };
            parked_vehicles_data[count] = vehicle_object;
            ++count;
        }
        this->memory_handler_->write_parked_vehicles_mem(ParkedVehiclesMemData{ parked_vehicles_data });
    }

    void TrafficProcessor::tick(scs_value_dplacement_t truck_pos)
    {
        this->truck_pos = float3_t{
            (float)truck_pos.position.x,
            (float)truck_pos.position.y,
            (float)truck_pos.position.z
        };
        this->clear_data();

        this->get_ai_traffic_data();

        this->get_player_traffic_data();
        this->get_traffic_objects_data();

        // Local\ETS2LATraffic
        this->write_active_traffic_data();
        // Local\ETS2LAParkedVehicles
        this->write_parked_vehicle_data();
        // Local\ETS2LASemaphore
        this->write_semaphore_data();
    }
}
