/**
 * @file USubUVComponent.cpp
 * @brief 아틀라스 격자 UV의 자동 재생을 제공합니다.
 * @ingroup Engine
 */

#include "Engine/Component/USubUVComponent.h"
#include "Engine/Component/UBillboardComponent.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/AActor.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"

#include <algorithm>
#include <cmath>

namespace Engine {

    /**
     * @brief 자동 재생과 에디터 Tick을 활성화한 SubUV 컴포넌트를 생성합니다.
     */
    Engine::USubUVComponent::USubUVComponent() {
        SetEditorTickable(true);
    }

    /**
     * @brief 대상 컴포넌트에 적용한 UV 변환을 해제합니다.
     */
    Engine::USubUVComponent::~USubUVComponent() {
        ClearTarget();
    }

    /**
     * @brief 대상 컴포넌트 이름을 반환합니다. 빈 이름은 자동 선택을 뜻합니다.
     */
    const Core::FString& Engine::USubUVComponent::GetTargetName() const {
        return mTargetName;
    }

    /**
     * @brief 같은 액터에서 이름으로 찾을 대상 컴포넌트를 설정합니다.
     */
    bool Engine::USubUVComponent::SetTargetName(const Core::FString& Name) {
        if (Name.size() > 255 || Name.find('\0') != Core::FString::npos) {
            return false;
        }

        ClearTarget();

        mTargetName = Name;

        ApplyFrame();

        return true;
    }

