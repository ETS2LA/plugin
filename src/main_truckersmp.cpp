#include <TruckersMP/TruckersMP.hxx>

#include "core.hpp"
#include "fmt/core.h"
#include "prism/common.hpp"
#include "processing/mp_players.hpp"
#include "utils/vehicle.hpp"
#include "version.hpp"
#include <memory>

namespace ets2la_plugin::truckersmp
{
    prism::aabox_t TMP_bounding_box_to_aabox( TruckersMP::Bounds bounding_box )
    {
        return {
            {
                bounding_box.min.x,
                bounding_box.min.y,
                bounding_box.min.z,
            },
            {
                bounding_box.max.x,
                bounding_box.max.y,
                bounding_box.max.z,
            }
        };
    }

    static std::unique_ptr< TruckersMP::Session > g_session;

    TMP_EXPORT bool TMP_API truckersmp_init( const TruckersMP_Host* host, TruckersMP_PluginDesc* desc )
    {
        // Identify yourself. The client shows this in its plugin list and logs.
        TruckersMP::PluginInfo info;
        info.m_name        = "ETS2LA Plugin";
        info.m_author      = "ETS2LA";
        info.m_version     = ETS2LA_SDK_VERSION;
        info.m_description = "ETS2LA TruckersMP SDK.";
        TruckersMP::FillPluginDesc( desc, info );

        // Establish the SDK connection. This declares the SDK version your plugin
        // was built against and acquires every available module.
        g_session = TruckersMP::Session::Create( host );

        if ( g_session == nullptr )
        {
            // The client refused the session (for example, the plugin was built
            // against a newer SDK than the client supports). Returning false
            // refuses the load; the client unloads the DLL without calling
            // truckersmp_shutdown.
            return false;
        }

        g_session->Player().OnStreamOut.Register(
            []( TruckersMP::PlayerStreamOutEvent& e )
            {
                if ( CCore::g_instance == nullptr || CCore::g_instance->get_mp_vehicle_processor() == nullptr )
                {
                    return;
                }

                if ( !e.GetPlayer().IsValid() )
                {
                    return;
                }

                const auto steam_id = e.GetPlayer().GetSteamID();
                if ( !steam_id )
                {
                    return;
                }

                // TODO: Check if this works. Not sure if this callback is running on the main thread or if this will be
                // a cross-thread call
                CCore::g_instance->get_mp_vehicle_processor()->remove_player( *steam_id );
            }
        );

        g_session->Player().OnUpdate.Register(
            []( TruckersMP::PlayerUpdateEvent& e )
            {
                if ( CCore::g_instance == nullptr || CCore::g_instance->get_mp_vehicle_processor() == nullptr )
                {
                    return;
                }

                const auto player_handle = e.GetPlayer();
                if ( !player_handle.IsValid() )
                {
                    return;
                }

                const auto steam_id = player_handle.GetSteamID();
                if ( !steam_id )
                {
                    return;
                }

                const auto latency      = player_handle.GetNetworkLatency().value_or( 0 );
                const auto local_player = g_session->Player().GetLocalPlayer();
                if ( !local_player )
                {
                    return;
                }
                const auto has_collision = player_handle.CanCollideWith( *local_player ).value_or( true );

                const auto vehicle = player_handle.GetVehicle();
                if ( !vehicle || !vehicle->IsValid() )
                {
                    return;
                }

                MpPlayerProcessor::mp_player_object_t player_object = { 0 };
                player_object.steam_id                              = *steam_id;
                player_object.latency                               = latency;
                player_object.has_collision                         = has_collision;


                // TODO: Check if this works. Not sure if this callback is running on the main thread or if this will be
                // a cross-thread call
                CCore::g_instance->get_mp_vehicle_processor()->update_player( *steam_id, player_object );
            }
        );

        return true;
    }

    TMP_EXPORT void TMP_API truckersmp_shutdown( void )
    {
        // Destroying the session unregisters every listener.
        g_session.reset();
    }

}
