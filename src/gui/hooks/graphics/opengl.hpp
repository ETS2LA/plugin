#pragma once

#include "base_graphics.hpp"
#include <cstdint>

namespace ets2la_plugin
{
    class COpenGlHook : public CBaseGraphics
    {
    private:
        uint32_t glx_swap_buffers_offset = 0;
        uint64_t gl_device_ptr           = 0;

    public:
        ~COpenGlHook() override;

        bool hook() override;
        bool unhook() override;
    };
}
