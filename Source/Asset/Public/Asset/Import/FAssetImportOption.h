/**
 * @file FAssetImportOption.h
 * @brief 메시 임포터가 공통으로 따르는 좌표축·변환·메시 구성·재질 임포트 옵션입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FVector.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "FAssetImportOption.generated.h"

namespace Asset {

    /**
     * @brief 원본 노멀과 탄젠트를 그대로 쓸지 다시 계산할지 정합니다.
     */
    UENUM()
    enum class ENormalImportMethod : Core::Uint8 {
        /**
         * @brief 원본 노멀과 탄젠트를 버리고 모두 다시 계산합니다.
         */
        ComputeNormals = 0,
        /**
         * @brief 원본 노멀을 사용하고 탄젠트는 다시 계산합니다.
         */
        ImportNormals = 1,
        /**
         * @brief 원본 노멀과 탄젠트를 사용하고 없는 요소만 계산합니다.
         */
        ImportNormalsAndTangents = 2
    };

    /**
     * @brief 메시 임포터가 공통으로 따르는 좌표축·변환·메시 구성·재질 임포트 옵션입니다.
     * @details 원본 좌표계는 옵션과 무관하게 항상 엔진의 Z-Up 왼손 좌표계로 변환하며, Front 축의 기본값은 언리얼 FBX 임포터의 기본값(Force Front XAxis 끔)과 같습니다.
     * 단위 변환은 엔진의 월드 단위가 미터이므로 언리얼과 달리 기본으로 켭니다.
     * @ingroup Asset
     */
    USTRUCT()
    struct FAssetImportOption {
        GENERATED_BODY()

    public:
        /**
         * @brief 이동·회전·균일 스케일 옵션을 엔진 공간에서 적용하는 행렬을 만듭니다.
         * @return 스케일, 회전, 이동 순서로 적용하는 행 벡터 규약의 행렬입니다.
         */
        Core::FMatrix GetImportTransform() const;

        /**
         * @brief 옵션 값이 임포트에 사용할 수 있는지 확인합니다.
         * @return 이동·회전·스케일이 유한하고 스케일이 0이 아니면 true입니다.
         */
        bool IsValid() const;

    public:
        /**
         * @brief 원본의 Front 축을 엔진의 +X(Forward)로 맞춥니다. 끄면 언리얼과 같이 Front가 +Y가 됩니다.
         */
        UPROPERTY(Edit)
        bool mForceFrontXAxis{false};

        /**
         * @brief 원본 파일에 기록된 길이 단위를 엔진 단위(미터)로 변환합니다.
         */
        UPROPERTY(Edit)
        bool mConvertSceneUnit{true};

        /**
         * @brief 축 변환 뒤 엔진 공간에서 정점에 더할 이동입니다.
         */
        UPROPERTY(Edit)
        Core::FVector mImportTranslation{};

        /**
         * @brief 축 변환 뒤 엔진 공간에서 정점에 적용할 회전입니다.
         */
        UPROPERTY(Edit)
        Core::FRotator mImportRotation{};

        /**
         * @brief 축 변환 뒤 엔진 공간에서 정점에 적용할 균일 스케일입니다.
         */
        UPROPERTY(Edit)
        float mImportUniformScale{1.0f};

        /**
         * @brief 파일의 모든 메시를 하나의 메시로 합칩니다. 끄면 메시 노드마다 별도 메시를 만듭니다.
         */
        UPROPERTY(Edit)
        bool mCombineMeshes{true};

        /**
         * @brief 원본에 스켈레톤 계층이 있으면 본 이름·부모·바인드 포즈와 정점별 본 인덱스·가중치를 함께 읽습니다.
         */
        UPROPERTY(Edit)
        bool mImportSkeleton{false};

        /**
         * @brief 원본 노멀과 탄젠트의 사용 방식입니다.
         */
        UPROPERTY(Edit)
        Asset::ENormalImportMethod mNormalImportMethod{Asset::ENormalImportMethod::ImportNormalsAndTangents};

        /**
         * @brief 원본 재질을 머티리얼 에셋으로 만듭니다. 끄면 메시 슬롯이 기본 머티리얼을 사용합니다.
         */
        UPROPERTY(Edit)
        bool mImportMaterials{true};

        /**
         * @brief 재질이 참조하는 텍스처를 텍스처 에셋으로 가져와 머티리얼에 연결합니다.
         */
        UPROPERTY(Edit)
        bool mImportTextures{true};
    };

}
