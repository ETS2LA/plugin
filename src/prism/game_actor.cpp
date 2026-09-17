#include "game_actor.hpp"
#include "prism/controllers/base_ctrl.hpp"

namespace ets2la_plugin::prism
{
    game_actor_u* game_actor_u::get()
    {
        auto* base_ctrl = prism::base_ctrl_u::get();

        if ( base_ctrl == nullptr || base_ctrl_u::game_actor_offset == 0 )
        {
            return nullptr;
        }

        return *reinterpret_cast< game_actor_u** >(
            reinterpret_cast< char* >( base_ctrl ) + base_ctrl_u::game_actor_offset
        );
    }
}
