/**
 * @file UStaticMeshComponent.h
 * @brief 기본 프리미티브 또는 정적 메시 에셋을 머티리얼과 함께 표시하는 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/Component/UMeshComponent.h"
#include "Engine/EPrimitiveType.h"
#include "UStaticMeshComponent.generated.h"

namespace Engine {

    /**
     * @brief 기본 프리미티브 또는 정적 메시 에셋을 머티리얼과 함께 표시하는 컴포넌트입니다.
     * @ingroup Engine
     */
    UCLASS()
    class UStaticMeshComponent final : public Engine::UMeshComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief UStaticMeshComponent의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        UStaticMeshComponent();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UStaticMeshComponent() override;

    public:
        /**
         * @brief 표시할 프리미티브 메시 종류를 반환합니다.
         * @return 표시할 프리미티브 메시 종류입니다.
         */
        Engine::EPrimitiveType GetPrimitiveType() const override;

        /**
         * @brief 지원되는 프리미티브 종류를 검증하고 렌더 상태 갱신을 예약합니다.
         * @param[in] Type 기본 프리미티브 종류입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool SetPrimitiveType(Engine::EPrimitiveType Type);

        /**
         * @brief 이 컴포넌트에 설정된 프로젝트 상대 메시 에셋 경로를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::FString& GetMeshPath() const;

        /**
         * @brief 프로젝트의 메시 에셋 경로를 검증하고 컴포넌트의 렌더 상태를 갱신합니다.
         * @param[in] Path Content 기준 메시 에셋 경로이며 비우면 프리미티브를 사용합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetMeshPath(const Core::FString& Path);

        /**
         * @brief 프리미티브가 사용하는 정적 메시의 비소유 주소를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Asset::UStaticMesh* GetStaticMesh() const override;

        /**
         * @brief 메시의 CPU 정점 속성·인덱스 데이터 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Asset::FMeshData& GetMeshData() const override;

        /**
         * @brief 등록된 렌더 상태와 월드 연결을 해제합니다.
         */
        void UnregisterComponent() override;

        /**
         * @brief 저장된 메시 경로를 에셋 관리자에서 해석하고 렌더 상태를 갱신합니다.
         * @return 경로가 비어 있거나 에셋을 찾았으면 true, 로드에 실패하면 false입니다.
         */
        bool ResolveMesh() override;

        /**
         * @brief 현재 표시·에셋 상태에서 렌더 프록시를 생성할지 판정합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool ShouldCreateRenderState() const override;

    private:
        /**
         * @brief Content 상대 경로의 메시를 에셋 관리자에서 조회합니다.
         * @param[in] Path Content 기준 메시 에셋 경로입니다.
         * @return 캐시된 에셋의 비소유 주소이며, 로드에 실패하면 nullptr입니다.
         */
        Asset::UStaticMesh* LoadMesh(const Core::FString& Path) const;

    private:
        /**
         * @brief 프리미티브 메시 종류입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetPrimitiveType, Setter = SetPrimitiveType)
        Engine::EPrimitiveType mPrimitive{Engine::EPrimitiveType::Cube};

        /**
         * @brief 프로젝트 콘텐츠 폴더 기준 메시 에셋의 상대 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "Mesh", Getter = GetMeshPath, Setter = SetMeshPath)
        Core::FString mMeshPath{};

        /**
         * @brief 에셋 관리자가 보유한 정적 메시의 비소유 주소입니다.
         */
        Asset::UStaticMesh* mStaticMesh{};
    };

}
