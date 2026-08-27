#include "opengl.hpp"

#include "dlfcn.h"
#include <cstdint>

#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

#include "core.hpp"
#include "gui/gui.hpp"
#include "memory/memory_utils.hpp"
#include "patterns.hpp"

namespace ets2la_plugin
{
    // openGL stuff
    typedef int GLint; /* 4-byte signed */
    typedef void Display;
    typedef unsigned long GLXDrawable;
#define GL_ARRAY_BUFFER 0x8892
#define GL_VIEWPORT 0x0BA2
#define GL_FRAMEBUFFER 0x8D40

    static bool imgui_initialized;

    COpenGlHook::~COpenGlHook() { this->unhook(); }

    static uint64_t glXSwapBuffersTrampoline = 0;

    static ImGuiContext* ig_ctx;

    static uint64_t original_swap_buffers = 0;
    static void ( *glBindFramebufferFn )( int, int );
    static void ( *glBindBufferFn )( int, int );
    static void ( *glGetIntegervFn )( int, int* );

    static bool hk_glXSwapBuffers( Display* dpy, GLXDrawable drawable )
    {
        GLint last_array_buffer;
        glGetIntegervFn( GL_ARRAY_BUFFER, &last_array_buffer );

        if ( !imgui_initialized )
        {
            ig_ctx = ImGui::CreateContext();
            ImGui::SetCurrentContext( ig_ctx );

            ImGuiIO& io = ImGui::GetIO();
            GLint vp[ 4 ];
            glGetIntegervFn( GL_VIEWPORT, vp );

            ImGui::GetIO().DisplaySize = ImVec2( vp[ 2 ], vp[ 3 ] );

            ImGui_ImplOpenGL3_Init();
            imgui_initialized = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        CCore::g_instance->get_gui()->render();

        ImGui::Render();
        glBindFramebufferFn( GL_FRAMEBUFFER, 0 );
        ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );

        glBindBufferFn( GL_ARRAY_BUFFER, last_array_buffer );

        return ( ( bool ( * )( Display*, GLXDrawable ) )original_swap_buffers )( dpy, drawable );
    }

    bool COpenGlHook::hook()
    {
        const auto buffer_swap = memory::get_address_for_pattern(
            patterns::graphics::gl::glx_swap_buffers::pattern, patterns::graphics::gl::glx_swap_buffers::offset
        );
        if ( buffer_swap == 0 )
        {
            CCore::g_instance->error( "Failed to find glXSwapBuffer offset" );
            return false;
        }
        this->glx_swap_buffers_offset = *reinterpret_cast< uint32_t* >( buffer_swap );
        CCore::g_instance->debug( "Found glXSwapBuffers {:x}", this->glx_swap_buffers_offset );

        const auto gl_device_instance_use_addr = memory::get_address_for_pattern(
            patterns::graphics::gl::device_t::pattern, patterns::graphics::gl::device_t::offset
        );
        if ( gl_device_instance_use_addr == 0 )
        {
            CCore::g_instance->error( "Failed to find gl_device_t instance" );
            return false;
        }

        this->gl_device_ptr = memory::get_absolute_address_from_offset( gl_device_instance_use_addr );

        CCore::g_instance->debug( "Found gl_device_t instance ptr {:x}", this->gl_device_ptr );

        CCore::g_instance->info(
            "swapbuffers @ {:x}", *reinterpret_cast< uint64_t* >( this->gl_device_ptr ) + this->glx_swap_buffers_offset
        );

        original_swap_buffers = *reinterpret_cast< uint64_t* >(
            *reinterpret_cast< uint64_t* >( this->gl_device_ptr ) + this->glx_swap_buffers_offset
        );

        CCore::g_instance->info( "original_swap_buffers = {:x}", original_swap_buffers );

        *reinterpret_cast< uint64_t* >(
            *reinterpret_cast< uint64_t* >( this->gl_device_ptr ) + this->glx_swap_buffers_offset
        ) = ( uint64_t )&hk_glXSwapBuffers;

        void* opengl_handle = dlopen( "libGL.so.1", RTLD_LAZY );
        if ( !opengl_handle )
        {
            CCore::g_instance->error( "Could not get libGL handle: {}", dlerror() );
            dlclose( opengl_handle );
            return false;
        }

        glBindFramebufferFn = ( void ( * )( int, int ) )dlsym( opengl_handle, "glBindFramebuffer" );
        if ( glBindFramebufferFn == 0 )
        {
            CCore::g_instance->error( "Could find glBindFramebuffer address: {}", dlerror() );
            dlclose( opengl_handle );
            return false;
        }

        glBindBufferFn = ( void ( * )( int, int ) )dlsym( opengl_handle, "glBindBuffer" );
        if ( glBindBufferFn == 0 )
        {
            CCore::g_instance->error( "Could find glBindBuffer address: {}", dlerror() );
            dlclose( opengl_handle );
            return false;
        }
        glGetIntegervFn = ( void ( * )( int, int* ) )dlsym( opengl_handle, "glGetIntegerv" );
        if ( glGetIntegervFn == 0 )
        {
            CCore::g_instance->error( "Could find glGetIntegerv address: {}", dlerror() );
            dlclose( opengl_handle );
            return false;
        }

        return true;
    }

    bool COpenGlHook::unhook()
    {
        *reinterpret_cast< uint64_t* >(
            *reinterpret_cast< uint64_t* >( this->gl_device_ptr ) + this->glx_swap_buffers_offset
        ) = original_swap_buffers;
        if ( imgui_initialized )
        {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui::DestroyContext();
        }
        return true;
    }
}
