/**
 * @file FMeshImportTransform.h
 * @brief 원본 메시 공간의 위치·방향·탄젠트 부호를 엔진 공간으로 옮기는 임포터 공용 변환입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector4.h"
#include "Asset/Import/FAssetImportOption.h"

namespace Asset {

    /**
     * @brief 원본 메시 공간의 위치·방향·탄젠트 부호를 엔진 공간으로 옮기는 임포터 공용 변환입니다.
     * @details 원본 데이터는 오른손 좌표계·CCW 앞면, 엔진은 Z-Up 왼손 좌표계·CW 앞면(D3D 기본 래스터라이저)을 가정합니다.
     * 전체 변환의 행렬식이 음수이면(좌표계의 손 방향이 바뀌면) 화면에서 감김 순서가 유지되므로 삼각형 순서를 뒤집어야 합니다.
     * @ingroup Asset
     */
    class FMeshImportTransform {
    public:
        /**
         * @brief 원본 메시 공간에서 엔진 공간으로 가는 행렬로 변환을 구성합니다.
         * @param[in] SourceToEngine 노드 변환·축 변환·임포트 옵션 변환을 모두 합친 행 벡터 규약의 아핀 행렬입니다.
         */
        explicit FMeshImportTransform(const Core::FMatrix& SourceToEngine);

    public:
        /**
         * @brief 원본 공간 좌표에서 Y만 반전해 왼손 좌표계로 옮기는 행렬을 만듭니다.
         * @return Y 성분의 부호를 바꾸는 행렬입니다.
         */
        static Core::FMatrix MakeFlipY();

        /**
         * @brief Y-Up 오른손 좌표계(Front +Z) 원본을 FBX 임포터와 같은 규칙으로 엔진 좌표계로 옮기는 행렬을 만듭니다.
         * @details FBX 임포터는 SDK의 ConvertScene 뒤에 Y를 반전하며, 이 행렬은 같은 결과를 직접 계산합니다.
         * @param[in] Option 축 변환 옵션입니다.
         * @return 원본 축을 엔진 축으로 옮기는 행렬이며 임포트 이동·회전·스케일은 포함하지 않습니다.
         */
        static Core::FMatrix MakeYUpConversion(const Asset::FAssetImportOption& Option);

        /**
         * @brief 원본 공간 기준 변환을 엔진 공간 기준 변환으로 바꿉니다.
         * @param[in] Basis 원본 공간 좌표를 엔진 공간 좌표로 옮기는 축 변환 행렬입니다.
         * @param[in] Source 원본 공간에서 표현한 변환입니다.
         * @param[out] Result 엔진 공간에서 표현한 변환을 기록할 대상입니다.
         * @return Basis가 역행렬을 가지면 true입니다.
         */
        static bool ConvertTransform(const Core::FMatrix& Basis, const Core::FMatrix& Source, Core::FMatrix& Result);

        /**
         * @brief 변환이 역행렬을 가져 노멀을 변환할 수 있는지 확인합니다.
         * @return 행렬식이 유한하고 0이 아니면 true입니다.
         */
        bool IsValid() const;

        /**
         * @brief 원본 삼각형의 꼭짓점 순서를 뒤집어야 하는지 확인합니다.
         * @return 전체 변환의 행렬식이 음수이면 true입니다.
         */
        bool ReversesWinding() const;

        /**
         * @brief 이동을 포함해 위치를 변환합니다.
         * @param[in] Position 원본 공간 위치입니다.
         * @return 엔진 공간 위치입니다.
         */
        Core::FVector TransformPosition(const Core::FVector& Position) const;

        /**
         * @brief 이동을 제외하고 표면을 따르는 방향(탄젠트·바이탄젠트)을 변환합니다.
         * @param[in] Direction 원본 공간 방향입니다.
         * @return 정규화하지 않은 엔진 공간 방향입니다.
         */
        Core::FVector TransformDirection(const Core::FVector& Direction) const;

        /**
         * @brief 역전치 행렬로 노멀을 변환합니다.
         * @param[in] Normal 원본 공간 노멀입니다.
         * @return 정규화하지 않은 엔진 공간 노멀입니다.
         */
        Core::FVector TransformNormal(const Core::FVector& Normal) const;

        /**
         * @brief 엔진 공간 노멀과 원본 탄젠트·바이탄젠트로 직교 탄젠트와 부호를 만듭니다.
         * @details 바이탄젠트는 원본 UV의 +V 방향입니다. 엔진은 V를 1 - V로 뒤집으므로 셰이더의
         * Bitangent = w × (Normal × Tangent)가 뒤집힌 V 방향을 가리키도록 w를 정합니다.
         * @param[in] Normal 정규화한 엔진 공간 노멀입니다.
         * @param[in] SourceTangent 원본 공간 탄젠트이며 없으면 0 벡터입니다.
         * @param[in] SourceBitangent 원본 공간 바이탄젠트이며 없으면 0 벡터입니다.
         * @param[in] SourceSign 바이탄젠트가 없을 때 쓰는 원본 탄젠트 w 부호이며 0이면 +1로 봅니다.
         * @return XYZ는 단위 탄젠트, W는 ±1 부호입니다. 탄젠트가 없거나 노멀과 평행하면 노멀에 수직인 임의 탄젠트입니다.
         */
        Core::FVector4 MakeTangent(const Core::FVector& Normal, const Core::FVector& SourceTangent, const Core::FVector& SourceBitangent, float SourceSign) const;

    private:
        /**
         * @brief 원본 공간에서 엔진 공간으로 가는 아핀 행렬입니다.
         */
        Core::FMatrix mMatrix{};

        /**
         * @brief 노멀 변환에 쓰는 mMatrix의 역전치 행렬이며 이동 성분은 0입니다.
         */
        Core::FMatrix mNormalMatrix{};

        /**
         * @brief mMatrix의 행렬식이 유한하고 0이 아닌지 여부입니다.
         */
        bool mValid{};

        /**
         * @brief mMatrix의 행렬식이 음수인지 여부입니다.
         */
        bool mReversesWinding{};
    };

}
