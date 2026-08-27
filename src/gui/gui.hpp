#pragma once

namespace ets2la_plugin
{
    class CGui
    {
        bool hijack_input_    = false;
        bool should_show_gui_ = false;

        class CBaseGraphics* graphics_hook_;
        class CBaseInput* input_hook_;

        class CTraffic* traffic_gui_;

    public:
        ~CGui();

        bool toggle_gui();
        bool toggle_input();

        inline bool is_input_hijacked() const { return hijack_input_; }
        inline bool should_show_gui() const { return should_show_gui_; }

        void init();
        void destroy();
        void render();
    };

}
