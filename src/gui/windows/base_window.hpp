#pragma once

namespace ets2la_plugin
{

    class CBaseWindow
    {
    public:
        virtual ~CBaseWindow() = default;
        virtual void render()  = 0;
    };

}
