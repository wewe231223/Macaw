/**
 * @file FBVH8TrianglePackets.h
 * @brief 여덟 삼각형의 성분별 저장 형식과 SIMD 광선 교차 방문자를 정의한다.
 */
/**
 * @var float BVH8::FTrianglePacket::Edge2[3][8]
 * @brief 각 삼각형의 V2 - V0 변 벡터를 축별 여덟 레인으로 저장합니다.
 */
/**
 * @var float BVH8::FTrianglePacket::V0[3][8]
 * @brief 각 삼각형의 기준 정점을 축별 여덟 레인으로 저장합니다.
 */
/**
 * @var BVH8::TTrianglePacketVisitor::V BVH8::TTrianglePacketVisitor::Direction[3]
 * @brief 광선 방향의 X, Y, Z 성분을 각각 SIMD 레인에 복제한 벡터입니다.
 */
/**
 * @var BVH8::TTrianglePacketVisitor::V BVH8::TTrianglePacketVisitor::Sign
 * @brief 와인딩을 뒤집을 때 부호 반전에 사용하는 SIMD 부호 비트 마스크입니다.
 */

#pragma once
#include "Core/Spatial/FBVH8.h"

namespace BVH8 {
    struct alignas(32) FTrianglePacket {
        /**
         * @brief 축별 삼각형 데이터. V0는 첫 정점이고 Edge1은 V1 - V0, Edge2는 V2 - V0이며 두 번째 첨자는 삼각형 레인이다.
         */
        float Edge1[3][8]{}, Edge2[3][8]{}, V0[3][8]{};
        Uint32 mFaceIndices[8]{};
    };

    static_assert(sizeof(FTrianglePacket) == 320);

    /**
     * @brief 삼각형 패킷의 앞면 교차를 SIMD로 검사하고 최근접 상한을 갱신하는 리프 방문자.
     * @tparam S V, Width 및 패킷 검사에 필요한 SIMD 연산을 제공할 구현. FSIMD4 또는 FSIMD8을 사용할 수 있다.
     */
    template <class S>
    struct TTrianglePacketVisitor {
    public:
        using V = typename S::V;

    public:
        TTrianglePacketVisitor(const FTrianglePacket* InPackets, const FRay& Ray, bool ReverseWinding, FTriangleHit* Hit);

    public:
        bool operator()(Uint32 Index, Uint32 Count, float& Limit) const;

    private:
        static V Dot(V AX, V AY, V AZ, V BX, V BY, V BZ);

    private:
        const FTrianglePacket* mPackets{};
        FTriangleHit* mHit{};
        V mOrigin[3]{}, mDirection[3]{}, mSign{};
    };

    template <class S>
    TTrianglePacketVisitor<S>::TTrianglePacketVisitor(const FTrianglePacket* InPackets, const FRay& Ray, bool ReverseWinding, FTriangleHit* Hit)
	: mPackets{InPackets},
	  mHit{Hit},
	  mSign{S::Set(ReverseWinding ? -0.0f : 0.0f)} {
        const float O[]{Ray.position.x, Ray.position.y, Ray.position.z}, D[]{Ray.direction.x, Ray.direction.y, Ray.direction.z};

        for (Uint32 Axis{}; Axis < 3; ++Axis) {
            mOrigin[Axis] = S::Set(O[Axis]);
            mDirection[Axis] = S::Set(D[Axis]);
        }
    }

    template <class S>
    __forceinline typename TTrianglePacketVisitor<S>::V TTrianglePacketVisitor<S>::Dot(V AX, V AY, V AZ, V BX, V BY, V BZ) {
        return S::Add(S::Add(S::Mul(AX, BX), S::Mul(AY, BY)), S::Mul(AZ, BZ));
    }

    template <class S>
    __forceinline bool TTrianglePacketVisitor<S>::operator()(Uint32 Index, Uint32 Count, float& Limit) const {
        const auto& Packet{mPackets[Index]};
        bool Hit{false};

        for (Uint32 Base{0}; Base < Count; Base += S::Width) {
            const Uint32 Active{(((1u << Count) - 1) >> Base) & ((1u << S::Width) - 1)};
            const V E1X{S::Load(Packet.Edge1[0] + Base)}, E1Y{S::Load(Packet.Edge1[1] + Base)}, E1Z{S::Load(Packet.Edge1[2] + Base)};
            const V E2X{S::Load(Packet.Edge2[0] + Base)}, E2Y{S::Load(Packet.Edge2[1] + Base)}, E2Z{S::Load(Packet.Edge2[2] + Base)};
            const V PX{S::Sub(S::Mul(mDirection[1], E2Z), S::Mul(mDirection[2], E2Y))};
            const V PY{S::Sub(S::Mul(mDirection[2], E2X), S::Mul(mDirection[0], E2Z))};
            const V PZ{S::Sub(S::Mul(mDirection[0], E2Y), S::Mul(mDirection[1], E2X))};
            const V Det{S::Xor(Dot(E1X, E1Y, E1Z, PX, PY, PZ), mSign)};
            V Valid{S::GE(Det, S::Set(DirectX::XMVectorGetX(DirectX::g_RayEpsilon)))};

            if ((S::Bits(Valid) & Active) == 0)
                continue;

            const V SX{S::Sub(mOrigin[0], S::Load(Packet.V0[0] + Base))}, SY{S::Sub(mOrigin[1], S::Load(Packet.V0[1] + Base))}, SZ{S::Sub(mOrigin[2], S::Load(Packet.V0[2] + Base))};
            const V QX{S::Sub(S::Mul(SY, E1Z), S::Mul(SZ, E1Y))};
            const V QY{S::Sub(S::Mul(SZ, E1X), S::Mul(SX, E1Z))};
            const V QZ{S::Sub(S::Mul(SX, E1Y), S::Mul(SY, E1X))};
            const V U{S::Xor(Dot(SX, SY, SZ, PX, PY, PZ), mSign)}, B{S::Xor(Dot(mDirection[0], mDirection[1], mDirection[2], QX, QY, QZ), mSign)};
            const V T{S::Xor(Dot(E2X, E2Y, E2Z, QX, QY, QZ), mSign)}, Zero{S::Set(0.0f)};

            Valid = S::And(Valid, S::And(S::GE(U, Zero), S::GE(B, Zero)));
            Valid = S::And(Valid, S::And(S::LE(S::Add(U, B), Det), S::GE(T, Zero)));

            if ((S::Bits(Valid) & Active) == 0)
                continue;

            const V Distance{S::Div(T, S::Select(Valid, Det, S::Set(1.0f)))};

            Valid = S::And(Valid, S::LE(Distance, S::Set(Limit)));

            const Uint32 Hits{S::Bits(Valid) & Active};

            if (Hits == 0)
                continue;

            Limit = S::MinLane(S::Select(S::Mask(Hits), Distance, S::Set(Limit)));

            if (mHit != nullptr) {
                mHit->mPacketIndex = Index;
                mHit->mLane = Base + std::countr_zero(S::Bits(S::LE(Distance, S::Set(Limit))) & Hits);
            }

            Hit = true;
        }

        return Hit;
    }
}
