#include "lidar_processor.hpp"
#include "traffic.hpp"

#include "prism/controllers/base_ctrl.hpp"
#include "prism/game_actor.hpp"
#include "prism/physics/physics_scene_physx.hpp"
#include "prism/vehicles/game_physics_vehicle.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

#if defined(_MSC_VER) && defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace ets2la_plugin
{
    namespace
    {
        constexpr float PI = 3.14159265358979323846f;
        constexpr float DEG2RAD = PI / 180.0f;

        bool SafeRaycast(
            prism::physx::PxScene* scene,
            const prism::physx::PxVec3& origin,
            const prism::physx::PxVec3& dir,
            float maxDist,
            prism::physx::PxRaycastBuffer& hitBuffer,
            prism::physx::PxHitFlags hitFlags,
            const prism::physx::PxQueryFilterData& filterData,
            uint32_t vtableOffset
        )
        {
#if defined(_MSC_VER) && defined(_WIN32)
            __try
            {
                return scene->raycast( origin, dir, maxDist, hitBuffer, hitFlags, filterData, nullptr, nullptr, vtableOffset );
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                return false;
            }
#else
            return scene->raycast( origin, dir, maxDist, hitBuffer, hitFlags, filterData, nullptr, nullptr, vtableOffset );
#endif
        }

        inline quat_t NLerp( const quat_t& q1, const quat_t& q2, float t )
        {
            float dot = q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;
            float sign = ( dot < 0.0f ) ? -1.0f : 1.0f;

            quat_t res{
                q1.w + t * ( q2.w * sign - q1.w ),
                q1.x + t * ( q2.x * sign - q1.x ),
                q1.y + t * ( q2.y * sign - q1.y ),
                q1.z + t * ( q2.z * sign - q1.z )
            };

            float len = std::sqrt( res.w * res.w + res.x * res.x + res.y * res.y + res.z * res.z );
            if ( len > 1e-6f )
            {
                float inv = 1.0f / len;
                res.w *= inv;
                res.x *= inv;
                res.y *= inv;
                res.z *= inv;
            }
            return res;
        }

        inline float ComputeClearLaunchDistance( const float3_t& localDir, float rearClearDist )
        {
            if ( localDir.y > 0.15f )
            {
                return 0.8f;
            }

            constexpr float halfWidth = 1.45f;
            const float absX = std::abs( localDir.x );
            const float tx = ( absX > 1e-4f ) ? ( halfWidth / absX ) : 999.0f;

            const float tfwd = ( localDir.z < -0.05f ) ? ( 0.55f / ( -localDir.z ) ) : 999.0f;
            const float trear = ( localDir.z > 0.05f ) ? ( ( rearClearDist + 2.2f ) / localDir.z ) : 999.0f;

            const float tExit = ( localDir.z < 0.0f ) ? ( (std::min)( tx, tfwd ) ) : ( (std::min)( tx, trear ) );
            return ( std::max )( 0.8f, ( std::min )( tExit + 0.15f, 22.0f ) );
        }
    }

    LidarProcessor::LidarProcessor( scs_log_t scsLog, CMemoryHandler* memoryHandler )
        : _scsLog( scsLog ),
          _memoryHandler( memoryHandler ),
          _mountOffset{ 0.0f, 3.55f, -2.2f },
          _maxRange( 120.0f ),
          _minRange( 0.8f ),
          _horizontalBeams( 512 ),
          _verticalRings( 128 ),
          _smoothedRot{ 1.0f, 0.0f, 0.0f, 0.0f },
          _hasSmoothedRot( false )
    {
        const unsigned int hwThreads = std::thread::hardware_concurrency();
        const size_t numThreads = std::max( 2u, std::min( 16u, hwThreads > 0 ? hwThreads : 8u ) );
        _threadPool = std::make_unique< ThreadPool >( numThreads );

        _threadLocalHits.resize( numThreads );
        for ( auto& hits : _threadLocalHits )
        {
            hits.reserve( 8192 );
        }

        _lidarBuffer = std::make_unique< LidarMemData >();

        GenerateUnitBeams();
        LogInfo(
            "Lidar initiated with {} rays ({} rings x {} beams, range: {}m) across {} worker threads",
            _unitRays.size(),
            _verticalRings,
            _horizontalBeams,
            _maxRange,
            numThreads
        );
    }

    LidarProcessor::~LidarProcessor()
    {
    }

    void LidarProcessor::GenerateUnitBeams()
    {
        _unitRays.clear();
        _rayChannels.clear();

        const uint32_t totalRays = _verticalRings * _horizontalBeams;
        _unitRays.reserve( totalRays );
        _rayChannels.reserve( totalRays );

        // 128 Channels with Road and Horizon Distribution (3.5m - 100m):
        // Zone 1: Near Ground & Flanks (24 rings): -45.0 deg to -14.0 deg (3.5m to 14m)
        // Zone 2: Mid-Range Road & Small Obstacles / Cones (38 rings): -14.0 deg to -5.0 deg (14m to 40m)
        // Zone 3: Far Road Surface (38 rings): -5.0 deg to -2.0 deg (40m to 100m)
        // Zone 4: Horizon Vehicles & Objects (20 rings): -2.0 deg to +3.0 deg
        // Zone 5: Overhead Signs & Bridges (8 rings): +3.0 deg to +9.0 deg
        std::vector< float > ringElevations;
        ringElevations.reserve( _verticalRings );

        constexpr uint32_t nearCount = 24;
        constexpr uint32_t midCount = 38;
        constexpr uint32_t farCount = 38;
        constexpr uint32_t horizonCount = 20;
        constexpr uint32_t upperCount = 8;

        for ( uint32_t i = 0; i < nearCount; ++i )
        {
            const float t = static_cast< float >( i ) / static_cast< float >( nearCount );
            ringElevations.push_back( -45.0f + t * ( -14.0f - ( -45.0f ) ) );
        }

        for ( uint32_t i = 0; i < midCount; ++i )
        {
            const float t = static_cast< float >( i ) / static_cast< float >( midCount );
            ringElevations.push_back( -14.0f + t * ( -5.0f - ( -14.0f ) ) );
        }

        for ( uint32_t i = 0; i < farCount; ++i )
        {
            const float t = static_cast< float >( i ) / static_cast< float >( farCount );
            ringElevations.push_back( -5.0f + t * ( -2.0f - ( -5.0f ) ) );
        }

        for ( uint32_t i = 0; i < horizonCount; ++i )
        {
            const float t = static_cast< float >( i ) / static_cast< float >( horizonCount );
            ringElevations.push_back( -2.0f + t * ( 3.0f - ( -2.0f ) ) );
        }

        for ( uint32_t i = 0; i < upperCount; ++i )
        {
            const float t = static_cast< float >( i ) / static_cast< float >( upperCount - 1 );
            ringElevations.push_back( 3.0f + t * ( 9.0f - 3.0f ) );
        }

        // 512 Horizontal Beams with Full 360 Long-Range Panoramic Coverage:
        // Sector 1: Left-Rear (-180 deg to -135 deg, 50 beams, step 0.90 deg)
        // Sector 2: Left-Side & Adjacent Lane (-135 deg to -45 deg, 130 beams, step 0.69 deg)
        // Sector 3: Forward Driving Corridor (-45 deg to +45 deg, 152 beams, step 0.59 deg)
        // Sector 4: Right-Side & Adjacent Lane (+45 deg to +135 deg, 130 beams, step 0.69 deg)
        // Sector 5: Right-Rear (+135 deg to +180 deg, 50 beams, step 0.90 deg)
        std::vector< float > beamAzimuths;
        beamAzimuths.reserve( _horizontalBeams );

        constexpr uint32_t rearCount = 30;
        constexpr uint32_t sideCount = 50;
        constexpr uint32_t fwdCount = 140;

        for ( uint32_t i = 0; i < rearCount; ++i )
        {
            const float t = static_cast< float >( i ) / static_cast< float >( rearCount );
            beamAzimuths.push_back( -180.0f + t * ( -135.0f - ( -180.0f ) ) );
        }

        for ( uint32_t i = 0; i < sideCount; ++i )
        {
            const float t = static_cast< float >( i ) / static_cast< float >( sideCount );
            beamAzimuths.push_back( -135.0f + t * ( -45.0f - ( -135.0f ) ) );
        }

        for ( uint32_t i = 0; i < fwdCount; ++i )
        {
            const float t = static_cast< float >( i ) / static_cast< float >( fwdCount );
            beamAzimuths.push_back( -45.0f + t * ( 45.0f - ( -45.0f ) ) );
        }

        for ( uint32_t i = 0; i < sideCount; ++i )
        {
            const float t = static_cast< float >( i ) / static_cast< float >( sideCount );
            beamAzimuths.push_back( 45.0f + t * ( 135.0f - 45.0f ) );
        }

        for ( uint32_t i = 0; i < rearCount; ++i )
        {
            const float t = static_cast< float >( i ) / static_cast< float >( rearCount );
            beamAzimuths.push_back( 135.0f + t * ( 180.0f - 135.0f ) );
        }

        for ( uint32_t ring = 0; ring < _verticalRings; ++ring )
        {
            const float elevationRad = ringElevations[ ring ] * DEG2RAD;
            const float cosElevation = std::cos( elevationRad );
            const float sinElevation = std::sin( elevationRad );

            for ( uint32_t beam = 0; beam < _horizontalBeams; ++beam )
            {
                const float azimuthRad = beamAzimuths[ beam ] * DEG2RAD;
                const float sinAzimuth = std::sin( azimuthRad );
                const float cosAzimuth = std::cos( azimuthRad );

                // In ETS2 truck coordinates:
                // +X: Right, -X: Left
                // +Y: Up, -Y: Down
                // -Z: Forward, +Z: Backward
                // When azimuth = 0 (forward): x = 0, y = sinElev, z = -cosElev (points forward -Z!)
                // When azimuth = 90 deg (right): x = +cosElev, y = sinElev, z = 0 (points right +X!)
                _unitRays.push_back( float3_t{
                    cosElevation * sinAzimuth,
                    sinElevation,
                    -cosElevation * cosAzimuth
                } );
                _rayChannels.push_back( ring );
            }
        }
    }

    void LidarProcessor::UpdateColliderStreaming(
        prism::base_ctrl_u* baseCtrl,
        const float3_t& globalSensorPos,
        const float3_t& globalTruckPos,
        double nowSec
    )
    {
        constexpr float kNearForwardDist  = 14.0f;
        constexpr float kMaxForwardDist   = 40.0f;
        constexpr float kBackwardDist     = 5.0f;
        constexpr float kSideDist         = 3.5f;
        constexpr float kSliceDepth       = 8.0f;
        constexpr float kSliceOverlap     = 1.0f;
        constexpr auto  kTimeBudget       = std::chrono::microseconds( 1500 );

        if ( _hasPrevTruckPos )
        {
            const double dt = nowSec - _prevColliderTime;
            if ( dt > 0.01 && dt < 2.0 )
            {
                const float dx = globalTruckPos.x - _prevTruckPos.x;
                const float dy = globalTruckPos.y - _prevTruckPos.y;
                const float dz = globalTruckPos.z - _prevTruckPos.z;
                const float dist = std::sqrt( dx * dx + dy * dy + dz * dz );
                if ( dist < 80.0f )
                {
                    const float instantSpeed = static_cast< float >( dist / dt );
                    _smoothedSpeed = _smoothedSpeed * 0.65f + instantSpeed * 0.35f;
                }
            }
        }
        _prevTruckPos     = globalTruckPos;
        _prevColliderTime = nowSec;
        _hasPrevTruckPos  = true;

        const float forwardLookahead = std::clamp( kNearForwardDist + _smoothedSpeed * 1.1f, kNearForwardDist, kMaxForwardDist );
        const auto startTime = std::chrono::steady_clock::now();

        // Always keep the immediate corridor around the truck active.
        baseCtrl->ensure_colliders_in_oriented_box(
            globalSensorPos, _smoothedRot, kNearForwardDist, kBackwardDist, kSideDist
        );

        if ( forwardLookahead <= kNearForwardDist + 0.5f )
        {
            _colliderFarOffset = kNearForwardDist;
            return;
        }

        if ( _colliderFarOffset < kNearForwardDist || _colliderFarOffset >= forwardLookahead )
        {
            _colliderFarOffset = kNearForwardDist;
        }

        // Incrementally stream the far corridor in 8m slices until the per-tick time budget is spent.
        const float startOffset = _colliderFarOffset;
        do
        {
            if ( std::chrono::steady_clock::now() - startTime >= kTimeBudget )
            {
                break;
            }

            const float sliceStart = _colliderFarOffset;
            const float sliceEnd   = ( std::min )( sliceStart + kSliceDepth, forwardLookahead );
            const float sliceBack  = -( ( std::max )( 0.0f, sliceStart - kSliceOverlap ) );

            baseCtrl->ensure_colliders_in_oriented_box(
                globalSensorPos, _smoothedRot, sliceEnd, sliceBack, kSideDist
            );

            _colliderFarOffset = ( sliceEnd >= forwardLookahead - 0.1f ) ? kNearForwardDist : sliceEnd;
        }
        while ( _colliderFarOffset != startOffset );
    }

    void LidarProcessor::Tick( const scs_value_dplacement_t& truckPos, const TrafficProcessor* trafficProcessor )
    {
        auto* const baseCtrl = prism::base_ctrl_u::get();
        if ( baseCtrl == nullptr )
        {
            return;
        }

        double cur = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (cur - lastTickTime < 1.0/5.0)
        {
            return;
        }
        lastTickTime = cur;

        auto* const scene1 = baseCtrl->get_physics_scene_1();
        auto* const scene2 = baseCtrl->get_physics_scene_2();

        prism::physx::PxScene* pxScene1 = scene1 ? scene1->get_px_scene() : nullptr;
        prism::physx::PxScene* pxScene2 = scene2 ? scene2->get_px_scene() : nullptr;

        if ( pxScene1 == nullptr && pxScene2 == nullptr )
        {
            return;
        }

        auto* const gameActor = prism::game_actor_u::get();
        if ( gameActor == nullptr || gameActor->game_physics_vehicle == nullptr )
        {
            return;
        }

        prism::placement_t truckPlacement{};
        gameActor->game_physics_vehicle->get_physics_placement( &truckPlacement );

        if ( !_hasSmoothedRot )
        {
            _smoothedRot = truckPlacement.rot;
            _hasSmoothedRot = true;
        }
        else
        {
            _smoothedRot = NLerp( _smoothedRot, truckPlacement.rot, 0.15f );
        }

        const float3_t rotatedOffset = _mountOffset.rotate( _smoothedRot );
        prism::placement_t sensorPlacement = truckPlacement;
        sensorPlacement.pos = sensorPlacement.pos + rotatedOffset;

        const prism::physx::PxVec3 sensorPxPos1 = scene1 ? scene1->to_physx_pos( sensorPlacement ) : prism::physx::PxVec3( 0, 0, 0 );
        const prism::physx::PxVec3 sensorPxPos2 = scene2 ? scene2->to_physx_pos( sensorPlacement ) : prism::physx::PxVec3( 0, 0, 0 );
        const float3_t globalTruckPos = truckPlacement.to_global_position();
        const float3_t globalSensorPos = sensorPlacement.to_global_position();

        // ETS2 lazy-loads static colliders only within ~0.5x of the vehicle's AABB dimensions.
        // Stream colliders in a speed-scaled, narrower corridor with a per-tick time budget.
        UpdateColliderStreaming( baseCtrl, globalSensorPos, globalTruckPos, cur );

        _activeDynamicBoxes.clear();
        if ( trafficProcessor != nullptr )
        {
            const auto& rawBoxes = trafficProcessor->GetActiveDynamicBoxes();
            _activeDynamicBoxes.reserve( rawBoxes.size() );

            constexpr float margin = 0.25f;
            const float maxDistSq = ( _maxRange + 15.0f ) * ( _maxRange + 15.0f );

            for ( const auto& rawBox : rawBoxes )
            {
                const float dx = rawBox.center.x - globalTruckPos.x;
                const float dy = rawBox.center.y - globalTruckPos.y;
                const float dz = rawBox.center.z - globalTruckPos.z;
                const float distToTruckSq = dx * dx + dy * dy + dz * dz;

                if ( distToTruckSq > maxDistSq )
                {
                    continue;
                }

                const float hx = rawBox.halfExtents.x + margin;
                const float hy = rawBox.halfExtents.y + margin;
                const float hz = rawBox.halfExtents.z + margin;
                const float radSq = hx * hx + hy * hy + hz * hz;

                _activeDynamicBoxes.push_back( PrecomputedDynamicBox{
                    rawBox.center,
                    float3_t{ hx, hy, hz },
                    quat_t{ rawBox.rot.w, -rawBox.rot.x, -rawBox.rot.y, -rawBox.rot.z },
                    radSq
                } );
            }
        }

        const auto now = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        _lidarBuffer->max_points = static_cast< uint32_t >( MAX_LIDAR_POINTS );
        _lidarBuffer->timestamp = static_cast< uint64_t >( now );
        _lidarBuffer->sensor_pos_x = globalTruckPos.x;
        _lidarBuffer->sensor_pos_y = globalTruckPos.y;
        _lidarBuffer->sensor_pos_z = globalTruckPos.z;
        _lidarBuffer->sensor_rot_w = _smoothedRot.w;
        _lidarBuffer->sensor_rot_x = _smoothedRot.x;
        _lidarBuffer->sensor_rot_y = _smoothedRot.y;
        _lidarBuffer->sensor_rot_z = _smoothedRot.z;

        const prism::physx::PxQueryFilterData filterData(
            prism::physx::PxQueryFlag::eSTATIC | prism::physx::PxQueryFlag::eDYNAMIC
        );
        // PhysX Scene Query Hit Flags:
        // PxHitFlag::eDEFAULT = ePOSITION (0x1) | eNORMAL (0x2) | eDISTANCE (0x4) = 0x0007.
        // PxHitFlag::eMESH_BOTH_SIDES = 0x0800 (ensures both front and back-facing mesh triangles are hit).
        // eMESH_ANY (0x0400) is intentionally omitted so PhysX always returns the true closest intersection.
        constexpr prism::physx::PxHitFlags hitFlags = 0x0807;

        const bool hasTrailer = ( gameActor->game_physics_vehicle->get_trailer() != nullptr );
        const float rearClearDist = hasTrailer ? 15.5f : 4.8f;

        const size_t rayCount = _unitRays.size();
        const size_t numWorkers = _threadPool->ThreadCount();
        const size_t chunkSize = ( rayCount + numWorkers - 1 ) / numWorkers;

        const quat_t rayRot = _smoothedRot;
        for ( size_t w = 0; w < numWorkers; ++w )
        {
            const size_t startIdx = w * chunkSize;
            const size_t endIdx = std::min( rayCount, startIdx + chunkSize );
            if ( startIdx >= endIdx )
            {
                continue;
            }

            _threadPool->Enqueue( [ this, w, startIdx, endIdx, sensorPxPos1, sensorPxPos2, rayRot, pxScene1, pxScene2, filterData, hitFlags, rearClearDist, globalSensorPos ]()
            {
                auto& hits = _threadLocalHits[ w ];
                hits.clear();

                for ( size_t i = startIdx; i < endIdx; ++i )
                {
                    const float3_t& localDir = _unitRays[ i ];
                    const float rayMinRange = ComputeClearLaunchDistance( localDir, rearClearDist );
                    const float rayLength = _maxRange - rayMinRange;

                    const float3_t worldDir = localDir.rotate( rayRot );
                    const prism::physx::PxVec3 pxDir( worldDir.x, worldDir.y, worldDir.z );

                    bool hasHit = false;
                    bool hitFromDynamicScene = false;
                    float closestDist = rayLength;

                    if ( pxScene1 != nullptr )
                    {
                        prism::physx::PxRaycastBuffer hit1;
                        const prism::physx::PxVec3 pxOrigin1 = sensorPxPos1 + pxDir * rayMinRange;
                        if ( SafeRaycast( pxScene1, pxOrigin1, pxDir, rayLength, hit1, hitFlags, filterData, prism::physics_scene_physx_t::raycast_vtable_offset ) && hit1.hasBlock )
                        {
                            if ( hit1.block.distance > 0.0f && hit1.block.distance < closestDist )
                            {
                                hasHit = true;
                                closestDist = hit1.block.distance;
                                hitFromDynamicScene = false;
                            }
                        }
                    }

                    if ( pxScene2 != nullptr )
                    {
                        prism::physx::PxRaycastBuffer hit2;
                        const prism::physx::PxVec3 pxOrigin2 = sensorPxPos2 + pxDir * rayMinRange;
                        if ( SafeRaycast( pxScene2, pxOrigin2, pxDir, closestDist, hit2, hitFlags, filterData, prism::physics_scene_physx_t::raycast_vtable_offset ) && hit2.hasBlock )
                        {
                            if ( hit2.block.distance > 0.0f && hit2.block.distance < closestDist )
                            {
                                const float candDist = hit2.block.distance + rayMinRange;
                                const float3_t candLocal = float3_t{
                                    localDir.x * candDist + _mountOffset.x,
                                    localDir.y * candDist + _mountOffset.y,
                                    localDir.z * candDist + _mountOffset.z
                                };

                                const bool isEgo = std::abs( candLocal.x ) < 1.45f &&
                                                   candLocal.y >= -0.6f && candLocal.y <= 4.3f &&
                                                   candLocal.z >= -3.5f && candLocal.z <= rearClearDist;

                                if ( !isEgo )
                                {
                                    hasHit = true;
                                    closestDist = hit2.block.distance;
                                    hitFromDynamicScene = true;
                                }
                            }
                        }
                    }

                    if ( hasHit )
                    {
                        const float hitDistance = closestDist + rayMinRange;

                        const float3_t localHit = float3_t{
                            localDir.x * hitDistance + _mountOffset.x,
                            localDir.y * hitDistance + _mountOffset.y,
                            localDir.z * hitDistance + _mountOffset.z
                        };

                        const float distFalloff = ( hitDistance < _maxRange )
                            ? ( 1.0f - ( hitDistance / _maxRange ) )
                            : 0.0f;

                        bool isDynamicHit = hitFromDynamicScene;
                        if ( !isDynamicHit && !_activeDynamicBoxes.empty() )
                        {
                            const float3_t worldHit = float3_t{
                                globalSensorPos.x + worldDir.x * hitDistance,
                                globalSensorPos.y + worldDir.y * hitDistance,
                                globalSensorPos.z + worldDir.z * hitDistance
                            };

                            for ( const auto& box : _activeDynamicBoxes )
                            {
                                const float dx = worldHit.x - box.center.x;
                                const float dy = worldHit.y - box.center.y;
                                const float dz = worldHit.z - box.center.z;
                                const float distSq = dx * dx + dy * dy + dz * dz;

                                if ( distSq > box.radiusSq )
                                {
                                    continue;
                                }

                                const float3_t boxLocal = float3_t{ dx, dy, dz }.rotate( box.invRot );
                                if ( std::abs( boxLocal.x ) <= box.halfExtents.x &&
                                     std::abs( boxLocal.y ) <= box.halfExtents.y &&
                                     std::abs( boxLocal.z ) <= box.halfExtents.z )
                                {
                                    isDynamicHit = true;
                                    break;
                                }
                            }
                        }

                        const uint8_t hitType = static_cast< uint8_t >(
                            isDynamicHit ? LidarHitType::Dynamic : LidarHitType::Static
                        );
                        const uint8_t isDynamic = isDynamicHit ? 1 : 0;

                        hits.push_back( LidarPoint{
                            localHit.x,
                            localHit.y,
                            localHit.z,
                            hitDistance,
                            distFalloff,
                            static_cast< uint16_t >( _rayChannels[ i ] ),
                            hitType,
                            isDynamic
                        } );
                    }
                }
            } );
        }

        _threadPool->WaitForAll();

        uint32_t totalPointCount = 0;
        for ( size_t w = 0; w < numWorkers && totalPointCount < MAX_LIDAR_POINTS; ++w )
        {
            const auto& hits = _threadLocalHits[ w ];
            const size_t toCopy = std::min( hits.size(), MAX_LIDAR_POINTS - totalPointCount );
            if ( toCopy > 0 )
            {
                std::memcpy( &_lidarBuffer->points[ totalPointCount ], hits.data(), toCopy * sizeof( LidarPoint ) );
                totalPointCount += static_cast< uint32_t >( toCopy );
            }
        }

        _lidarBuffer->point_count = totalPointCount;
        _memoryHandler->write_lidar_mem( *_lidarBuffer );
    }
}
