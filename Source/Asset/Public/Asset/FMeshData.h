/**
 * @file FMeshData.h
 * @brief CPU 메시의 위치·법선 정점, 인덱스와 로컬 경계 상자입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FBox.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector4.h"
#include "Asset/FSubMesh.h"
#include "Asset/FMeshMaterialSlot.h"

namespace Asset {

    /**
     * @brief CPU 메시의 위치·법선 정점, 인덱스와 로컬 경계 상자입니다.
     * @ingroup Asset
     */
    struct FMeshData {
    public:
        /**
         * @brief 정점별 위치 벡터 배열의 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        Core::TArray<Core::FVector>& GetPositions();

        /**
         * @brief 정점별 위치 벡터 배열의 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::TArray<Core::FVector>& GetPositions() const;

        /**
         * @brief 정점별 법선 벡터 배열의 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        Core::TArray<Core::FVector>& GetNormals();

        /**
         * @brief 정점별 법선 벡터 배열의 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::TArray<Core::FVector>& GetNormals() const;

        /**
         * @brief 정점별 탄젠트와 방향 부호 배열의 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        Core::TArray<Core::FVector4>& GetTangents();

        /**
         * @brief 정점별 탄젠트와 방향 부호 배열의 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::TArray<Core::FVector4>& GetTangents() const;

        /**
         * @brief 채널별 정점 UV 좌표 배열의 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        Core::TArray<Core::TArray<Core::FVector2>>& GetUVChannels();

        /**
         * @brief 채널별 정점 UV 좌표 배열의 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::TArray<Core::TArray<Core::FVector2>>& GetUVChannels() const;

        /**
         * @brief 정점별 색상 배열의 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        Core::TArray<Core::FVector4>& GetColors();

        /**
         * @brief 정점별 색상 배열의 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::TArray<Core::FVector4>& GetColors() const;

        /**
         * @brief 삼각형을 구성하는 정점 인덱스 배열의 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        Core::TArray<Core::Uint32>& GetIndices();

        /**
         * @brief 삼각형을 구성하는 정점 인덱스 배열의 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::TArray<Core::Uint32>& GetIndices() const;

        /**
         * @brief 메시의 로컬 공간 경계 상자를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::FBox& GetBounds() const;

        /**
         * @brief 재질별 인덱스 구간을 반환합니다.
         * @return 수정 가능한 SubMesh 배열입니다.
         */
        Core::TArray<Asset::FSubMesh>& GetSubMeshes();

        /**
         * @brief 재질별 인덱스 구간을 반환합니다.
         * @return 읽기 전용 SubMesh 배열입니다.
         */
        const Core::TArray<Asset::FSubMesh>& GetSubMeshes() const;

        /**
         * @brief 재질 슬롯을 반환합니다.
         * @return 수정 가능한 재질 슬롯 배열입니다.
         */
        Core::TArray<Asset::FMeshMaterialSlot>& GetMaterialSlots();

        /**
         * @brief 재질 슬롯을 반환합니다.
         * @return 읽기 전용 재질 슬롯 배열입니다.
         */
        const Core::TArray<Asset::FMeshMaterialSlot>& GetMaterialSlots() const;

        /**
         * @brief 메시의 로컬 공간 경계 상자를 교체합니다.
         * @param[in] Bounds 메시에 설정할 로컬 공간 경계 상자입니다.
         */
        void SetBounds(const Core::FBox& Bounds);

        /**
         * @brief 현재 정점 위치를 모두 감싸는 로컬 공간 경계 상자를 계산해 설정합니다.
         */
        void ComputeBounds();

        /**
         * @brief 정점 속성과 인덱스 배열에 할당된 CPU 메모리 바이트 수를 반환합니다.
         * @return 정점 속성과 인덱스 배열에 할당된 CPU 메모리 바이트 수를 나타내는 정수 값입니다.
         */
        std::size_t GetAllocatedBytes() const;

    private:
        /**
         * @brief CPU 위치·법선 정점 목록입니다.
         */
        Core::TArray<Core::FVector> mPositions{};

        /**
         * @brief 정점별 법선 벡터 배열입니다.
         */
        Core::TArray<Core::FVector> mNormals{};

        /**
         * @brief 정점별 탄젠트 벡터와 방향 부호 배열입니다.
         */
        Core::TArray<Core::FVector4> mTangents{};

        /**
         * @brief 채널별로 보관하는 정점 UV 좌표 배열입니다.
         */
        Core::TArray<Core::TArray<Core::FVector2>> mUVChannels{};

        /**
         * @brief 정점별 RGBA 색상 배열입니다.
         */
        Core::TArray<Core::FVector4> mColors{};

        /**
         * @brief 삼각형을 구성하는 정점 인덱스 목록입니다.
         */
        Core::TArray<Core::Uint32> mIndices{};

        /**
         * @brief 축 정렬 경계 상자입니다.
         */
        Core::FBox mBounds{};

        /**
         * @brief 공유 인덱스 버퍼를 분할하는 재질별 구간입니다.
         */
        Core::TArray<Asset::FSubMesh> mSubMeshes{};

        /**
         * @brief 원본 재질 이름과 프로젝트 재질 에셋 경로입니다.
         */
        Core::TArray<Asset::FMeshMaterialSlot> mMaterialSlots{};
    };

}
