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
#include "prism/vehicles/game_physics_vehicle.hpp"
#include "prism/vehicles/game_trailer_actor.hpp"

#include <math.h>
#include <algorithm>
#include <unordered_set>

namespace ets2la_plugin
{
    // vehicle coordinates are not at the center of the vehicle, so we use the aabox to calculate the center
    float3_t TrafficProcessor::get_center_coords(const prism::placement_t& placement, const prism::aabox_t& aabox) const
    {
        const float width = abs(aabox.start.x - aabox.end.x);
        const float height = abs(aabox.start.y - aabox.end.y);
        const float length = abs(aabox.start.z - aabox.end.z);

        float3_t offset = {
            aabox.start.x + (width / 2.f),
            aabox.start.y + (height / 2.f),
            aabox.start.z + (length / 2.f),
        };

        return placement.to_global_position() + offset.rotate(placement.rot);
    }

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
        this->tmp_vehicles.clear();
        this->tmp_trailers.clear();
        this->semaphore_objects.clear();
        this->_activeDynamicBoxes.clear();
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

    void TrafficProcessor::get_truckersmp_traffic_data()
    {
        auto* game_ctrl = prism::game_ctrl_u::get();
        if (game_ctrl == nullptr)
        {
            return;
        }

        static prism::unit_descriptor_t *stored_game_trailer_actor_unit_descriptor = nullptr;
        const auto* vehicles_list = game_ctrl->get_some_nearby_non_ai_vehicles_list();
        if (vehicles_list == nullptr)
        {
            return;
        }


        auto* node = vehicles_list->begin;
        for(auto i = 0; i < vehicles_list->size && node->item != vehicles_list->empty_item; ++i, node = node->next)
        {
            const auto unit_descriptor = node->item->get_unit_descriptor();

            if (stored_game_trailer_actor_unit_descriptor == nullptr)
            {
                // check if the node item is of type 'game_trailer_actor' and store the address of the unit descriptor if it is
                // that way we can just compare the unit descriptor addresses instead of comparing strings every time
                if (strcmp(*unit_descriptor->p_class_name, "game_trailer_actor") == 0)
                {
                    stored_game_trailer_actor_unit_descriptor = unit_descriptor;
                }
            }

            const auto is_trailer = stored_game_trailer_actor_unit_descriptor == unit_descriptor;
            if (is_trailer)
            {
                prism::game_trailer_actor_u* trailer = reinterpret_cast<prism::game_trailer_actor_u *>(node->item);
                tmp_trailers.push_back(trailer);
            }
            else
            {
                auto *truck = reinterpret_cast<prism::game_physics_vehicle_u *>(node->item);
                tmp_vehicles.push_back(truck);
            }
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
            traffic_object.type = 0;
            traffic_object.traffic_actor = traffic_actor.traffic_actor;
            traffic_object.truck_distance = traffic_actor.distance;
            traffic_object.speed = traffic_actor.speed;
            traffic_object.acceleration = traffic_actor.acceleration;

            traffic_objects.push_back(traffic_object);
        }

        for (const auto& truck : this->tmp_vehicles)
        {
            processor_traffic_vehicle_object_t traffic_object;
            traffic_object.type = 1;
            traffic_object.tmp_truck = truck;

            prism::placement_t truck_placement;
            truck->get_physics_placement(&truck_placement);
            traffic_object.truck_distance = truck_placement.get_distance_to(this->truck_pos);

            traffic_objects.push_back(traffic_object);
        }

        for (const auto& trailer : this->tmp_trailers)
        {
            processor_traffic_vehicle_object_t traffic_object;
            traffic_object.type = 2;

            prism::placement_t trailer_placement;
            trailer->get_physics_placement(&trailer_placement);
            traffic_object.truck_distance = trailer_placement.get_distance_to(this->truck_pos);

            auto* trailer_obj = trailer;
            while (trailer_obj != nullptr)
            {
                traffic_object.tmp_trailers.push_back(trailer_obj);
                trailer_obj = trailer_obj->get_slave_trailer();
            }

            traffic_objects.push_back(traffic_object);
        }


        std::sort(traffic_objects.begin(), traffic_objects.end(), [](const processor_traffic_vehicle_object_t& a, const processor_traffic_vehicle_object_t& b) {
            return a.truck_distance < b.truck_distance;
        });

        // Now we need to convert this data to the format the memory uses. Once again the code
        // for TMP vehicles is convoluted, so if there's a better way to match trailers to their
        // trucks then do the same as above and ping @Tumppi066 on our Discord server or create a PR.
        // (especially figuring out a way to match the trailers to their trucks)

        std::array<TrafficVehicleObject, 40> traffic_vehicle_objects = {};
        int count = 0;
        this->_activeDynamicBoxes.clear();
        this->_activeDynamicBoxes.reserve(traffic_objects.size() * 2);

        for (const auto& traffic_object : traffic_objects)
        {
            if (count >= 40)
                break;

            TrafficVehicleObject vehicle_object = {};
            vehicle_object.vehicle.id = -1;

            if (traffic_object.type == 0) // traffic_actor (ai vehicle/convoy players/escort vehicles)
            {
                const auto* traffic_actor = traffic_object.traffic_actor;
                const auto position = get_center_coords(traffic_actor->placement, traffic_actor->aabox);
                const float halfW = abs(traffic_actor->aabox.start.x - traffic_actor->aabox.end.x) * 0.5f;
                const float halfH = abs(traffic_actor->aabox.start.y - traffic_actor->aabox.end.y) * 0.5f;
                const float halfL = abs(traffic_actor->aabox.start.z - traffic_actor->aabox.end.z) * 0.5f;
                this->_activeDynamicBoxes.push_back(DynamicBox{
                    position,
                    float3_t{ halfW, halfH, halfL },
                    traffic_actor->placement.rot
                });

                vehicle_object.vehicle = TrafficVehicle{
                    position.x,
                    position.y,
                    position.z,
                    traffic_actor->placement.rot.w,
                    traffic_actor->placement.rot.x,
                    traffic_actor->placement.rot.y,
                    traffic_actor->placement.rot.z,
                    halfW * 2.0f, // width
                    halfH * 2.0f, // height
                    halfL * 2.0f, // length
                    traffic_object.speed,          // speed
                    traffic_object.acceleration,   // acceleration
                    0,                                        // trailer_count
                    get_uid_for_vehicle(reinterpret_cast<uintptr_t>(traffic_actor)), // id
                    false,                                    // is_tmp
                    false                                     // is_trailer
                };

                auto trailer = traffic_actor->slave;
                int i = 0;
                while (trailer != nullptr && i < 3)
                {
                    const auto trailer_position = get_center_coords(trailer->placement, trailer->aabox);
                    const float tHalfW = abs(trailer->aabox.start.x - trailer->aabox.end.x) * 0.5f;
                    const float tHalfH = abs(trailer->aabox.start.y - trailer->aabox.end.y) * 0.5f;
                    const float tHalfL = abs(trailer->aabox.start.z - trailer->aabox.end.z) * 0.5f;
                    this->_activeDynamicBoxes.push_back(DynamicBox{
                        trailer_position,
                        float3_t{ tHalfW, tHalfH, tHalfL },
                        trailer->placement.rot
                    });

                    vehicle_object.trailers[i] = TrafficTrailer{
                        trailer_position.x,
                        trailer_position.y,
                        trailer_position.z,
                        trailer->placement.rot.w,
                        trailer->placement.rot.x,
                        trailer->placement.rot.y,
                        trailer->placement.rot.z,
                        tHalfW * 2.0f, // width
                        tHalfH * 2.0f, // height
                        tHalfL * 2.0f  // length
                    };

                    trailer = trailer->slave;
                    i++;
                }

                vehicle_object.vehicle.trailer_count = i;
            }
            else if (traffic_object.type == 1) // tmp vehicle
            {
                const auto* tmp_truck = traffic_object.tmp_truck;
                prism::placement_t truck_placement;
                tmp_truck->get_physics_placement(&truck_placement);
                const auto truck_position = get_center_coords(truck_placement, tmp_truck->aabox);
                const float halfW = abs(tmp_truck->aabox.start.x - tmp_truck->aabox.end.x) * 0.5f;
                const float halfH = abs(tmp_truck->aabox.start.y - tmp_truck->aabox.end.y) * 0.5f;
                const float halfL = abs(tmp_truck->aabox.start.z - tmp_truck->aabox.end.z) * 0.5f;
                this->_activeDynamicBoxes.push_back(DynamicBox{
                    truck_position,
                    float3_t{ halfW, halfH, halfL },
                    truck_placement.rot
                });

                vehicle_object.vehicle = TrafficVehicle{
                    truck_position.x,
                    truck_position.y,
                    truck_position.z,
                    truck_placement.rot.w,
                    truck_placement.rot.x,
                    truck_placement.rot.y,
                    truck_placement.rot.z,
                    halfW * 2.0f, // width
                    halfH * 2.0f, // height
                    halfL * 2.0f, // length
                    0,                               // speed (null)
                    0,                               // acceleration (null)
                    0,                               // trailer_count
                    get_uid_for_vehicle(reinterpret_cast<uintptr_t>(tmp_truck)),
                    true,                            // is_tmp
                    false                            // is_trailer
                };
            }
            else if (traffic_object.type == 2) // tmp trailer
            {
                const auto* tmp_trailer = traffic_object.tmp_trailers.empty() ? nullptr : traffic_object.tmp_trailers[0];
                if (tmp_trailer == nullptr)
                    continue;

                prism::placement_t trailer_placement;
                tmp_trailer->get_physics_placement(&trailer_placement);
                const auto trailer_position = get_center_coords(trailer_placement, tmp_trailer->aabox);
                const float halfW = abs(tmp_trailer->aabox.start.x - tmp_trailer->aabox.end.x) * 0.5f;
                const float halfH = abs(tmp_trailer->aabox.start.y - tmp_trailer->aabox.end.y) * 0.5f;
                const float halfL = abs(tmp_trailer->aabox.start.z - tmp_trailer->aabox.end.z) * 0.5f;
                this->_activeDynamicBoxes.push_back(DynamicBox{
                    trailer_position,
                    float3_t{ halfW, halfH, halfL },
                    trailer_placement.rot
                });

                vehicle_object.vehicle = TrafficVehicle{
                    trailer_position.x,
                    trailer_position.y,
                    trailer_position.z,
                    trailer_placement.rot.w,
                    trailer_placement.rot.x,
                    trailer_placement.rot.y,
                    trailer_placement.rot.z,
                    halfW * 2.0f, // width
                    halfH * 2.0f, // height
                    halfL * 2.0f, // length
                    0,                               // speed (null)
                    0,                               // acceleration (null)
                    0,                               // trailer_count
                    get_uid_for_vehicle(reinterpret_cast<uintptr_t>(tmp_trailer)),
                    true,                            // is_tmp
                    true                             // is_trailer
                };

                for (size_t i = 1; i < traffic_object.tmp_trailers.size() && i < 4; i++)
                {
                    const auto* trailer = traffic_object.tmp_trailers[i];
                    prism::placement_t trailer_placement;
                    trailer->get_physics_placement(&trailer_placement);
                    const auto trailer_position = get_center_coords(trailer_placement, trailer->aabox);
                    const float tHalfW = abs(trailer->aabox.start.x - trailer->aabox.end.x) * 0.5f;
                    const float tHalfH = abs(trailer->aabox.start.y - trailer->aabox.end.y) * 0.5f;
                    const float tHalfL = abs(trailer->aabox.start.z - trailer->aabox.end.z) * 0.5f;
                    this->_activeDynamicBoxes.push_back(DynamicBox{
                        trailer_position,
                        float3_t{ tHalfW, tHalfH, tHalfL },
                        trailer_placement.rot
                    });

                    vehicle_object.trailers[i-1] = TrafficTrailer{
                        trailer_position.x,
                        trailer_position.y,
                        trailer_position.z,
                        trailer_placement.rot.w,
                        trailer_placement.rot.x,
                        trailer_placement.rot.y,
                        trailer_placement.rot.z,
                        tHalfW * 2.0f, // width
                        tHalfH * 2.0f, // height
                        tHalfL * 2.0f  // length
                    };
                }
            }

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

            const auto position = get_center_coords(actor_data.traffic_actor->placement, actor_data.traffic_actor->aabox);
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

#if defined(_WIN32)
        this->get_truckersmp_traffic_data();
#endif

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
