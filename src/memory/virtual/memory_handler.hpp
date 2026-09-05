#pragma once

#include "fmt/core.h"
#include "scssdk_telemetry.h"
#include <array>

#if defined(_WIN32)

    typedef void *HANDLE;

#else

    typedef int HANDLE;

#endif

namespace ets2la_plugin
{
    struct PluginStateData;
    struct InputMemData;
    struct CameraMemData;
    struct TrafficMemData;
    struct MpPlayersMemData;
    struct ParkedVehiclesMemData;
    struct SemaphoreMemData;
    struct RouteMemData;

    class CMemoryHandler
    {
    private:
        struct MmapData
        {
            const wchar_t* name;
            void* mmap;    // pointer to the data | result of `MapViewOfFile` / `mmap`
            HANDLE fd;     // result of `CreateFileMapping` / `shm_open`
            HANDLE shm_fd; // wine/proton only, file handle to the /dev/shm/* file we make
        };

        MmapData state_mmap_data           = { L"ETS2LAPluginStatus" };
        MmapData input_mmap_data           = { L"ETS2LAPluginInput" };
        MmapData camera_mmap_data          = { L"ETS2LACameraProps" };
        MmapData traffic_mmap_data         = { L"ETS2LATraffic" };
        MmapData mp_players_mmap_data      = { L"ETS2LAMpPlayers" };
        MmapData parked_vehicles_mmap_data = { L"ETS2LAParkedVehicles" };
        MmapData semaphore_mmap_data       = { L"ETS2LASemaphore" };
        MmapData route_mmap_data           = { L"ETS2LARoute" };

        bool is_running_under_wine = false;

        scs_log_t scs_log_;

        template <class... T>
        void info(const char *fmt_s, T &&...args) const
        {
            scs_log_(0, fmt::vformat(std::string("[ets2la_plugin][CMemoryHandler] ") + fmt_s, fmt::make_format_args(args...)).c_str());
        }

        template <class... T>
        void error(const char *fmt_s, T &&...args) const
        {
            scs_log_(2, fmt::vformat(std::string("[ets2la_plugin] ") + fmt_s, fmt::make_format_args(args...)).c_str());
        }

        void unmap_file(MmapData& mmap_data, const size_t size);

    public:
        CMemoryHandler(scs_log_t scs_log);
        ~CMemoryHandler();

        bool init();
        void destroy();

        // initializers
        void initialize_memory_file(MmapData& mmap_data, const wchar_t* format) const;
        void initialize_memory_file_multiple(MmapData& mmap_data, const wchar_t* format, int count) const;

        // read/write
        InputMemData read_input_mem() const;
        void write_state_mem(const PluginStateData data) const;
        void write_camera_mem(const CameraMemData data) const;
        void write_traffic_mem(const TrafficMemData data) const;
        void write_mp_players_mem( const MpPlayersMemData data ) const;
        void write_parked_vehicles_mem(const ParkedVehiclesMemData data) const;
        void write_semaphore_mem(const SemaphoreMemData data) const;
        void write_route_mem(const RouteMemData data) const;
    };
}