    /**
     * @brief 현재 SubUV 대상 컴포넌트의 비소유 주소를 반환합니다.
     */
    Engine::UPrimitiveComponent* Engine::USubUVComponent::GetTarget() const {
        return CoreUObject::Cast<Engine::UPrimitiveComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(mTarget));
    }

    /**
     * @brief 아틀라스의 가로 셀 수를 반환합니다.
     */
    Core::Uint32 Engine::USubUVComponent::GetColumns() const {
        return mColumns;
    }

    /**
     * @brief 아틀라스의 가로 셀 수를 설정합니다.
     */
    bool Engine::USubUVComponent::SetColumns(Core::Uint32 Columns) {
        if (Columns == 0 || Columns > 256) {
            return false;
        }

        mColumns = Columns;
        mFramePosition = 0.0;

        ApplyFrame();

        return true;
    }

    /**
     * @brief 아틀라스의 세로 셀 수를 반환합니다.
     */
    Core::Uint32 Engine::USubUVComponent::GetRows() const {
        return mRows;
    }

    /**
     * @brief 아틀라스의 세로 셀 수를 설정합니다.
     */
    bool Engine::USubUVComponent::SetRows(Core::Uint32 Rows) {
        if (Rows == 0 || Rows > 256) {
            return false;
        }

        mRows = Rows;
        mFramePosition = 0.0;

        ApplyFrame();

        return true;
    }

    /**
     * @brief 재생할 셀 수를 반환합니다. 0은 전체 격자를 뜻합니다.
     */
    Core::Uint32 Engine::USubUVComponent::GetFrameCount() const {
        return mFrameCount;
    }

    /**
     * @brief 재생할 셀 수를 설정합니다. 0이면 전체 격자를 재생합니다.
     */
    bool Engine::USubUVComponent::SetFrameCount(Core::Uint32 FrameCount) {
        if (FrameCount > 65536) {
            return false;
        }

        mFrameCount = FrameCount;
        mFramePosition = 0.0;

        ApplyFrame();

        return true;
    }

    /**
     * @brief 초당 재생 프레임 수를 반환합니다.
     */
    float Engine::USubUVComponent::GetFramesPerSecond() const {
        return mFramesPerSecond;
    }

    /**
     * @brief 초당 재생 프레임 수를 설정합니다.
     */
    bool Engine::USubUVComponent::SetFramesPerSecond(float FramesPerSecond) {
        if (!std::isfinite(FramesPerSecond) || FramesPerSecond < 0.0f || FramesPerSecond > 10000.0f) {
            return false;
        }

        mFramesPerSecond = FramesPerSecond;

        return true;
    }

    /**
     * @brief 마지막 프레임 이후 반복 여부인지 검사합니다.
     */
    bool Engine::USubUVComponent::IsLooping() const {
        return mLooping;
    }

    /**
     * @brief 마지막 프레임 이후 반복 여부를 설정합니다.
     */
    void Engine::USubUVComponent::SetLooping(bool Looping) {
        mLooping = Looping;
    }

    /**
     * @brief SubUV 재생 상태인지 검사합니다.
     */
    bool Engine::USubUVComponent::IsPlaying() const {
        return mPlaying;
    }

    /**
     * @brief SubUV 재생 상태를 설정합니다.
     */
    void Engine::USubUVComponent::SetPlaying(bool Playing) {
        mPlaying = Playing;

        ApplyFrame();
    }

    /**
     * @brief 전체 격자 수로 제한한 실제 재생 프레임 수를 반환합니다.
     */
    Core::Uint32 Engine::USubUVComponent::GetEffectiveFrameCount() const {
        return mFrameCount == 0 ? mColumns * mRows : std::min(mFrameCount, mColumns * mRows);
    }

    /**
     * @brief 현재 0 기반 프레임 번호를 반환합니다.
     */
    Core::Uint32 Engine::USubUVComponent::GetCurrentFrame() const {
        return std::min(static_cast<Core::Uint32>(mFramePosition), GetEffectiveFrameCount() - 1);
    }

    /**
     * @brief 완료된 애니메이션을 되감고 현재 위치부터 SubUV 재생을 시작합니다.
     */
    void Engine::USubUVComponent::Play() {
        if (mFramePosition >= static_cast<double>(GetEffectiveFrameCount())) {
            mFramePosition = 0.0;
        }

        SetPlaying(true);
    }

    /**
     * @brief 현재 프레임을 유지하면서 SubUV 재생을 멈춥니다.
     */
    void Engine::USubUVComponent::Pause() {
        SetPlaying(false);
    }

    /**
     * @brief 첫 프레임으로 되감고 SubUV 재생을 멈춥니다.
     */
    void Engine::USubUVComponent::Stop() {
        mFramePosition = 0.0;

        SetPlaying(false);
    }

    /**
     * @brief 컴포넌트의 실행 또는 허용된 편집 갱신 시 호출되는 확장 지점입니다.
     */
    void Engine::USubUVComponent::Tick(float DeltaTime) {
        if (mPlaying && std::isfinite(DeltaTime) && DeltaTime > 0.0f) {
            const double Next{mFramePosition + static_cast<double>(DeltaTime) * mFramesPerSecond};
            const double Count{static_cast<double>(GetEffectiveFrameCount())};

            mFramePosition = mLooping ? std::fmod(Next, Count) : std::min(Next, Count);

            if (!mLooping && Next >= Count) {
                mPlaying = false;
            }
        }

        ApplyFrame();
    }

    /**
     * @brief 월드에 등록된 컴포넌트가 참조하는 에셋과 렌더 상태를 준비합니다.
     */
    void Engine::USubUVComponent::OnRegister() {
        ApplyFrame();
    }

    /**
     * @brief 등록된 렌더 상태와 월드 연결을 해제합니다.
     */
    void Engine::USubUVComponent::UnregisterComponent() {
        ClearTarget();
        Engine::UActorComponent::UnregisterComponent();
    }

    /**
     * @brief 대상에 적용한 UV 변환을 해제하고 대상 핸들을 비웁니다.
     */
    void Engine::USubUVComponent::ClearTarget() {
        if (Engine::UPrimitiveComponent* Target{GetTarget()}) {
            Target->ClearUVTransform(GetHandle());
        }

        mTarget = {};
    }

    /**
     * @brief 같은 액터의 대상을 찾고 현재 프레임의 UV 배율과 오프셋을 적용합니다.
     */
    void Engine::USubUVComponent::ApplyFrame() {
        if (!IsRegistered() || GetOwner() == nullptr) {
            return;
        }

        Engine::UPrimitiveComponent* Target{GetTarget()};

        if (Target == nullptr) {
            if (!mTargetName.empty()) {
                Target = CoreUObject::Cast<Engine::UPrimitiveComponent>(CoreUObject::FObjectRegistry::Get()->FindObject(Core::FName{mTargetName}, GetOwner()));

                if (Target != nullptr && !Target->IsA<Engine::UBillboardComponent>() && !Target->IsA<Engine::UStaticMeshComponent>()) {
                    Target = nullptr;
                }
            } else {
                for (const auto& Component : GetOwner()->GetComponents()) {
                    auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())};

                    if (Primitive != nullptr && (Primitive->IsA<Engine::UBillboardComponent>() || Primitive->IsA<Engine::UStaticMeshComponent>())) {
                        if (Target == nullptr || Primitive->IsA<Engine::UBillboardComponent>()) {
                            Target = Primitive;
                        }

                        if (Primitive->IsA<Engine::UBillboardComponent>()) {
                            break;
                        }
                    }
                }
            }

            mTarget = Target != nullptr ? Target->GetHandle() : CoreUObject::FObjectHandle{};
        }

        if (Target != nullptr) {
            const Core::Uint32 Frame{GetCurrentFrame()};

            Target->SetUVTransform({1.0f / mColumns, 1.0f / mRows}, {static_cast<float>(Frame % mColumns) / mColumns, static_cast<float>(Frame / mColumns) / mRows}, GetHandle());
        }
    }

}
