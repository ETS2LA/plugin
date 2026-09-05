#include "memory/virtual/memory_handler.hpp"
#include "core.hpp"
#include <sstream>
#include <wchar.h>

#if defined(_WIN32)

#include "Windows.h"

#elif defined(__linux__)

#include <sys/mman.h>
#include <fcntl.h> /* For O_* constants */
#include <unistd.h>

#endif


namespace ets2la_plugin
{
    CMemoryHandler::CMemoryHandler(scs_log_t scs_log) : scs_log_(scs_log)
    {
    }

    CMemoryHandler::~CMemoryHandler()
    {
        destroy();
    }

    void CMemoryHandler::unmap_file(MmapData& mmap_data, const size_t size)
    {
#if defined(_WIN32)
        if (mmap_data.mmap != nullptr)
        {
            UnmapViewOfFile(mmap_data.mmap);
            mmap_data.mmap = nullptr;
        }
        if (mmap_data.fd != nullptr && mmap_data.fd != INVALID_HANDLE_VALUE)
        {
            CloseHandle(mmap_data.fd);
            mmap_data.fd = INVALID_HANDLE_VALUE;
        }
        if (mmap_data.shm_fd != nullptr && mmap_data.shm_fd != INVALID_HANDLE_VALUE)
        {
            CloseHandle(mmap_data.shm_fd);
            mmap_data.shm_fd = INVALID_HANDLE_VALUE;
        }
#elif defined(__linux__)
        std::wstring wfile_name(mmap_data.name);
        std::string sfile_name(wfile_name.begin(), wfile_name.end());
        if (mmap_data.mmap != nullptr)
        {
            munmap(mmap_data.mmap, size);
            mmap_data.mmap = nullptr;
        }
        shm_unlink(("/" + sfile_name).c_str());
        if (mmap_data.fd != 0)
        {
            close(mmap_data.fd);
            mmap_data.fd = 0;
        }
#endif
    }

    bool CMemoryHandler::init()
    {
#if defined(_WIN32)
        // detect if running under wine/proton (https://www.winehq.org/pipermail/wine-devel/2008-September/069387.html)
        HMODULE ntdll_module = GetModuleHandle(L"ntdll.dll");
        if(ntdll_module)
        {
            this->is_running_under_wine = GetProcAddress(ntdll_module, "wine_get_version") != nullptr;
            if (this->is_running_under_wine)
            {
                this->info("Detected wine/proton, will try to create shared memory files accessible on Linux.");
            }
        }
#endif

        this->initialize_memory_file(state_mmap_data, L"ibb");
        this->initialize_memory_file(input_mmap_data, L"fbdfbd");
        this->initialize_memory_file(camera_mmap_data,L"ffffssfffffffffffffffffffffffffff");
        this->initialize_memory_file_multiple(traffic_mmap_data, L"ffffffffffffssffffffffffffffffffffffffffffff", 40);
        this->initialize_memory_file_multiple(
            mp_players_mmap_data, L"ffffffffffffssfffffffffffffffffffffffffffffflsb", 40
        );
        this->initialize_memory_file_multiple(parked_vehicles_mmap_data, L"ffffffffffsb", 40);
        this->initialize_memory_file_multiple(semaphore_mmap_data, L"fffssffffifii", 40);
        this->initialize_memory_file_multiple(route_mmap_data, L"lff", 6000);

        return true;
    }

    void CMemoryHandler::destroy()
    {
        this->unmap_file(state_mmap_data, sizeof(PluginStateData));
        this->unmap_file(input_mmap_data, sizeof(InputMemData));
        this->unmap_file(camera_mmap_data, sizeof(CameraMemData));
        this->unmap_file(traffic_mmap_data, sizeof(TrafficMemData));
        this->unmap_file(mp_players_mmap_data, sizeof(MpPlayersMemData));
        this->unmap_file(parked_vehicles_mmap_data, sizeof(ParkedVehiclesMemData));
        this->unmap_file(semaphore_mmap_data, sizeof(SemaphoreMemData));
        this->unmap_file(route_mmap_data, sizeof(RouteMemData));
    }

