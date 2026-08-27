#include "x11.hpp"

#include "core.hpp"
#include "gui/gui.hpp"
#include "memory/memory_utils.hpp"
#include "patterns.hpp"
#include <memory>

#include "imgui.h"
#include <polyhook2/Detour/x64Detour.hpp>

#include "X11/Xlib.h"
#include "X11/keysym.h"
#include <X11/XKBlib.h>

namespace ets2la_plugin
{

    static std::unique_ptr< PLH::x64Detour > handle_key_input_hook = nullptr;
    static uint64_t handle_key_input_trampoline                    = 0;

    static std::unique_ptr< PLH::x64Detour > handle_button_input_hook = nullptr;
    static uint64_t handle_button_input_trampoline                    = 0;

    static std::unique_ptr< PLH::x64Detour > handle_pointer_motion_hook = nullptr;
    static uint64_t handle_pointer_motion_trampoline                    = 0;

    // https://github.com/Nemirtingas/imgui/blob/my_master/backends/imgui_impl_x11.cpp
    static ImGuiKey virtual_key_to_imgui_key( uint32_t param )
    {
        switch ( param )
        {
        case XK_Tab:
            return ImGuiKey_Tab;
        case XK_ISO_Left_Tab:
            return ImGuiKey_Tab;
        case XK_Left:
        case XK_KP_Left:
            return ImGuiKey_LeftArrow;
        case XK_Right:
        case XK_KP_Right:
            return ImGuiKey_RightArrow;
        case XK_Up:
        case XK_KP_Up:
            return ImGuiKey_UpArrow;
        case XK_Down:
        case XK_KP_Down:
            return ImGuiKey_DownArrow;
        case XK_Prior:
        case XK_KP_Page_Up:
            return ImGuiKey_PageUp;
        case XK_Next:
        case XK_KP_Page_Down:
            return ImGuiKey_PageDown;
        case XK_Home:
        case XK_KP_Home:
        case XK_KP_Begin:
            return ImGuiKey_Home;
        case XK_End:
        case XK_KP_End:
            return ImGuiKey_End;
        case XK_Insert:
        case XK_KP_Insert:
            return ImGuiKey_Insert;
        case XK_Delete:
            return ImGuiKey_Delete;
        case XK_BackSpace:
            return ImGuiKey_Backspace;
        case XK_space:
            return ImGuiKey_Space;
        case XK_Return:
            return ImGuiKey_Enter;
        case XK_Escape:
            return ImGuiKey_Escape;
        case XK_apostrophe:
            return ImGuiKey_Apostrophe;
        case XK_comma:
            return ImGuiKey_Comma;
        case XK_minus:
            return ImGuiKey_Minus;
        case XK_period:
            return ImGuiKey_Period;
        case XK_slash:
            return ImGuiKey_Slash;
        case XK_semicolon:
            return ImGuiKey_Semicolon;
        case XK_equal:
            return ImGuiKey_Equal;
        case XK_bracketleft:
            return ImGuiKey_LeftBracket;
        case XK_backslash:
            return ImGuiKey_Backslash;
        case XK_bracketright:
            return ImGuiKey_RightBracket;
        case XK_grave:
            return ImGuiKey_GraveAccent;
        case XK_Caps_Lock:
            return ImGuiKey_CapsLock;
        case XK_Scroll_Lock:
            return ImGuiKey_ScrollLock;
        case XK_Num_Lock:
            return ImGuiKey_NumLock;
        case XK_Print:
            return ImGuiKey_PrintScreen;
        case XK_Pause:
            return ImGuiKey_Pause;
        case XK_KP_0:
            return ImGuiKey_Keypad0;
        case XK_KP_1:
            return ImGuiKey_Keypad1;
        case XK_KP_2:
            return ImGuiKey_Keypad2;
        case XK_KP_3:
            return ImGuiKey_Keypad3;
        case XK_KP_4:
            return ImGuiKey_Keypad4;
        case XK_KP_5:
            return ImGuiKey_Keypad5;
        case XK_KP_6:
            return ImGuiKey_Keypad6;
        case XK_KP_7:
            return ImGuiKey_Keypad7;
        case XK_KP_8:
            return ImGuiKey_Keypad8;
        case XK_KP_9:
            return ImGuiKey_Keypad9;
        case XK_KP_Decimal:
            return ImGuiKey_KeypadDecimal;
        case XK_KP_Divide:
            return ImGuiKey_KeypadDivide;
        case XK_KP_Multiply:
            return ImGuiKey_KeypadMultiply;
        case XK_KP_Subtract:
            return ImGuiKey_KeypadSubtract;
        case XK_KP_Add:
            return ImGuiKey_KeypadAdd;
        case XK_KP_Enter:
            return ImGuiKey_KeypadEnter;
        case XK_Shift_L:
            return ImGuiKey_LeftShift;
        case XK_Control_L:
            return ImGuiKey_LeftCtrl;
        case XK_Alt_L:
            return ImGuiKey_LeftAlt;
        case XK_Super_L:
            return ImGuiKey_LeftSuper;
        case XK_Shift_R:
            return ImGuiKey_RightShift;
        case XK_Control_R:
            return ImGuiKey_RightCtrl;
        case XK_Alt_R:
            return ImGuiKey_RightAlt;
        case XK_Super_R:
            return ImGuiKey_RightSuper;
        // case XK_APPS: return ImGuiKey_Menu;
        case XK_0:
            return ImGuiKey_0;
        case XK_1:
            return ImGuiKey_1;
        case XK_2:
            return ImGuiKey_2;
        case XK_3:
            return ImGuiKey_3;
        case XK_4:
            return ImGuiKey_4;
        case XK_5:
            return ImGuiKey_5;
        case XK_6:
            return ImGuiKey_6;
        case XK_7:
            return ImGuiKey_7;
        case XK_8:
            return ImGuiKey_8;
        case XK_9:
            return ImGuiKey_9;
        case XK_a:
        case XK_A:
            return ImGuiKey_A;
        case XK_b:
        case XK_B:
            return ImGuiKey_B;
        case XK_c:
        case XK_C:
            return ImGuiKey_C;
        case XK_d:
        case XK_D:
            return ImGuiKey_D;
        case XK_e:
        case XK_E:
            return ImGuiKey_E;
        case XK_f:
        case XK_F:
            return ImGuiKey_F;
        case XK_g:
        case XK_G:
            return ImGuiKey_G;
        case XK_h:
        case XK_H:
            return ImGuiKey_H;
        case XK_i:
        case XK_I:
            return ImGuiKey_I;
        case XK_j:
        case XK_J:
            return ImGuiKey_J;
        case XK_k:
        case XK_K:
            return ImGuiKey_K;
        case XK_l:
        case XK_L:
            return ImGuiKey_L;
        case XK_m:
        case XK_M:
            return ImGuiKey_M;
        case XK_n:
        case XK_N:
            return ImGuiKey_N;
        case XK_o:
        case XK_O:
            return ImGuiKey_O;
        case XK_p:
        case XK_P:
            return ImGuiKey_P;
        case XK_q:
        case XK_Q:
            return ImGuiKey_Q;
        case XK_r:
        case XK_R:
            return ImGuiKey_R;
        case XK_s:
        case XK_S:
            return ImGuiKey_S;
        case XK_t:
        case XK_T:
            return ImGuiKey_T;
        case XK_u:
        case XK_U:
            return ImGuiKey_U;
        case XK_v:
        case XK_V:
            return ImGuiKey_V;
        case XK_w:
        case XK_W:
            return ImGuiKey_W;
        case XK_x:
        case XK_X:
            return ImGuiKey_X;
        case XK_y:
        case XK_Y:
            return ImGuiKey_Y;
        case XK_z:
        case XK_Z:
            return ImGuiKey_Z;
        case XK_F1:
            return ImGuiKey_F1;
        case XK_F2:
            return ImGuiKey_F2;
        case XK_F3:
            return ImGuiKey_F3;
        case XK_F4:
            return ImGuiKey_F4;
        case XK_F5:
            return ImGuiKey_F5;
        case XK_F6:
            return ImGuiKey_F6;
        case XK_F7:
            return ImGuiKey_F7;
        case XK_F8:
            return ImGuiKey_F8;
        case XK_F9:
            return ImGuiKey_F9;
        case XK_F10:
            return ImGuiKey_F10;
        case XK_F11:
            return ImGuiKey_F11;
        case XK_F12:
            return ImGuiKey_F12;
        default:
            return ImGuiKey_None;
        }
    }

