/**
 * @file FWorldContext.cpp
 * @brief 하나의 월드와 엔진 식별자를 연결하는 수명 경계입니다.
 * @ingroup Engine
 */

#include "Engine/FWorldContext.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/Reflection/RegisterEngine.h"
#include "CoreUObject/Reflection/NewObject.h"

#include <utility>

namespace Engine {

    /**
     * @brief FWorldContext의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Engine::FWorldContext::FWorldContext(Engine::EWorldType Type) {
        Engine::ReflectionModules::RegisterEngine();

        mWorld = CoreUObject::NewObject<Engine::UWorld>();

        if (mWorld == nullptr) {
            Core::ErrorHandler::Fatal("World", "World creation failed");
        }

        mWorld->mWorldType = Type;
        mWorld->SetName(Type == Engine::EWorldType::Editor ? "EditorWorld" : "World");

        auto Level{CoreUObject::NewObject<Engine::ULevel>()};

        if (Level == nullptr) {
            Core::ErrorHandler::Fatal("World", "Persistent level creation failed");
        }

        Level->SetName("PersistentLevel");

        if (!mWorld->SetPersistentLevel(std::move(Level))) {
            Core::ErrorHandler::Fatal("World", "Persistent level initialization failed");
        }
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Engine::FWorldContext::~FWorldContext() = default;

    /**
     * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
     */
    Engine::UWorld* Engine::FWorldContext::GetWorld() const {
        return mWorld.get();
    }

    /**
     * @brief 엔진에 등록된 컨텍스트의 월드 핸들을 반환합니다.
     */
    Engine::FWorldHandle Engine::FWorldContext::GetHandle() const {
        return mHandle;
    }

    /**
     * @brief 엔진이 발급한 식별자를 컨텍스트와 월드에 함께 반영합니다.
     */
    void Engine::FWorldContext::SetHandle(Engine::FWorldHandle Handle) {
        mHandle = Handle;
        mWorld->mWorldHandle = Handle;
    }

    Engine::FEditorCameraState& Engine::FWorldContext::GetEditorCamera() {
        return mEditorCamera;
    }

    const Engine::FEditorCameraState& Engine::FWorldContext::GetEditorCamera() const {
        return mEditorCamera;
    }

}
