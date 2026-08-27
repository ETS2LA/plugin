#pragma once

#include "scssdk_telemetry.h"
#include "fmt/core.h"
#include "array"

#include "memory/virtual/memory_handler.hpp"
#include "processing/traffic.hpp"

namespace ets2la_plugin
{

#pragma pack(push, 1)

    struct PluginStateData
    {
        int version;
        bool steering_overridden;
        bool acceleration_overridden;
    };
    static_assert(sizeof(PluginStateData) == 6);

    struct InputMemData
    {
        float steering;                // 0
        bool override_steering;        // 4
        double steering_timestamp;     // 5
        float acceleration;            // 13
        bool override_acceleration;    // 17
        double acceleration_timestamp; // 18
                                       // 26
    };
    static_assert(sizeof(InputMemData) == 26);

    struct CameraMemData
    {
        float fov;   // 0
        float pos_x; // 4
        float pos_y; // 8
        float pos_z; // 12
        int16_t cx;  // 16
        int16_t cz;  // 18
        float qw;    // 20
        float qx;    // 24
        float qy;    // 28
        float qz;    // 32
        // 4x4 projection matrix
        float m11;   // 36
        float m12;   // 40
        float m13;   // 44
        float m14;   // 48
        float m21;   // 52
        float m22;   // 56
        float m23;   // 60
        float m24;   // 64
        float m31;   // 68
        float m32;   // 72
        float m33;   // 76
        float m34;   // 80
        float m41;   // 84
        float m42;   // 88
        float m43;   // 92
        float m44;   // 96
        float truck_pos_x; // 100
        float truck_pos_y; // 104
        float truck_pos_z; // 108
        float truck_rot_w; // 112
        float truck_rot_x; // 116
        float truck_rot_y; // 120
        float truck_rot_z; // 124
                           // 128
    };
    static_assert(sizeof(CameraMemData) == 128);

    struct TrafficVehicle
    {
        float x;               // 0
        float y;               // 4
        float z;               // 8
        float qw;              // 12
        float qx;              // 16
        float qy;              // 20
        float qz;              // 24
        float width;           // 28
        float height;          // 32
        float length;          // 36
        float speed;           // 40
        float acceleration;    // 44
        short trailer_count;   // 48
        short id;              // 50
        bool is_tmp;           // 52
        bool is_trailer;       // 53
                               // 54
    };

    struct TrafficTrailer
    {
        float x;        // 0
        float y;        // 4
        float z;        // 8
        float qw;       // 12
        float qx;       // 16
        float qy;       // 20
        float qz;       // 24
        float width;    // 28
        float height;   // 32
        float length;   // 36
                        // 40
    };

    struct TrafficVehicleObject
    {
        TrafficVehicle vehicle;      // 0
        TrafficTrailer trailers[3];  // 54
                                     // 174
    };

    struct TrafficMemData
    {
        std::array<TrafficVehicleObject, 40> vehicles; // 0
                                                       // 6800
    };
    static_assert(sizeof(TrafficMemData) == 6960);

    struct ParkedVehicle
    {
        float x;               // 0
        float y;               // 4
        float z;               // 8
        float qw;              // 12
        float qx;              // 16
        float qy;              // 20
        float qz;              // 24
        float width;           // 28
        float height;          // 32
        float length;          // 36
        short id;              // 40
        bool is_trailer;       // 42
                               // 43
    };

    struct ParkedVehiclesMemData
    {
        std::array<ParkedVehicle, 40> parked_vehicles; // 0
                                                       // 1720
    };
    static_assert(sizeof(ParkedVehiclesMemData) == 1720);

    struct SemaphoreObject
    {
        float x;                // 0
        float y;                // 4
        float z;                // 8
        short cx;               // 12
        short cz;               // 14
        float qw;               // 16
        float qx;               // 20
        float qy;               // 24
        float qz;               // 28
        int type;               // 32
        float time_remaining;   // 36
        int state;              // 40
        int id;                 // 44
                                // 48
    };

    struct SemaphoreMemData
    {
        std::array<SemaphoreObject, 40> semaphores; // 1920
    };
    static_assert(sizeof(SemaphoreMemData) == 1920);

    struct RouteTaskObject
    {
        long long uid;  // 0
        float distance; // 8
        float time;     // 12
                        // 16
    };

    struct RouteMemData
    {
        std::array<RouteTaskObject, 6000> tasks; // 96 000
    };
    static_assert(sizeof(RouteMemData) == 96'000);

#pragma pack(pop)

    class CCore
    {
    private:
        const scs_telemetry_init_params_v101_t *init_params_;
        scs_log_t scs_log_;

        mutable size_t last_route_length_{0};
        mutable double steering_start_time = 0.0;

        CMemoryHandler *memory_manager_;
        TrafficProcessor *traffic_processor_;

#ifdef INCLUDE_GUI
        class CGui* gui_;
#endif

        mutable bool was_overriding_acceleration = false;
        mutable bool was_overriding_steering = false;

        bool scan_for_required_patterns();

    public:
        scs_value_dplacement_t truck_pos;
        static CCore *g_instance;

        CCore(const scs_telemetry_init_params_v101_t *init_params);
        ~CCore();

        void get_camera_data() const;

        void get_navigation_data() const;

        void override_inputs() const;
        void update_plugin_state() const;

        bool init();
        void destroy();

        void tick() const;

        TrafficProcessor *get_traffic_processor() const { return this->traffic_processor_; }
        CMemoryHandler *get_memory_manager() const { return this->memory_manager_; }

#if INCLUDE_GUI
        inline CGui* get_gui() const { return this->gui_; }
#endif

        template <class... T>
        void debug(const char *fmt_s, T &&...args) const
        {
#ifdef _DEBUG
            scs_log_(0, fmt::vformat(std::string("[ets2la_plugin] ") + fmt_s, fmt::make_format_args(args...)).c_str());
#endif
        }

        template <class... T>
        void info(const char *fmt_s, T &&...args) const
        {
            scs_log_(0, fmt::vformat(std::string("[ets2la_plugin] ") + fmt_s, fmt::make_format_args(args...)).c_str());
        }

        template <class... T>
        void warning(const char *fmt_s, T &&...args) const
        {
            scs_log_(1, fmt::vformat(std::string("[ets2la_plugin] ") + fmt_s, fmt::make_format_args(args...)).c_str());
        }

        template <class... T>
        void error(const char *fmt_s, T &&...args) const
        {
            scs_log_(2, fmt::vformat(std::string("[ets2la_plugin] ") + fmt_s, fmt::make_format_args(args...)).c_str());
        }
    };
}
