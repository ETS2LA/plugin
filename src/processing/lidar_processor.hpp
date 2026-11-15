#pragma once

#include "core.hpp"
#include "prism/common.hpp"
#include "thread_pool.hpp"
#include <chrono>
#include <memory>
#include <vector>

namespace ets2la_plugin
{
    class TrafficProcessor;

    class LidarProcessor
    {
    private:
        struct PrecomputedDynamicBox
        {
            float3_t center;
            float3_t halfExtents;
            quat_t invRot;
            float radiusSq;
        };

        scs_log_t _scsLog;
        CMemoryHandler* _memoryHandler;

        float3_t _mountOffset;
        float _maxRange;
        float _minRange;

        uint32_t _horizontalBeams;
        uint32_t _verticalRings;

        std::vector< float3_t > _unitRays;
        std::vector< uint32_t > _rayChannels;

        std::unique_ptr< ThreadPool > _threadPool;
        std::unique_ptr< LidarMemData > _lidarBuffer;
        std::vector< std::vector< LidarPoint > > _threadLocalHits;
        std::vector< PrecomputedDynamicBox > _activeDynamicBoxes;

        quat_t _smoothedRot;
        bool _hasSmoothedRot;

        template < class... T >
        void LogDebug( const char* fmtString, T&&... args ) const
        {
#ifdef _DEBUG
            _scsLog(
                0,
                fmt::vformat(
                    std::string( "[ets2la_plugin][LidarProcessor] " ) + fmtString,
                    fmt::make_format_args( args... )
                ).c_str()
            );
#endif
        }

        template < class... T >
        void LogInfo( const char* fmtString, T&&... args ) const
        {
            _scsLog(
                0,
                fmt::vformat(
                    std::string( "[ets2la_plugin][LidarProcessor] " ) + fmtString,
                    fmt::make_format_args( args... )
                ).c_str()
            );
        }

        template < class... T >
        void LogError( const char* fmtString, T&&... args ) const
        {
            _scsLog(
                2,
                fmt::vformat(
                    std::string( "[ets2la_plugin][LidarProcessor] " ) + fmtString,
                    fmt::make_format_args( args... )
                ).c_str()
            );
        }

        void GenerateUnitBeams();

    public:
        LidarProcessor( scs_log_t scsLog, CMemoryHandler* memoryHandler );
        ~LidarProcessor();

        void Tick( const scs_value_dplacement_t& truckPos, const TrafficProcessor* trafficProcessor = nullptr );
    };
}
