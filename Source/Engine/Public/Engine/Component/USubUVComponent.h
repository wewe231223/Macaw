/**
 * @file USubUVComponent.h
 * @brief 아틀라스 격자 UV의 자동 재생을 제공합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UActorComponent.h"

namespace Engine {

    class UPrimitiveComponent;

    /**
     * @brief 같은 액터의 스프라이트 또는 메시 UV를 등록 직후부터 격자 프레임으로 재생합니다.
     */
    class USubUVComponent final : public Engine::UActorComponent {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(USubUVComponent, Engine::UActorComponent);

    public:
        /**
         * @brief 자동 재생과 에디터 Tick을 활성화한 SubUV 컴포넌트를 생성합니다.
         */
        USubUVComponent();

        /**
         * @brief 객체를 소멸하고 소유한 GPU 자원 또는 UV 제어 상태를 해제합니다.
         */
        ~USubUVComponent() override;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
         */
        ReflectClass(USubUVComponent);

        /**
         * @brief 대상 컴포넌트 이름을 반환합니다. 빈 이름은 자동 선택을 뜻합니다.
         * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
         */
        const Core::FString& GetTargetName() const;

        /**
         * @brief 같은 액터에서 이름으로 찾을 대상 컴포넌트를 설정합니다.
         * @param[in] Name 조회하거나 설정할 이름입니다.
         * @return 검증 또는 요청한 처리에 성공하면 true입니다.
         * @note 빈 이름이면 같은 액터의 첫 Billboard 또는 Sprite를 선택하며, 없으면 첫 정적 메시를 사용합니다.
         */
        bool SetTargetName(const Core::FString& Name);

        /**
         * @brief 현재 SubUV 대상 컴포넌트의 비소유 주소를 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
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
         * @return 검증 또는 요청한 처리에 성공하면 true입니다.
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
         * @return 검증 또는 요청한 처리에 성공하면 true입니다.
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
         * @return 검증 또는 요청한 처리에 성공하면 true입니다.
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
         * @return 검증 또는 요청한 처리에 성공하면 true입니다.
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
        Core::Uint32 GetCurrentFrame() const;

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
        void Tick(float DeltaTime) override;
        void UnregisterComponent() override;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

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
        Core::FString mTargetName{};
        ReflectAccessor(mTargetName, GetTargetName, SetTargetName, "TargetComponent", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 아틀라스의 가로 셀 수입니다.
         */
        Core::Uint32 mColumns{4};
        ReflectAccessor(mColumns, GetColumns, SetColumns, "Columns", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 아틀라스의 세로 셀 수입니다.
         */
        Core::Uint32 mRows{4};
        ReflectAccessor(mRows, GetRows, SetRows, "Rows", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 재생할 셀 수이며 0은 전체 격자입니다.
         */
        Core::Uint32 mFrameCount{};
        ReflectAccessor(mFrameCount, GetFrameCount, SetFrameCount, "FrameCount", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 초당 재생 프레임 수입니다.
         */
        float mFramesPerSecond{12.0f};
        ReflectAccessor(mFramesPerSecond, GetFramesPerSecond, SetFramesPerSecond, "FramesPerSecond", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 마지막 프레임 이후 반복 여부입니다.
         */
        bool mLooping{true};
        ReflectAccessor(mLooping, IsLooping, SetLooping, "Looping", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief SubUV 재생 상태입니다.
         */
        bool mPlaying{true};
        ReflectAccessor(mPlaying, IsPlaying, SetPlaying, "Playing", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 같은 액터에서 자동 선택하거나 이름으로 지정한 UV 대상의 세대 핸들입니다.
         */
        CoreUObject::FObjectHandle mTarget{};
        ReflectProperty(mTarget, "Target", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 소수 프레임을 포함한 현재 재생 위치입니다.
         */
        double mFramePosition{};
        ReflectProperty(mFramePosition, "FramePosition", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