    static int get_virtual_key_char( int vk )
    {
        if ( vk < 256 )
        {
            return vk;
        }

        switch ( vk )
        {
        case XK_KP_0:
            return '0';
        case XK_KP_1:
            return '1';
        case XK_KP_2:
            return '2';
        case XK_KP_3:
            return '3';
        case XK_KP_4:
            return '4';
        case XK_KP_5:
            return '5';
        case XK_KP_6:
            return '6';
        case XK_KP_7:
            return '7';
        case XK_KP_8:
            return '8';
        case XK_KP_9:
            return '9';
        }

        return 0;
    }

    static bool GetKeyState( Display* hDisplay, int keysym, char keys[ 32 ] )
    {
        int keycode = XKeysymToKeycode( hDisplay, keysym );
        return keys[ keycode / 8 ] & ( 1 << keycode % 8 );
    }

    static void update_key_modifiers( XEvent* event )
    {
        bool k;
        char szKey[ 32 ];
        XQueryKeymap( event->xkey.display, szKey );

        auto& io = ImGui::GetIO();

        io.AddKeyEvent(
            ImGuiMod_Ctrl,
            GetKeyState( event->xkey.display, XK_Control_L, szKey ) ||
                GetKeyState( event->xkey.display, XK_Control_R, szKey )
        );
        io.AddKeyEvent(
            ImGuiMod_Shift,
            GetKeyState( event->xkey.display, XK_Shift_L, szKey ) ||
                GetKeyState( event->xkey.display, XK_Shift_R, szKey )
        );
        io.AddKeyEvent(
            ImGuiMod_Alt,
            GetKeyState( event->xkey.display, XK_Alt_L, szKey ) || GetKeyState( event->xkey.display, XK_Alt_R, szKey )
        );
        io.AddKeyEvent(
            ImGuiMod_Super,
            GetKeyState( event->xkey.display, XK_Super_L, szKey ) ||
                GetKeyState( event->xkey.display, XK_Super_R, szKey )
        );
    }

