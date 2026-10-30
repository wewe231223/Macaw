/**
 * @file ULevel.cpp
 * @brief 월드에 속한 액터의 소유권과 컴포넌트 등록 수명을 관리합니다.
 * @ingroup Engine
 */

#include "Engine/ULevel.h"
#include "Engine/AActor.h"
#include "Engine/UWorld.h"
#include "CoreUObject/FObjectHandle.h"

#include <algorithm>
#include <utility>

namespace Engine {

    /**
     * @brief ULevel의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Engine::ULevel::ULevel() = default;

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Engine::ULevel::~ULevel() {
        SetWorld(nullptr);
    }

    /**
     * @brief 객체가 소속된 월드 연결을 지정합니다.
     */
    void Engine::ULevel::SetWorld(Engine::UWorld* World) {
        mWorld = World;

        for (const auto& Actor : mActors) {
            Actor->SetWorld(World);
        }
    }

    /**
     * @brief 검증한 액터의 소유권을 레벨로 옮기고 컴포넌트를 월드에 등록합니다.
     */
    bool Engine::ULevel::AddActor(std::unique_ptr<Engine::AActor> Actor) {
        if (Actor == nullptr || Actor->GetWorld() != nullptr || (mWorld != nullptr && !mWorld->CanAddActor())) {
            return false;
        }

        Engine::UWorld::FDispatchScope Scope{mWorld};
        Engine::AActor* Added{Actor.get()};

        if (!Added->SetOuter(this)) {
            return false;
        }

        mActors.push_back(std::move(Actor));
        Added->SetWorld(mWorld);

        if (mWorld != nullptr) {
            mWorld->NotifyActorAdded(Added);
        }

        return true;
    }

    /**
     * @brief 핸들에 해당하는 액터의 컴포넌트를 등록 해제하고 액터를 제거합니다.
     */
    bool Engine::ULevel::DestroyActor(CoreUObject::FObjectHandle Handle) {
        return mWorld != nullptr ? mWorld->DestroyActor(Handle) : RemoveActor(Handle);
    }

    /**
     * @brief 안전한 삭제 경계에서 실행을 종료하고 액터의 소유권을 해제합니다.
     */
    bool Engine::ULevel::RemoveActor(CoreUObject::FObjectHandle Handle) {
        const auto Position{std::find_if(mActors.begin(), mActors.end(), [Handle](const auto& Actor) {
            return Actor->GetHandle() == Handle;
        })};

        if (Position == mActors.end()) {
            return false;
        }

        auto Removed{std::move(*Position)};

        mActors.erase(Position);
        Removed->SetWorld(nullptr);

        return true;
    }

    /**
     * @brief 레벨이 소유한 액터 목록을 반환합니다.
     */
    const Core::TArray<std::unique_ptr<Engine::AActor>>& Engine::ULevel::GetActors() const {
        return mActors;
    }

}
