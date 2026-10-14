/**
 * @file FBVH8TrianglePackets.h
 * @brief 여덟 삼각형의 성분별 저장 형식과 SIMD 광선 교차 방문자를 정의한다.
 */
#pragma once
#include "Core/Spatial/FBVH8.h"

namespace BVH8 {
    /**
     * @brief 삼각형 최대 여덟 개의 기준 정점과 두 변 벡터를 축별로 저장하는 32바이트 정렬 패킷. 크기는 288바이트이다.
     */
    struct alignas(32) FTrianglePacket {
        /**
         * @brief 축별 삼각형 데이터. V0는 첫 정점이고 Edge1은 V1 - V0, Edge2는 V2 - V0이며 두 번째 첨자는 삼각형 레인이다.
         */
        float Edge1[3][8]{}, Edge2[3][8]{}, V0[3][8]{};
    };

    static_assert(sizeof(FTrianglePacket) == 288);

    /**
     * @brief 삼각형 패킷의 앞면 교차를 SIMD로 검사하고 최근접 상한을 갱신하는 리프 방문자.
     * @tparam S V, Width 및 패킷 검사에 필요한 SIMD 연산을 제공할 구현. FSIMD4 또는 FSIMD8을 사용할 수 있다.
     */
    template <class S>
    struct TTrianglePacketVisitor {
        /**
         * @brief 선택한 SIMD 구현의 벡터 및 비교 마스크 타입.
         */
        using V = typename S::V;
        /**
         * @brief 리프 인덱스로 접근할 외부 삼각형 패킷 배열. 방문자를 사용하는 동안 유효해야 한다.
         */
        const FTrianglePacket* Packets;
        /**
         * @brief 광선 원점과 방향의 축별 브로드캐스트 벡터 및 와인딩 반전에 사용할 부호 비트 벡터.
         */
        V Origin[3], Direction[3], Sign;

        /**
         * @brief 광선과 면 방향을 준비한 삼각형 패킷 방문자를 생성한다.
         * @param InPackets 검사할 패킷 배열의 시작 주소.
         * @param Ray 검사할 광선. 원점과 방향을 SIMD 벡터에 복사한다.
         * @param ReverseWinding true이면 행렬식과 교차 분자의 부호를 반전하여 면 판정을 뒤집는다.
         */
        TTrianglePacketVisitor(const FTrianglePacket* InPackets, const FRay& Ray, bool ReverseWinding)
            : Packets(InPackets),
              Sign(S::Set(ReverseWinding ? -0.0f : 0.0f)) {
            const float O[]{Ray.position.x, Ray.position.y, Ray.position.z}, D[]{Ray.direction.x, Ray.direction.y, Ray.direction.z};

            for (Uint32 Axis = 0; Axis < 3; ++Axis) {
                Origin[Axis] = S::Set(O[Axis]);
                Direction[Axis] = S::Set(D[Axis]);
            }
        }

        /**
         * @brief 레인별로 두 3차원 벡터의 내적을 계산한다.
         * @param AX 첫 벡터의 X 성분들.
         * @param AY 첫 벡터의 Y 성분들.
         * @param AZ 첫 벡터의 Z 성분들.
         * @param BX 두 번째 벡터의 X 성분들.
         * @param BY 두 번째 벡터의 Y 성분들.
         * @param BZ 두 번째 벡터의 Z 성분들.
         * @return 레인별 AX * BX + AY * BY + AZ * BZ.
         */
        static __forceinline V Dot(V AX, V AY, V AZ, V BX, V BY, V BZ) {
            return S::Add(S::Add(S::Mul(AX, BX), S::Mul(AY, BY)), S::Mul(AZ, BZ));
        }

        /**
         * @brief 패킷의 유효한 삼각형들과 광선의 최근접 앞면 교차를 검사한다.
         * @param Index 검사할 패킷 인덱스.
         * @param Count 유효한 삼각형 레인 수. 1부터 8까지 허용한다.
         * @param[in,out] Limit 광선 매개변수 상한. 교차한 레인의 최솟값으로 갱신한다.
         * @return 상한 이하이고 광선 앞쪽에 있는 유효한 교차가 있으면 true.
         * @details 와인딩 방향, 행렬식 오차 한계 및 무게중심 좌표를 검사하고 남은 레인에서 최근접 값을 선택한다.
         * @pre Index에 대응하는 패킷이 존재해야 한다.
         */
        __forceinline bool operator()(Uint32 Index, Uint32 Count, float& Limit) const {
            const auto& Packet = Packets[Index];
            bool Hit = false;

            for (Uint32 Base = 0; Base < Count; Base += S::Width) {
                const Uint32 Active = (((1u << Count) - 1) >> Base) & ((1u << S::Width) - 1);
                const V E1X = S::Load(Packet.Edge1[0] + Base), E1Y = S::Load(Packet.Edge1[1] + Base), E1Z = S::Load(Packet.Edge1[2] + Base);
                const V E2X = S::Load(Packet.Edge2[0] + Base), E2Y = S::Load(Packet.Edge2[1] + Base), E2Z = S::Load(Packet.Edge2[2] + Base);
                const V PX = S::Sub(S::Mul(Direction[1], E2Z), S::Mul(Direction[2], E2Y));
                const V PY = S::Sub(S::Mul(Direction[2], E2X), S::Mul(Direction[0], E2Z));
                const V PZ = S::Sub(S::Mul(Direction[0], E2Y), S::Mul(Direction[1], E2X));
                const V Det = S::Xor(Dot(E1X, E1Y, E1Z, PX, PY, PZ), Sign);
                V Valid = S::GE(Det, S::Set(DirectX::XMVectorGetX(DirectX::g_RayEpsilon)));

                if ((S::Bits(Valid) & Active) == 0)
                    continue;

                const V SX = S::Sub(Origin[0], S::Load(Packet.V0[0] + Base)), SY = S::Sub(Origin[1], S::Load(Packet.V0[1] + Base)), SZ = S::Sub(Origin[2], S::Load(Packet.V0[2] + Base));
                const V QX = S::Sub(S::Mul(SY, E1Z), S::Mul(SZ, E1Y));
                const V QY = S::Sub(S::Mul(SZ, E1X), S::Mul(SX, E1Z));
                const V QZ = S::Sub(S::Mul(SX, E1Y), S::Mul(SY, E1X));
                const V U = S::Xor(Dot(SX, SY, SZ, PX, PY, PZ), Sign), B = S::Xor(Dot(Direction[0], Direction[1], Direction[2], QX, QY, QZ), Sign);
                const V T = S::Xor(Dot(E2X, E2Y, E2Z, QX, QY, QZ), Sign), Zero = S::Set(0.0f);

                Valid = S::And(Valid, S::And(S::GE(U, Zero), S::GE(B, Zero)));
                Valid = S::And(Valid, S::And(S::LE(S::Add(U, B), Det), S::GE(T, Zero)));

                if ((S::Bits(Valid) & Active) == 0)
                    continue;

                const V Distance = S::Div(T, S::Select(Valid, Det, S::Set(1.0f)));

                Valid = S::And(Valid, S::LE(Distance, S::Set(Limit)));

                const Uint32 Hits = S::Bits(Valid) & Active;

                if (Hits == 0)
                    continue;

                Limit = S::MinLane(S::Select(S::Mask(Hits), Distance, S::Set(Limit)));
                Hit = true;
            }

            return Hit;
        }
    };
}
