/**
 * @file FSequencePlayer.h
 * @brief 시퀀스의 시간을 진행하고 월드 객체에 값을 적용하는 플레이어에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Math/FTransform.h"
#include "CoreUObject/FObjectHandle.h"
#include "Engine/Sequence/ESequencePlayState.h"
#include "Engine/Sequence/FSequenceBindingTarget.h"

namespace Asset {
    class FSequenceBinding;
    class ULevelSequence;
}

namespace Engine {

    class UWorld;
    class USceneComponent;
    class UCameraComponent;

    /**
     * @brief 시퀀스의 시간을 진행하고 월드 객체에 값을 적용합니다.
     * @ingroup Engine
     * @note 시퀀스와 월드를 소유하지 않으며 값을 처음 쓰기 전의 변환을 보관해 복원합니다.
     */
    class FSequencePlayer {
    public:
        /**
         * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
         */
        ReflectTypes(FSequencePlayer);

    private:
        /**
         * @brief 값을 처음 쓰기 전의 프로퍼티 값입니다.
         */
        struct FSavedProperty {
        public:
            /**
             * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
             */
            ReflectTypes(FSavedProperty);

        public:
            /**
             * @brief 정적 구조체 메타데이터를 반환합니다.
             * @return 메타데이터의 비소유 주소입니다.
             */
            ReflectStruct(FSavedProperty);

        private:
            /**
             * @brief 생성된 멤버 등록 함수의 접근 권한입니다.
             */
            ReflectMembers();

        public:
            /**
             * @brief 프로퍼티의 리플렉션 이름입니다.
             */
            Core::FString mName{};
            ReflectProperty(mName, "Name", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

            /**
             * @brief 값을 처음 쓰기 전의 프로퍼티 값입니다.
             */
            float mValue{};
            ReflectProperty(mValue, "Value", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        };

        /**
         * @brief 월드 객체로 해석한 바인딩과 원래 값입니다.
         */
        struct FResolvedBinding {
        public:
            /**
             * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
             */
            ReflectTypes(FResolvedBinding);

        public:
            /**
             * @brief 정적 구조체 메타데이터를 반환합니다.
             * @return 메타데이터의 비소유 주소입니다.
             */
            ReflectStruct(FResolvedBinding);

        private:
            /**
             * @brief 생성된 멤버 등록 함수의 접근 권한입니다.
             */
            ReflectMembers();

        public:
            /**
             * @brief 시퀀스가 소유한 바인딩의 비소유 주소입니다.
             */
            const Asset::FSequenceBinding* mBinding{};
            ReflectProperty(mBinding, "Binding", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

            /**
             * @brief 값을 적용할 공간 컴포넌트의 핸들입니다.
             */
            CoreUObject::FObjectHandle mComponent{};
            ReflectProperty(mComponent, "Component", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

            /**
             * @brief 값을 처음 쓰기 전의 변환입니다.
             */
            Core::FTransform mBaseTransform{};
            ReflectProperty(mBaseTransform, "BaseTransform", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

            /**
             * @brief 원래 변환을 보관했는지 여부입니다.
             */
            bool mCaptured{};
            ReflectProperty(mCaptured, "Captured", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

            /**
             * @brief 값을 처음 쓰기 전의 프로퍼티 값 목록입니다.
             */
            Core::TArray<FSavedProperty> mBaseProperties{};
            ReflectProperty(mBaseProperties, "BaseProperties", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        };

    public:
        /**
         * @brief 정적 구조체 메타데이터를 반환합니다.
         * @return 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSequencePlayer);

        /**
         * @brief 시퀀스의 바인딩을 월드 객체로 해석하고 정지 상태로 초기화합니다.
         * @param[in] Sequence 재생할 시퀀스이며 플레이어보다 오래 유지해야 합니다.
         * @param[in] World 바인딩 대상을 찾을 월드입니다.
         * @param[in] Targets 바인딩 ID와 씬 객체의 연결 목록입니다.
         * @return 시퀀스와 월드가 유효하면 true이며 찾지 못한 바인딩은 경고 후 건너뜁니다.
         */
        bool Initialize(const Asset::ULevelSequence* Sequence, Engine::UWorld* World, const Core::TArray<Engine::FSequenceBindingTarget>& Targets);

        /**
         * @brief 시퀀스 연결과 보관한 변환을 값 복원 없이 해제합니다.
         */
        void Reset();

        /**
         * @brief 재생을 시작하거나 일시정지를 해제하고 현재 시간의 값을 적용합니다.
         * @return 시퀀스가 연결되어 있으면 true입니다.
         */
        bool Play();

        /**
         * @brief 재생 중이면 현재 시간에서 일시정지합니다.
         */
        void Pause();

        /**
         * @brief 재생을 끝내고 복원 설정이 켜져 있으면 원래 변환을 복원합니다.
         */
        void Stop();

        /**
         * @brief 재생 중이면 시간을 진행하고 값을 적용합니다.
         * @param[in] DeltaTime 진행할 시간이며 초 단위입니다.
         */
        void Advance(float DeltaTime);

        /**
         * @brief 재생 구간 안의 시간으로 이동하여 값을 적용합니다.
         * @param[in] Tick 이동할 시간이며 틱 단위입니다.
         * @note 정지 상태이면 일시정지로 바꾸어 다음 재생을 이 시간부터 진행합니다.
         */
        void Seek(double Tick);

        /**
         * @brief 바인딩에 연결된 공간 컴포넌트를 조회합니다.
         * @param[in] BindingId 시퀀스 바인딩 ID입니다.
         * @return 살아 있는 컴포넌트의 비소유 주소이며 없으면 nullptr입니다.
         */
        Engine::USceneComponent* FindBoundComponent(const Core::FGuid& BindingId) const;

        /**
         * @brief 현재 카메라 컷이 활성화한 카메라를 조회합니다.
         * @return 살아 있는 카메라의 비소유 주소이며 적용된 컷이 없으면 nullptr입니다.
         */
        Engine::UCameraComponent* GetCutCamera() const;

        /**
         * @brief 값을 쓴 객체와 활성 카메라를 원래 상태로 되돌립니다.
         */
        void RestoreState();

        /**
         * @brief 끝에 도달했을 때 시작 시간으로 돌아갈지 설정합니다.
         * @param[in] Looping 반복 재생 여부입니다.
         */
        void SetLooping(bool Looping);

        /**
         * @brief 재생이 끝났을 때 원래 변환을 복원할지 설정합니다.
         * @param[in] RestoreOnFinish 복원 여부입니다.
         */
        void SetRestoreOnFinish(bool RestoreOnFinish);

        /**
         * @brief 현재 재생 상태를 반환합니다.
         * @return 재생 상태입니다.
         */
        Engine::ESequencePlayState GetState() const;

        /**
         * @brief 현재 재생 시간을 반환합니다.
         * @return 틱 단위 재생 시간입니다.
         */
        double GetTick() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수의 접근 권한입니다.
         */
        ReflectMembers();

        /**
         * @brief 현재 시간의 값을 바인딩 대상에 적용합니다.
         */
        void Evaluate();

        /**
         * @brief 현재 시간의 카메라 컷을 월드의 활성 카메라에 적용합니다.
         */
        void EvaluateCameraCut();

    private:
        /**
         * @brief 바인딩을 해석하고 활성 카메라를 바꿀 월드의 비소유 주소입니다.
         */
        Engine::UWorld* mWorld{};
        ReflectProperty(mWorld, "World", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 첫 카메라 컷을 적용하기 전의 활성 카메라 핸들입니다.
         */
        CoreUObject::FObjectHandle mBaseCamera{};
        ReflectProperty(mBaseCamera, "BaseCamera", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 카메라 컷이 활성화한 카메라 핸들입니다.
         */
        CoreUObject::FObjectHandle mCutCamera{};
        ReflectProperty(mCutCamera, "CutCamera", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 원래 활성 카메라를 보관했는지 여부입니다.
         */
        bool mCameraCaptured{};
        ReflectProperty(mCameraCaptured, "CameraCaptured", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 재생할 시퀀스의 비소유 주소입니다.
         */
        const Asset::ULevelSequence* mSequence{};
        ReflectProperty(mSequence, "Sequence", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 월드 객체로 해석한 바인딩 목록입니다.
         */
        Core::TArray<FResolvedBinding> mBindings{};
        ReflectProperty(mBindings, "Bindings", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 틱 단위 현재 재생 시간입니다.
         */
        double mTick{};
        ReflectProperty(mTick, "Tick", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 재생 상태입니다.
         */
        Engine::ESequencePlayState mState{Engine::ESequencePlayState::Stopped};
        ReflectProperty(mState, "State", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 끝에 도달했을 때 시작 시간으로 돌아갈지 나타냅니다.
         */
        bool mLooping{};
        ReflectProperty(mLooping, "Looping", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 재생이 끝났을 때 원래 변환을 복원할지 나타냅니다.
         */
        bool mRestoreOnFinish{};
        ReflectProperty(mRestoreOnFinish, "RestoreOnFinish", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
