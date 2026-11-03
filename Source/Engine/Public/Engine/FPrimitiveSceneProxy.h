/**
 * @file FPrimitiveSceneProxy.h
 * @brief 컴포넌트에서 복사한 식별자, 메시 종류, 월드 행렬과 경계 상자입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Guid.h"
#include "Core/FRevision.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FBox.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/FDrawText.h"
#include "Engine/FDrawSprite.h"

#include <optional>

namespace Asset {
    class UStaticMesh;
    class USkeletalMesh;
    class UMaterial;
}

namespace Engine {

    /**
     * @brief 컴포넌트에서 복사한 식별자, 메시 종류, 월드 행렬과 경계 상자입니다.
     * @ingroup Engine
     */
    struct FPrimitiveSceneProxy {
    public:
        /**
         * @brief 렌더 프리미티브의 영속 식별자입니다.
         */
        Core::FGuid mId{};

        /**
         * @brief 프리미티브 메시 종류입니다.
         */
        Engine::EPrimitiveType mPrimitive{Engine::EPrimitiveType::Cube};

        /**
         * @brief 프리미티브의 월드 변환 행렬입니다.
         */
        Core::FMatrix mWorld{};

        /**
         * @brief 축 정렬 경계 상자입니다.
         */
        Core::FBox mBounds{};

        /**
         * @brief 공통 드로에 사용할 형상이며 스켈레탈 메시에서는 바인드 형상입니다.
         */
        Asset::UStaticMesh* mMesh{};

        /**
         * @brief 스키닝할 공유 에셋이며 mMesh는 이 에셋의 바인드 형상을 가리킵니다.
         */
        Asset::USkeletalMesh* mSkeletalMesh{};

        /**
         * @brief 이 객체의 바인드 위치를 현재 메시 공간 위치로 옮기는 본 행렬입니다.
         */
        Core::TArray<Core::FMatrix> mSkinMatrices{};

        /**
         * @brief 월드 행렬이 같아도 포즈 업로드와 그림자 무효화를 구분하는 개정입니다.
         */
        Core::FRevision mPoseRevision{};

        /**
         * @brief 텍스트 컴포넌트가 그릴 텍스트 요청이며, 텍스트가 없는 프리미티브는 비어 있습니다.
         */
        std::optional<Engine::FDrawText> mText{};

        /**
         * @brief 연결된 머티리얼의 비소유 주소입니다.
         */
        Asset::UMaterial* mMaterial{};

        /**
         * @brief SubMesh의 재질 인덱스로 조회하는 슬롯별 비소유 재질 주소입니다.
         */
        Core::TArray<Asset::UMaterial*> mMaterials{};

        /**
         * @brief 스프라이트 전용 렌더 데이터입니다.
         */
        std::optional<Engine::FDrawSprite> mSprite{};

        /**
         * @brief 기본 UV에 곱할 셀 크기입니다.
         */
        Core::FVector2 mUVScale{1.0f, 1.0f};

        /**
         * @brief 기본 UV에 더할 셀 원점입니다.
         */
        Core::FVector2 mUVOffset{};

        /**
         * @brief 프리미티브의 그림자 생성 여부입니다.
         */
        bool mCastShadow{true};

        /**
         * @brief 프리미티브의 그림자 수신 여부입니다.
         */
        bool mReceiveShadow{true};
    };

}