    // This is used to boot up the shared memory file. It creates a file mapping
    // based on the format below.
    // f - float      (4 bytes)
    // b - bool       (1 byte)
    // i - int        (4 bytes)
    // s - short      (2 bytes)
    // l - long long  (8 bytes)
    // d - double     (8 bytes)
    void CMemoryHandler::initialize_memory_file(MmapData& mmap_data, const wchar_t* format) const {
        std::wstring wformat(format);
        std::string sformat(wformat.begin(), wformat.end());
        std::wstring wfile_name(mmap_data.name);
        std::string sfile_name(wfile_name.begin(), wfile_name.end());
        this->info("Initializing shared mem file '{}' with format {}", sfile_name, sformat);

        const char float_type = 'f';
        const char boolean_type = 'b';
        const char integer_type = 'i';
        const char short_type = 's';
        const char long_long_type = 'l';
        const char double_type = 'd';

        size_t size = 0;
        for (int i = 0; format[i] != '\0'; i++)
        {
            if (format[i] == float_type)
            {
                size += sizeof(float);
            }
            else if (format[i] == boolean_type)
            {
                size += sizeof(bool);
            }
            else if (format[i] == integer_type)
            {
                size += sizeof(int);
            }
            else if (format[i] == short_type)
            {
                size += sizeof(short);
            }
            else if (format[i] == long_long_type)
            {
                size += sizeof(long long);
            }
            else if (format[i] == double_type)
            {
                size += sizeof(double);
            }
        }
#if defined(_WIN32)

        mmap_data.shm_fd = INVALID_HANDLE_VALUE;

        if (this->is_running_under_wine)
        {
            // In Linux shared-memory files use a `tmpfs` filesystem mounted at `/dev/shm`
            // But when we use the Windows functions in wine, they obviously don't do that.
            // So we manually create a file directly in `/dev/shm` and use that file handle in `CreateFileMapping`.
            const auto shm_path = L"/dev/shm/" + wfile_name;
            mmap_data.shm_fd = CreateFile(
                shm_path.c_str(),
                GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_TEMPORARY,
                NULL
            );

            if (mmap_data.shm_fd == INVALID_HANDLE_VALUE)
            {
                this->error("Failed to open file '/dev/shm/{}'", sfile_name);
            }
        }

        mmap_data.fd = CreateFileMapping(
            mmap_data.shm_fd,  // handle
            NULL,              // default security
            PAGE_READWRITE,    // read/write access
            0,                 // maximum object size (high-order DWORD)
            size,              // maximum object size (low-order DWORD)
            (L"Local\\" + wfile_name).c_str() // name of mapping object
        );

        if (mmap_data.fd == 0) {
            this->error("Failed to create file mapping for '{}'. Error code: {}", sfile_name, GetLastError());
            return;
        }

        mmap_data.mmap = MapViewOfFile(mmap_data.fd, FILE_MAP_ALL_ACCESS, 0, 0, size);

        if (mmap_data.mmap == nullptr) {
            this->error("Failed to map view of file '{}'. Error code: {}", sfile_name, GetLastError());
            return;
        }

#elif defined(__linux__)

        mmap_data.fd =  shm_open(("/" + sfile_name).c_str(), O_CREAT | O_RDWR, 0600);

        if (mmap_data.fd == -1) {
            this->error("Failed to create file mapping for '{}'. Error code: {}", sfile_name.c_str(), errno);
            return;
        }

        if (ftruncate(mmap_data.fd, size) == -1)
        {
            this->error("Failed to truncate shm '{}'. Error code: {}", sfile_name.c_str(), errno);
            return;
        }

        mmap_data.mmap = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, mmap_data.fd, 0);

        if (mmap_data.mmap == MAP_FAILED) {
            this->error("Failed to map view of file '{}'. Error code: {}", sfile_name.c_str(), errno);
            return;
        }

#endif

        memset(mmap_data.mmap, 0, size);
        this->info("Successfully opened shared mem file '{}' with size {}", sfile_name, size);
    }

    InputMemData CMemoryHandler::read_input_mem() const {
        if (input_mmap_data.mmap == nullptr) {
            this->error("Shared mem file not open.");
            return InputMemData();
        }

        InputMemData memData;
        memcpy(&memData, static_cast<char*>(input_mmap_data.mmap), sizeof(InputMemData));
        return memData;
    }

    void CMemoryHandler::write_state_mem(const PluginStateData data) const {
        if (state_mmap_data.mmap == nullptr) {
            this->error("Shared mem file not open.");
            return;
        }

        memcpy(static_cast<char*>(state_mmap_data.mmap), &data, sizeof(PluginStateData));
    }

    void CMemoryHandler::write_camera_mem(const CameraMemData data) const {
        if (camera_mmap_data.mmap == nullptr) {
            this->error("Shared mem file not open.");
            return;
        }

        memcpy(static_cast<char*>(camera_mmap_data.mmap), &data, sizeof(CameraMemData));
    }

    void CMemoryHandler::write_traffic_mem(const TrafficMemData data) const
    {
        if (traffic_mmap_data.mmap == nullptr)
        {
            this->error("Traffic shared mem file not open.");
            return;
        }

        memcpy(static_cast<char*>(traffic_mmap_data.mmap), &data, sizeof(TrafficMemData));
    }

    void CMemoryHandler::write_mp_players_mem( const MpPlayersMemData data ) const
    {
        if ( mp_players_mmap_data.mmap == nullptr )
        {
            this->error( "MP players shared mem file not open." );
            return;
        }

        memcpy( static_cast< char* >( mp_players_mmap_data.mmap ), &data, sizeof( MpPlayersMemData ) );
    }

    void CMemoryHandler::write_parked_vehicles_mem(const ParkedVehiclesMemData data) const
    {
        if (parked_vehicles_mmap_data.mmap == nullptr)
        {
            this->error("Parked vehicles shared mem file not open.");
            return;
        }

        memcpy(static_cast<char*>(parked_vehicles_mmap_data.mmap), &data, sizeof(ParkedVehiclesMemData));
    }

    void CMemoryHandler::write_semaphore_mem(const SemaphoreMemData data) const
    {
        if (semaphore_mmap_data.mmap == nullptr)
        {
            this->error("Semaphore shared mem file not open.");
            return;
        }

        memcpy(static_cast<char*>(semaphore_mmap_data.mmap), &data, sizeof(SemaphoreMemData));
    }

    void CMemoryHandler::write_route_mem(const RouteMemData data) const
    {
        if (route_mmap_data.mmap == nullptr)
        {
            this->error("Route shared mem file not open.");
            return;
        }

        memcpy(static_cast<char*>(route_mmap_data.mmap), &data, sizeof(RouteMemData));
    }

    void CMemoryHandler::initialize_memory_file_multiple(MmapData& mmap_data, const wchar_t* format, int count) const {
        wchar_t* total_format = (wchar_t*)malloc(count * (wcslen(format) + 1) * sizeof(wchar_t)); // count objects, +1 for null terminator
        if (!total_format) {
            this->error("Couldn't generate total_format");
            return;
        }

        std::wstring wformat(format);
        std::wstring wtotal_format;
        for (int i = 0; i < count; i++) {
            wtotal_format += wformat;
        }
        wcscpy(total_format, wtotal_format.c_str());

        this->initialize_memory_file(mmap_data, total_format);
        free(total_format);

    }
}
