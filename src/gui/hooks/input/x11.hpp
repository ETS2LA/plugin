#pragma once

#include "base_input.hpp"

namespace ets2la_plugin
{
    class CX11Hook : public CBaseInput
    {
    private:
        bool hook_key_input();
        bool hook_button_input();
        bool hook_mouse_motion();

    public:
        ~CX11Hook() override;

        bool hook() override;
        bool unhook() override;
    };
}