    static uint64_t hk_handle_key_input( void* dev, XEvent* event )
    {
        auto& io = ImGui::GetIO();

        const bool is_key_down = event->type == KeyPress;
        int ksym               = XkbKeycodeToKeysym(
            event->xkey.display, event->xkey.keycode, 0, event->xkey.state & ( ShiftMask | LockMask ) ? ShiftMask : 0
        );

        if ( ( event->xkey.state & Mod2Mask ) && ksym >= XK_KP_Space && ksym <= XK_KP_9 ) // KeyPad keys
        {
            ksym = XkbKeycodeToKeysym( event->xkey.display, event->xkey.keycode, 0, ShiftMask );
        }
        if ( ksym == NoSymbol )
        {
            return ksym;
        }
        if ( !is_key_down )
        {
            if ( ksym == XK_Delete )
            {
                CCore::g_instance->info( "pressed delete key!" );
                CCore::g_instance->get_gui()->toggle_gui();
            }
            else if ( ksym == XK_Insert )
            {
                CCore::g_instance->info( "pressed insert key!" );
                CCore::g_instance->get_gui()->toggle_input();
            }
        }

        if ( !CCore::g_instance->get_gui()->is_input_hijacked() )
        {
            return PLH::FnCast( handle_key_input_trampoline, hk_handle_key_input )( dev, event );
        }

        if ( ksym >= 0x1000100 && ksym <= 0x110ffff )
        {
            if ( is_key_down )
            {
                io.AddInputCharacterUTF16( ksym );
            }
        }
        else
        {
            // Submit modifiers
            update_key_modifiers( event );

            const ImGuiKey key = virtual_key_to_imgui_key( ksym );
            if ( key != ImGuiKey_None )
            {
                io.AddKeyEvent( key, is_key_down );
            }

            if ( is_key_down )
            {
                int keyChar = get_virtual_key_char( ksym );
                if ( keyChar != 0 )
                {
                    io.AddInputCharacter( keyChar );
                }
            }
        }

        return 0;
    }

    static uint64_t hk_handle_button_input( void* dev, XEvent* event )
    {
        if ( !CCore::g_instance->get_gui()->is_input_hijacked() )
        {
            return PLH::FnCast( handle_button_input_trampoline, hk_handle_button_input )( dev, event );
        }

        auto& io               = ImGui::GetIO();
        const bool is_key_down = event->type == ButtonPress;
        switch ( event->xbutton.button )
        {
        case Button1:
            io.AddMouseButtonEvent( ImGuiMouseButton_Left, is_key_down );
            break;

        case Button2:
            io.AddMouseButtonEvent( ImGuiMouseButton_Middle, is_key_down );
            break;

        case Button3:
            io.AddMouseButtonEvent( ImGuiMouseButton_Right, is_key_down );
            break;

        case Button4: // Mouse wheel up
            if ( is_key_down )
            {
                io.AddMouseWheelEvent( 0, 1 );
            }
            break;

        case Button5: // Mouse wheel down
            if ( is_key_down )
            {
                io.AddMouseWheelEvent( 0, -1 );
            }
            break;
        }

        return 0;
    }

