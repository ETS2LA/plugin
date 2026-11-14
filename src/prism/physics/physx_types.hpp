#pragma once

#include <cstdint>

namespace ets2la_plugin::prism::physx
{
#pragma pack( push, 16 )

    struct PxVec3
    {
        float x;
        float y;
        float z;

        PxVec3()
            : x( 0.0f ), y( 0.0f ), z( 0.0f )
        {
        }

        PxVec3( float xVal, float yVal, float zVal )
            : x( xVal ), y( yVal ), z( zVal )
        {
        }

        PxVec3 operator+( const PxVec3& other ) const
        {
            return PxVec3( x + other.x, y + other.y, z + other.z );
        }

        PxVec3 operator-( const PxVec3& other ) const
        {
            return PxVec3( x - other.x, y - other.y, z - other.z );
        }

        PxVec3 operator*( float scalar ) const
        {
            return PxVec3( x * scalar, y * scalar, z * scalar );
        }
    };

    struct PxFilterData
    {
        uint32_t word0;
        uint32_t word1;
        uint32_t word2;
        uint32_t word3;

        PxFilterData()
            : word0( 0 ), word1( 0 ), word2( 0 ), word3( 0 )
        {
        }

        PxFilterData( uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3 )
            : word0( w0 ), word1( w1 ), word2( w2 ), word3( w3 )
        {
        }
    };

    namespace PxQueryFlag
    {
        enum Enum : uint16_t
        {
            eSTATIC     = ( 1 << 0 ),
            eDYNAMIC    = ( 1 << 1 ),
            ePREFILTER  = ( 1 << 2 ),
            ePOSTFILTER = ( 1 << 3 ),
            eANY_HIT    = ( 1 << 4 ),
            eNO_BLOCK   = ( 1 << 5 )
        };
    }
    using PxQueryFlags = uint16_t;

    struct PxQueryFilterData
    {
        PxFilterData data;
        PxQueryFlags flags;
        uint16_t pad;

        PxQueryFilterData()
            : flags( PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC ), pad( 0 )
        {
        }

        explicit PxQueryFilterData( PxQueryFlags queryFlags )
            : flags( queryFlags ), pad( 0 )
        {
        }

        PxQueryFilterData( const PxFilterData& filterData, PxQueryFlags queryFlags )
            : data( filterData ), flags( queryFlags ), pad( 0 )
        {
        }
    };

    namespace PxHitFlag
    {
        enum Enum : uint16_t
        {
            ePOSITION        = ( 1 << 0 ),
            eNORMAL          = ( 1 << 1 ),
            eDISTANCE        = ( 1 << 2 ),
            eUV              = ( 1 << 3 ),
            eMESH_MULTIPLE   = ( 1 << 9 ),
            eMESH_ANY        = ( 1 << 10 ),
            eMESH_BOTH_SIDES = ( 1 << 11 ),
            eDEFAULT         = ePOSITION | eNORMAL | eDISTANCE
        };
    }
    using PxHitFlags = uint16_t;

    namespace PxQueryHitType
    {
        enum Enum
        {
            eNONE  = 0,
            eTOUCH = 1,
            eBLOCK = 2
        };
    }

    class PxRigidActor;
    class PxShape;

    struct PxActorShape
    {
        PxRigidActor* actor;
        PxShape* shape;

        PxActorShape()
            : actor( nullptr ), shape( nullptr )
        {
        }

        PxActorShape( PxRigidActor* a, PxShape* s )
            : actor( a ), shape( s )
        {
        }
    };
    static_assert( sizeof( PxActorShape ) == 0x10, "PxActorShape size must be 0x10" );

    struct PxQueryHit : public PxActorShape
    {
        uint32_t faceIndex;
        uint32_t pad0;

        PxQueryHit()
            : PxActorShape(), faceIndex( 0xFFFFFFFF ), pad0( 0 )
        {
        }
    };
    static_assert( sizeof( PxQueryHit ) == 0x18, "PxQueryHit size must be 0x18" );

    struct PxLocationHit : public PxQueryHit
    {
        PxHitFlags flags;
        uint16_t pad1;
        PxVec3 position;
        PxVec3 normal;
        float distance;

        PxLocationHit()
            : PxQueryHit(), flags( 0 ), pad1( 0 ), position(), normal(), distance( 3.402823466e+38F )
        {
        }
    };
    static_assert( sizeof( PxLocationHit ) == 0x38, "PxLocationHit size must be 0x38" );

    struct PxRaycastHit : public PxLocationHit
    {
        float u;
        float v;

        PxRaycastHit()
            : PxLocationHit(), u( 0.0f ), v( 0.0f )
        {
        }
    };
    static_assert( sizeof( PxRaycastHit ) == 0x40, "PxRaycastHit size must be 0x40" );

