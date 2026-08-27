#pragma once

namespace ets2la_plugin
{
    class CBaseGraphics
    {
    public:
        virtual ~CBaseGraphics() = default;

        virtual bool hook()   = 0;
        virtual bool unhook() = 0;
    };
}