    static uint64_t hk_handle_pointer_motion( void* dev, XEvent* event )
    {
        if ( !CCore::g_instance->get_gui()->is_input_hijacked() )
        {
            return PLH::FnCast( handle_pointer_motion_trampoline, hk_handle_pointer_motion )( dev, event );
        }

        auto& io      = ImGui::GetIO();
        io.MousePos.x = static_cast< float >( static_cast< int >( event->xkey.x ) );
        io.MousePos.y = static_cast< float >( static_cast< int >( event->xkey.y ) );

        return 0;
    }

    CX11Hook::~CX11Hook() { this->unhook(); }

    bool CX11Hook::hook()
    {
        if ( !this->hook_key_input() )
        {
            CCore::g_instance->error( "Could not hook key input" );
            return false;
        }

        if ( !this->hook_button_input() )
        {
            CCore::g_instance->error( "Could not hook button input" );
            return false;
        }

        if ( !this->hook_mouse_motion() )
        {
            CCore::g_instance->error( "Could not hook mouse motion" );
            return false;
        }

        return true;
    }

    bool CX11Hook::hook_key_input()
    {
        const auto handle_key_input_call_address = memory::get_address_for_pattern(
            patterns::input::x11::key_input::pattern, patterns::input::x11::key_input::offset
        );

        if ( handle_key_input_call_address == 0 )
        {
            CCore::g_instance->error( "Could not find key input handle address" );
            return false;
        }

        const auto handle_key_input_fn_address =
            handle_key_input_call_address + 4 + *reinterpret_cast< int32_t* >( handle_key_input_call_address );

        CCore::g_instance->info(
            "Creating handle_key_input hook @ {:x}", memory::as_offset( handle_key_input_fn_address )
        );
        handle_key_input_hook = std::make_unique< PLH::x64Detour >(
            handle_key_input_fn_address, ( uint64_t )hk_handle_key_input, &handle_key_input_trampoline
        );

        CCore::g_instance->info( "Hooking handle_key_input" );
        if ( !handle_key_input_hook->hook() )
        {
            CCore::g_instance->error( "Could not hook handle_key_input" );
            return false;
        }
        return true;
    }

    bool CX11Hook::hook_button_input()
    {
        const auto handle_button_input_call_address = memory::get_address_for_pattern(
            patterns::input::x11::button_input::pattern, patterns::input::x11::button_input::offset
        );

        if ( handle_button_input_call_address == 0 )
        {
            CCore::g_instance->error( "Could not find button input handle address" );
            return false;
        }

        const auto handle_button_input_fn_address =
            handle_button_input_call_address + 4 + *reinterpret_cast< int32_t* >( handle_button_input_call_address );

        CCore::g_instance->info( "Creating handle_button_input hook @ {:x}", handle_button_input_fn_address );
        handle_button_input_hook = std::make_unique< PLH::x64Detour >(
            handle_button_input_fn_address, ( uint64_t )hk_handle_button_input, &handle_button_input_trampoline
        );

        CCore::g_instance->info( "Hooking handle_button_input" );
        if ( !handle_button_input_hook->hook() )
        {
            CCore::g_instance->error( "Could not hook handle_button_input" );
            return false;
        }
        return true;
    }

    bool CX11Hook::hook_mouse_motion()
    {
        const auto handle_pointer_motion_call_address = memory::get_address_for_pattern(
            patterns::input::x11::pointer_motion_input::pattern, patterns::input::x11::pointer_motion_input::offset
        );

        if ( handle_pointer_motion_call_address == 0 )
        {
            CCore::g_instance->error( "Could not find pointer motion handle address" );
            return false;
        }

        const auto handle_pointer_motion_fn_address =
            handle_pointer_motion_call_address + 4 +
            *reinterpret_cast< int32_t* >( handle_pointer_motion_call_address );

        CCore::g_instance->info(
            "Creating handle_pointer_motion hook @ {:x}", memory::as_offset( handle_pointer_motion_fn_address )
        );
        handle_pointer_motion_hook = std::make_unique< PLH::x64Detour >(
            handle_pointer_motion_fn_address, ( uint64_t )hk_handle_pointer_motion, &handle_pointer_motion_trampoline
        );

        CCore::g_instance->info( "Hooking handle_pointer_motion" );
        if ( !handle_pointer_motion_hook->hook() )
        {
            CCore::g_instance->error( "Could not hook handle_pointer_motion" );
            return false;
        }
        return true;
    }

    bool CX11Hook::unhook()
    {
        return handle_key_input_hook->unHook() | handle_button_input_hook->unHook() |
               handle_pointer_motion_hook->unHook();
    }
}
