#pragma once

#include "fmt/core.h"
#include "scssdk_telemetry.h"
#include <string>

namespace ets2la_plugin
{
    class FfbProcessor
    {
    private:
        scs_log_t scs_log_;

        template <class... T>
        void info(const char* fmt_s, T&&... args) const
        {
            if (scs_log_)
            {
                scs_log_(0, fmt::vformat(std::string("[ets2la_plugin][FfbProcessor] ") + fmt_s, fmt::make_format_args(args...)).c_str());
            }
        }

        template <class... T>
        void warning(const char* fmt_s, T&&... args) const
        {
            if (scs_log_)
            {
                scs_log_(1, fmt::vformat(std::string("[ets2la_plugin][FfbProcessor] ") + fmt_s, fmt::make_format_args(args...)).c_str());
            }
        }

        template <class... T>
        void error(const char* fmt_s, T&&... args) const
        {
            if (scs_log_)
            {
                scs_log_(2, fmt::vformat(std::string("[ets2la_plugin][FfbProcessor] ") + fmt_s, fmt::make_format_args(args...)).c_str());
            }
        }

    public:
        explicit FfbProcessor(scs_log_t scs_log);
        ~FfbProcessor();

        bool init(const scs_telemetry_init_params_v101_t* init_params);
        void destroy();

        // Called every frame from CCore::tick() to log hook/servo events and handle faults
        void tick();

        // Returns true when a force-feedback wheel is hooked, receiving constant-force updates from the game,
        // and receiving fresh steering channel readings so FFB can turn the wheel instead of memory editing.
        bool is_available() const;

        // Sets the target steering angle (-1..1 of lock, counterclockwise positive) and whether ETS2LA is overriding steering
        void set_steering_target(float target_angle, bool engaged);

        // Synchronizes the in-game vehicle steering angle at the game's high-frequency FFB tick rate
        void set_vehicle_steering_ptr(float* ptr);

        // SDK callbacks
        void on_steering_input(float value);
        void on_drive_state(bool driving);
    };
}
