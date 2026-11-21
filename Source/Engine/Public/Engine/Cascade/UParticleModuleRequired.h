/**
 * @file UParticleModuleRequired.h
 * @brief 파티클 머티리얼·이미터 변환·시뮬레이션 공간과 반복 설정을 보관합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Cascade/UParticleModule.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FVector.h"
#include "CoreUObject/TObjectRef.h"
#include "Asset/FAssetManager.h"
#include "Asset/UMaterial.h"

#include "UParticleModuleRequired.generated.h"

namespace Engine {

    /**
     * @brief 파티클 머티리얼·이미터 변환·시뮬레이션 공간과 반복 설정을 보관합니다.
     */
    UCLASS()
    class UParticleModuleRequired : public Engine::UParticleModule {
        GENERATED_BODY()

    public:
        /**
         * @brief UParticleModuleRequired의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        UParticleModuleRequired();

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~UParticleModuleRequired() override;

    public:
        /**
         * @brief 필수 모듈에 연결된 머티리얼을 조회합니다.
         * @return 머티리얼의 비소유 주소이며 해석되지 않았거나 파괴되었으면 nullptr입니다.
         */
        Asset::UMaterial* GetMaterial() const;

        /**
         * @brief 머티리얼의 등록 경로와 비소유 참조를 함께 설정합니다.
         * @param[in] Material 연결할 머티리얼의 비소유 주소입니다.
         * @param[in] Assets 참조 에셋의 경로 조회·로드를 제공하는 관리자입니다.
         * @return 등록 경로를 확인해 적용했으면 true이며 nullptr은 연결을 해제합니다.
         */
        bool SetMaterial(Asset::UMaterial* Material, const Asset::FAssetManager& Assets);

        /**
         * @brief 저장된 머티리얼 경로를 에셋 관리자로 해석합니다.
         * @param[in] Assets 참조 에셋의 경로 조회·로드를 제공하는 관리자입니다.
         * @param[out] Error 실패 원인을 전달하는 오류 문자열입니다.
         * @return 경로가 비었거나 머티리얼 로드에 성공하면 true입니다.
         */
        bool ResolveMaterial(Asset::FAssetManager& Assets, Core::FString& Error);

        /**
         * @brief 필수 모듈의 프로젝트 상대 머티리얼 경로를 반환합니다.
         * @return 저장된 머티리얼 에셋 경로입니다.
         */
        const Core::FString& GetMaterialPath() const;

        /**
         * @brief 프로젝트 상대 .masset 경로를 검증하고 머티리얼 경로를 설정합니다.
         * @param[in] Path 대상 에셋 또는 콘텐츠 경로입니다.
         * @return 루트·상위 경로 이동·널 문자가 없는 유효한 경로이면 true입니다.
         */
        bool SetMaterialPath(const Core::FString& Path);

        /**
         * @brief 이미터 기준 생성 원점을 반환합니다.
         * @return 이미터 로컬 위치 오프셋입니다.
         */
        const FVector& GetEmitterOrigin() const;

        /**
         * @brief 유한한 이미터 생성 원점을 설정합니다.
         * @param[in] Origin 이미터 기준 생성 위치 오프셋입니다.
         * @return 모든 성분이 유한해 적용했으면 true입니다.
         */
        bool SetEmitterOrigin(const FVector& Origin);

        /**
         * @brief 이미터 기준 생성 회전을 반환합니다.
         * @return 이미터 로컬 회전 오프셋입니다.
         */
        const FRotator& GetEmitterRotation() const;

        /**
         * @brief 유한한 이미터 생성 회전을 설정합니다.
         * @param[in] Rotation 이미터 기준 생성 회전 오프셋입니다.
         * @return 모든 성분이 유한해 적용했으면 true입니다.
         */
        bool SetEmitterRotation(const FRotator& Rotation);

        /**
         * @brief 파티클을 이미터 로컬 공간에서 시뮬레이션하는지 조회합니다.
         * @return 로컬 공간을 사용하면 true입니다.
         */
        bool GetUseLocalSpace() const;

        /**
         * @brief 파티클 시뮬레이션의 로컬 공간 사용 여부를 설정합니다.
         * @param[in] UseLocalSpace 이미터 로컬 공간에서 시뮬레이션할지 지정합니다.
         */
        void SetUseLocalSpace(bool UseLocalSpace);

        /**
         * @brief 이미터 한 주기의 길이를 반환합니다.
         * @return 주기 길이이며 단위는 초입니다.
         */
        float GetEmitterDuration() const;

        /**
         * @brief 유한한 양수의 이미터 주기 길이를 설정합니다.
         * @param[in] Duration 이미터 한 주기의 초 단위 길이이며 유한한 양수여야 합니다.
         * @return 유효한 초 단위 길이를 적용했으면 true입니다.
         */
        bool SetEmitterDuration(float Duration);

        /**
         * @brief 설정된 이미터 반복 횟수를 반환합니다.
         * @return 반복 횟수 설정 값입니다.
         */
        Uint32 GetEmitterLoops() const;

        /**
         * @brief 이미터 반복 횟수 설정 값을 변경합니다.
         * @param[in] Loops 이미터 반복 횟수 설정 값입니다.
         */
        void SetEmitterLoops(Uint32 Loops);

    private:
        /**
         * @brief 필수 모듈이 참조하는 머티리얼의 비소유 참조입니다.
         */
        CoreUObject::TObjectRef<Asset::UMaterial> mMaterial{};

        /**
         * @brief 프로젝트 상대 머티리얼 에셋 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "Material", Getter = GetMaterialPath, Setter = SetMaterialPath)
        Core::FString mMaterialPath{};

        /**
         * @brief 이미터 기준 생성 위치 오프셋입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetEmitterOrigin, Setter = SetEmitterOrigin)
        FVector mEmitterOrigin{};

        /**
         * @brief 이미터 기준 생성 회전 오프셋입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetEmitterRotation, Setter = SetEmitterRotation)
        FRotator mEmitterRotation{};

        /**
         * @brief 파티클을 이미터 로컬 공간에서 시뮬레이션할지 나타냅니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetUseLocalSpace, Setter = SetUseLocalSpace)
        bool mUseLocalSpace{};

        /**
         * @brief 이미터 한 반복 주기의 길이이며 단위는 초입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetEmitterDuration, Setter = SetEmitterDuration)
        float mEmitterDuration{1.0f};

        /**
         * @brief 이미터 반복 횟수 설정 값입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetEmitterLoops, Setter = SetEmitterLoops)
        Uint32 mEmitterLoops{};
    };

}
