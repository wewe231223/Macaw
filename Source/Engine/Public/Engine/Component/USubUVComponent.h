/**
 * @file USubUVComponent.h
 * @brief 아틀라스 격자 UV의 자동 재생을 제공합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Engine/Component/UActorComponent.h"
#include "USubUVComponent.generated.h"

namespace Engine {

    class UPrimitiveComponent;

    /**
     * @brief 같은 액터의 스프라이트 또는 메시 UV를 등록 직후부터 격자 프레임으로 재생합니다.
     */
    UCLASS()
    class USubUVComponent final : public Engine::UActorComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief 자동 재생과 에디터 Tick을 활성화한 SubUV 컴포넌트를 생성합니다.
         */
        USubUVComponent();

        /**
         * @brief 대상 컴포넌트에 적용한 UV 변환을 해제합니다.
         */
        ~USubUVComponent() override;

    public:
        /**
         * @brief 대상 컴포넌트 이름을 반환합니다. 빈 이름은 자동 선택을 뜻합니다.
         * @return 이 객체가 보관하는 값의 참조입니다.
         */
        const Core::FString& GetTargetName() const;

        /**
         * @brief 같은 액터에서 이름으로 찾을 대상 컴포넌트를 설정합니다.
         * @param[in] Name 조회하거나 설정할 이름입니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         * @note 빈 이름이면 같은 액터의 첫 Billboard 또는 Sprite를 선택하며, 없으면 첫 정적 메시를 사용합니다.
         */
        bool SetTargetName(const Core::FString& Name);

        /**
         * @brief 현재 SubUV 대상 컴포넌트의 비소유 주소를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Engine::UPrimitiveComponent* GetTarget() const;

        /**
         * @brief 아틀라스의 가로 셀 수를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::Uint32 GetColumns() const;

        /**
         * @brief 아틀라스의 가로 셀 수를 설정합니다.
         * @param[in] Columns 1에서 256 사이의 가로 셀 수입니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetColumns(Core::Uint32 Columns);

        /**
         * @brief 아틀라스의 세로 셀 수를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::Uint32 GetRows() const;

        /**
         * @brief 아틀라스의 세로 셀 수를 설정합니다.
         * @param[in] Rows 1에서 256 사이의 세로 셀 수입니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetRows(Core::Uint32 Rows);

        /**
         * @brief 재생할 셀 수를 반환합니다. 0은 전체 격자를 뜻합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::Uint32 GetFrameCount() const;

        /**
         * @brief 재생할 셀 수를 설정합니다. 0이면 전체 격자를 재생합니다.
         * @param[in] FrameCount 최대 65536개이며 0은 전체 격자를 뜻합니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetFrameCount(Core::Uint32 FrameCount);

        /**
         * @brief 초당 재생 프레임 수를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetFramesPerSecond() const;

        /**
         * @brief 초당 재생 프레임 수를 설정합니다.
         * @param[in] FramesPerSecond 0에서 10000 사이의 유한한 초당 프레임 수입니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetFramesPerSecond(float FramesPerSecond);

        /**
         * @brief 마지막 프레임 이후 반복 여부인지 검사합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool IsLooping() const;

        /**
         * @brief 마지막 프레임 이후 반복 여부를 설정합니다.
         * @param[in] Looping 마지막 셀 다음에 첫 셀로 돌아갈지 지정합니다.
         */
        void SetLooping(bool Looping);

        /**
         * @brief SubUV 재생 상태인지 검사합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool IsPlaying() const;

        /**
         * @brief SubUV 재생 상태를 설정합니다.
         * @param[in] Playing 재생을 진행할지 지정합니다.
         */
        void SetPlaying(bool Playing);

        /**
         * @brief 현재 0 기반 프레임 번호를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::Index_t GetCurrentFrame() const;

        /**
         * @brief 완료된 애니메이션을 되감고 현재 위치부터 SubUV 재생을 시작합니다.
         */
        void Play();

        /**
         * @brief 현재 프레임을 유지하면서 SubUV 재생을 멈춥니다.
         */
        void Pause();

        /**
         * @brief 첫 프레임으로 되감고 SubUV 재생을 멈춥니다.
         */
        void Stop();

        /**
         * @brief 컴포넌트의 실행 또는 허용된 편집 갱신 시 호출되는 확장 지점입니다.
         * @param[in] DeltaTime 이번 갱신이 진행하는 초 단위 시간입니다.
         */
        void Tick(float DeltaTime) override;

        /**
         * @brief 등록된 렌더 상태와 월드 연결을 해제합니다.
         */
        void UnregisterComponent() override;

    private:
        /**
         * @brief 월드에 등록된 컴포넌트가 참조하는 에셋과 렌더 상태를 준비합니다.
         */
        void OnRegister() override;

        /**
         * @brief 같은 액터의 대상을 찾고 현재 프레임의 UV 배율과 오프셋을 적용합니다.
         */
        void ApplyFrame();

        /**
         * @brief 대상에 적용한 UV 변환을 해제하고 대상 핸들을 비웁니다.
         */
        void ClearTarget();

        /**
         * @brief 전체 격자 수로 제한한 실제 재생 프레임 수를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::Uint32 GetEffectiveFrameCount() const;

    private:
        /**
         * @brief 대상 컴포넌트 이름이며 빈 문자열은 같은 액터에서 자동 선택입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "TargetComponent", Getter = GetTargetName, Setter = SetTargetName)
        Core::FString mTargetName{};

        /**
         * @brief 아틀라스의 가로 셀 수입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetColumns, Setter = SetColumns)
        Core::Uint32 mColumns{4};

        /**
         * @brief 아틀라스의 세로 셀 수입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetRows, Setter = SetRows)
        Core::Uint32 mRows{4};

        /**
         * @brief 재생할 셀 수이며 0은 전체 격자입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetFrameCount, Setter = SetFrameCount)
        Core::Uint32 mFrameCount{};

        /**
         * @brief 초당 재생 프레임 수입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetFramesPerSecond, Setter = SetFramesPerSecond)
        float mFramesPerSecond{12.0f};

        /**
         * @brief 마지막 프레임 이후 반복 여부입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = IsLooping, Setter = SetLooping)
        bool mLooping{true};

        /**
         * @brief SubUV 재생 상태입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = IsPlaying, Setter = SetPlaying)
        bool mPlaying{true};

        /**
         * @brief 같은 액터에서 자동 선택하거나 이름으로 지정한 UV 대상의 세대 핸들입니다.
         */
        CoreUObject::FObjectHandle mTarget{};

        /**
         * @brief 소수 프레임을 포함한 현재 재생 위치입니다.
         */
        double mFramePosition{};
    };

}
