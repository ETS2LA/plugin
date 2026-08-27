#include "gui.hpp"

#include "gui/windows/traffic.hpp"
#include "hooks/graphics/opengl.hpp"
#include "hooks/input/x11.hpp"
#include "imgui.h"
#include <stdexcept>

namespace ets2la_plugin
{
    CGui::~CGui() { this->destroy(); }

    void CGui::destroy()
    {
        delete this->graphics_hook_;
        delete this->input_hook_;
        delete this->traffic_gui_;
    }

    bool CGui::toggle_gui()
    {
        this->should_show_gui_ = !this->should_show_gui_;
        return this->should_show_gui_;
    }

    bool CGui::toggle_input()
    {
        this->hijack_input_ = !this->hijack_input_;
        auto& io            = ImGui::GetIO();
        io.MouseDrawCursor  = this->hijack_input_;
        return this->hijack_input_;
    }

    void CGui::init()
    {
        // TODO: add other render options and logic to detect which one is active
        this->graphics_hook_ = new COpenGlHook();
        if ( !this->graphics_hook_->hook() )
        {
            throw std::runtime_error( "Failed to hook OpenGL" );
        }
        this->input_hook_ = new CX11Hook();
        if ( !this->input_hook_->hook() )
        {
            throw std::runtime_error( "Failed to hook X11" );
        }

        this->traffic_gui_ = new CTraffic();
    }

    void CGui::render()
    {
        ImGui::ShowDemoWindow();
        this->traffic_gui_->render();
    }
}
