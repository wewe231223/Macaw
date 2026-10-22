/**
 * @file FWorldContext.cpp
 * @brief 하나의 월드와 그 객체 레지스트리를 함께 소유하는 수명 경계입니다.
 * @ingroup Engine
 */

#include "Engine/FWorldContext.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"
#include "CoreUObject/FObjectRegistry.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/Reflection/RegisterEngine.h"
#include "CoreUObject/Reflection/NewObject.h"

#include <utility>

/**
 * @brief FWorldContext의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FWorldContext::FWorldContext() {
    ReflectionModules::RegisterEngine();

    mWorld = NewObject<UWorld>(mRegistry);

    if (mWorld == nullptr) {
        ErrorHandler::Fatal("World", "World creation failed");
    }

    mWorld->SetName("EditorWorld");

    auto Level{NewObject<ULevel>(mRegistry)};

    if (Level == nullptr) {
        ErrorHandler::Fatal("World", "Persistent level creation failed");
    }

    Level->SetName("PersistentLevel");

    if (!mWorld->SetPersistentLevel(std::move(Level))) {
        ErrorHandler::Fatal("World", "Persistent level initialization failed");
    }
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FWorldContext::~FWorldContext() = default;

/**
 * @brief 객체를 등록한 레지스트리를 반환합니다.
 */
FObjectRegistry& FWorldContext::GetRegistry() {
    return mRegistry;
}

/**
 * @brief 객체를 등록한 레지스트리를 반환합니다.
 */
const FObjectRegistry& FWorldContext::GetRegistry() const {
    return mRegistry;
}

/**
 * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
 */
UWorld* FWorldContext::GetWorld() const {
    return mWorld.get();
}