    template< typename HitType >
    struct PxHitCallback
    {
        HitType block;
        bool hasBlock;
        uint8_t pad[ 7 ];
        HitType* touches;
        uint32_t maxNbTouches;
        uint32_t nbTouches;

        PxHitCallback( HitType* aTouches = nullptr, uint32_t aMaxNbTouches = 0 )
            : block(),
              hasBlock( false ),
              pad{ 0 },
              touches( aTouches ),
              maxNbTouches( aMaxNbTouches ),
              nbTouches( 0 )
        {
        }

        virtual bool processTouches( const HitType* buffer, uint32_t nbHits )
        {
            return true;
        }

        virtual void finalizeQuery()
        {
        }

        virtual ~PxHitCallback()
        {
        }

        bool hasAnyHits() const
        {
            return hasBlock || ( nbTouches > 0 );
        }
    };

    template< typename HitType >
    struct PxHitBuffer : public PxHitCallback< HitType >
    {
        PxHitBuffer( HitType* aTouches = nullptr, uint32_t aMaxNbTouches = 0 )
            : PxHitCallback< HitType >( aTouches, aMaxNbTouches )
        {
        }

        virtual ~PxHitBuffer() override
        {
        }

        virtual bool processTouches( const HitType* buffer, uint32_t nbHits ) override
        {
            return true;
        }

        virtual void finalizeQuery() override
        {
        }

        const HitType& getBlock() const
        {
            return this->block;
        }

        bool hasAnyHits() const
        {
            return this->hasBlock || ( this->nbTouches > 0 );
        }
    };

    using PxRaycastCallback = PxHitCallback< PxRaycastHit >;
    using PxRaycastBuffer   = PxHitBuffer< PxRaycastHit >;

    static_assert( sizeof( PxRaycastCallback ) == 0x60, "PxRaycastCallback size must be 0x60" );
    static_assert( sizeof( PxRaycastBuffer ) == 0x60, "PxRaycastBuffer size must be 0x60" );

    class PxQueryFilterCallback
    {
    public:
        virtual PxQueryHitType::Enum preFilter(
            const PxFilterData& filterData,
            const PxShape* shape,
            const PxRigidActor* actor,
            PxHitFlags& queryFlags
        ) = 0;

        virtual PxQueryHitType::Enum postFilter(
            const PxFilterData& filterData,
            const PxLocationHit& hit
        ) = 0;

        virtual ~PxQueryFilterCallback()
        {
        }
    };

    class PxScene
    {
    public:
        bool raycast(
            const PxVec3& origin,
            const PxVec3& unitDir,
            float distance,
            PxRaycastCallback& hitCall,
            const PxHitFlags& hitFlags,
            const PxQueryFilterData& filterData,
            PxQueryFilterCallback* filterCall,
            const void* cache,
            uint32_t vtableOffset
        ) const
        {
            using RaycastFn = bool( __fastcall* )(
                const PxScene*,
                const PxVec3&,
                const PxVec3&,
                float,
                PxRaycastCallback&,
                const PxHitFlags&,
                const PxQueryFilterData&,
                PxQueryFilterCallback*,
                const void*
            );

            auto* const vtable = *reinterpret_cast< const uintptr_t* const* >( this );
            if ( vtable == nullptr )
            {
                return false;
            }

            auto* const fn = reinterpret_cast< RaycastFn >( *reinterpret_cast< const uintptr_t* >(
                reinterpret_cast< const char* >( vtable ) + vtableOffset
            ) );

            if ( fn == nullptr )
            {
                return false;
            }

            return fn( this, origin, unitDir, distance, hitCall, hitFlags, filterData, filterCall, cache );
        }

        bool lockRead( const char* file, uint32_t line, uint32_t vtableOffset )
        {
            using LockReadFn = bool( __fastcall* )( PxScene*, const char*, uint32_t );

            auto* const vtable = *reinterpret_cast< const uintptr_t* const* >( this );
            if ( vtable == nullptr )
            {
                return false;
            }

            auto* const fn = reinterpret_cast< LockReadFn >( *reinterpret_cast< const uintptr_t* >(
                reinterpret_cast< const char* >( vtable ) + vtableOffset
            ) );

            if ( fn == nullptr )
            {
                return false;
            }

            return fn( this, file, line );
        }

        void unlockRead( uint32_t vtableOffset )
        {
            using UnlockReadFn = void( __fastcall* )( PxScene* );

            auto* const vtable = *reinterpret_cast< const uintptr_t* const* >( this );
            if ( vtable == nullptr )
            {
                return;
            }

            auto* const fn = reinterpret_cast< UnlockReadFn >( *reinterpret_cast< const uintptr_t* >(
                reinterpret_cast< const char* >( vtable ) + vtableOffset
            ) );

            if ( fn != nullptr )
            {
                fn( this );
            }
        }
    };

#pragma pack( pop )
}
