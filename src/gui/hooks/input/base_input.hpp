#pragma once

namespace ets2la_plugin
{
    class CBaseInput
    {
    public:
        virtual ~CBaseInput() = default;

        virtual bool hook()   = 0;
        virtual bool unhook() = 0;
    };
}
