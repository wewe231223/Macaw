/**
 * @file FEditorContext.cpp
 * @brief UI와 독립적인 선택, 장면 상태와 공통 편집 연산을 구현합니다.
 * @ingroup Editor
 */

#include "Editor/FEditorContext.h"
#include "Engine/Engine.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/FWorldContext.h"
#include "Engine/AActor.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/SceneSerializer.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/FPropertyChangedEvent.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Core/Base/ErrorHandler.h"

#include <Windows.h>
#include <sstream>
#include <utility>

/** @brief 편집 로그에서 사용할 UTF-8 경로 표시를 제공합니다. */
namespace {
    /** @brief 파일시스템 경로를 로그용 UTF-8 문자열로 변환합니다. @param[in] Path 표시할 경로입니다. @return UTF-8 경로 문자열입니다. */
    FString PathText(const std::filesystem::path& Path) {
        const auto Text{Path.u8string()};
        return FString{reinterpret_cast<const char*>(Text.data()), Text.size()};
    }
}

/** @brief 범위 동안 재진입 제한 플래그를 설정합니다. */
FEditorContext::FOperationScope::FOperationScope(bool* Flag)
	: mFlag{Flag} {
    *mFlag = true;
}

/** @brief 범위가 끝나면 재진입 제한을 해제합니다. */
FEditorContext::FOperationScope::~FOperationScope() {
    *mFlag = false;
}

/** @brief 편집에 필요한 엔진의 비소유 연결을 구성합니다. */
FEditorContext::FEditorContext(UEngine* Engine)
	: mEngine{Engine} {
}

/** @brief 편집 상태와 콜백 저장소를 정리합니다. */
FEditorContext::~FEditorContext() = default;

/** @brief 명시적인 편집 월드 연결을 변경합니다. */
bool FEditorContext::SetWorld(FWorldHandle Handle) {
    if (mEngine == nullptr || mEngine->GetWorld(Handle) == nullptr || mChangingState || mEditingProperty || mEngine->IsDispatching()) {
        return false;
    }

    FOperationScope Scope{&mChangingState};
    mWorld = Handle;
    AssignSelection({}, {});
    mScenePath.clear();
    mDirty = false;
    return true;
}

/** @brief UI와 월드 사이의 비소유 편집 연결을 해제합니다. */
void FEditorContext::Reset() {
    if (mChangingState || mEditingProperty) {
        return;
    }

    FOperationScope Scope{&mChangingState};
    AssignSelection({}, {});
    mWorld = {};
    mScenePath.clear();
    mDirty = false;
    ClearLogs();
}

/** @brief 편집 대상 월드의 식별자를 반환합니다. */
FWorldHandle FEditorContext::GetWorldHandle() const {
    return mWorld;
}

/** @brief 엔진에서 편집 월드의 세대 핸들을 해석합니다. */
UWorld* FEditorContext::GetWorld() const {
    return mEngine != nullptr ? mEngine->GetWorld(mWorld) : nullptr;
}

/** @brief 정지한 편집 용도 월드인지 검사합니다. */
bool FEditorContext::CanEditWorld() const {
    const UWorld* World{GetWorld()};
    return World != nullptr && (World->GetWorldType() == EWorldType::Editor || World->GetWorldType() == EWorldType::EditorPreview) && World->GetPlayState() == EWorldPlayState::Stopped;
}

/** @brief 렌더러의 비소유 연결을 반환합니다. */
IRendererModule* FEditorContext::GetRenderer() const {
    return mEngine != nullptr ? mEngine->GetRenderer() : nullptr;
}

/** @brief 패널 순회 중 액터 수명 변경을 분리할 핸들 스냅샷을 만듭니다. */
TArray<FObjectHandle> FEditorContext::GetActorHandles() const {
    TArray<FObjectHandle> Handles{};
    const UWorld* World{GetWorld()};

    if (World != nullptr && World->GetPersistentLevel() != nullptr) {
        for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
            if (!Actor->IsPendingDestroy()) {
                Handles.push_back(Actor->GetHandle());
            }
        }
    }

    return Handles;
}

/** @brief 선택에 대응하는 프로퍼티 표시 대상의 핸들 스냅샷을 만듭니다. */
TArray<FObjectHandle> FEditorContext::GetDetailsObjects() const {
    TArray<FObjectHandle> Handles{};
    UObject* Object{GetSelectedObject()};

    if (Object == nullptr) {
        return Handles;
    }

    Handles.push_back(Object->GetHandle());

    if (const auto* Actor{Cast<AActor>(Object)}) {
        for (const auto& Component : Actor->GetComponents()) {
            Handles.push_back(Component->GetHandle());
        }
    }

    return Handles;
}

/** @brief 객체의 전역 식별과 실제 편집 월드 소속을 함께 검사합니다. */
UObject* FEditorContext::ResolveObject(FObjectHandle Handle) const {
    UWorld* World{GetWorld()};
    UObject* Object{FObjectRegistry::Get()->Resolve(Handle)};
    return World != nullptr && World->ContainsObject(Object) ? Object : nullptr;
}

/** @brief 유효한 액터를 선택하고 프로퍼티 편집 대상으로 지정합니다. */
bool FEditorContext::SelectActor(FObjectHandle Handle) {
    return Cast<AActor>(ResolveObject(Handle)) != nullptr && SetSelection(Handle, Handle);
}

/** @brief 객체 선택과 소유 액터 선택을 함께 갱신합니다. */
bool FEditorContext::SelectObject(FObjectHandle Handle) {
    UObject* Object{ResolveObject(Handle)};

    if (Object == nullptr) {
        return false;
    }

    AActor* Actor{Cast<AActor>(Object)};

    if (auto* Component{Cast<UActorComponent>(Object)}) {
        Actor = Component->GetOwner();
    }

    return SetSelection(Actor != nullptr ? Actor->GetHandle() : FObjectHandle{}, Handle);
}

/** @brief 액터와 객체 선택 핸들을 모두 무효화합니다. */
void FEditorContext::ClearSelection() {
    SetSelection({}, {});
}

/** @brief 요청 대상이 유효한지 확인하고 선택을 적용합니다. */
bool FEditorContext::SetSelection(FObjectHandle Actor, FObjectHandle Object) {
    if (mChangingState || mEditingProperty) {
        return false;
    }

    if (mSelectedActor == Actor && mSelectedObject == Object) {
        return true;
    }

    FOperationScope Scope{&mChangingState};

    if ((Actor.IsValid() && ResolveObject(Actor) == nullptr) || (Object.IsValid() && ResolveObject(Object) == nullptr)) {
        return false;
    }

    AssignSelection(Actor, Object);
    return true;
}

/** @brief 검증한 선택 상태를 저장합니다. */
void FEditorContext::AssignSelection(FObjectHandle Actor, FObjectHandle Object) {
    mSelectedActor = Actor;
    mSelectedObject = Object;
}

/** @brief 선택한 액터를 월드 소속과 함께 검증합니다. */
AActor* FEditorContext::GetSelectedActor() const {
    return Cast<AActor>(ResolveObject(mSelectedActor));
}

/** @brief 프로퍼티 편집 대상을 월드 소속과 함께 검증합니다. */
UObject* FEditorContext::GetSelectedObject() const {
    return ResolveObject(mSelectedObject);
}

/** @brief 지정한 기본 메시 종류의 액터를 생성하여 영속 레벨에 추가합니다. */
AStaticMeshActor* FEditorContext::SpawnPrimitive(EPrimitiveType Type) {
    if (!CanEditWorld() || mChangingState || mEditingProperty || mEngine->IsDispatching()) {
        return nullptr;
    }

    FOperationScope Scope{&mChangingState};
    const FTransform Placement{mPlacementProvider ? mPlacementProvider() : FTransform{}};
    UWorld* World{GetWorld()};

    if (World == nullptr) {
        return nullptr;
    }

    AStaticMeshActor* Actor{World->SpawnPrimitive(Type)};

    if (Actor != nullptr) {
        if (!Actor->SetActorTransform(Placement)) {
            World->DestroyActor(Actor->GetHandle());
            return nullptr;
        }

        AssignSelection(Actor->GetHandle(), Actor->GetHandle());
        mDirty = true;
        AddLog("Spawned " + Actor->GetName());
    }

    return Actor;
}

/** @brief 현재 선택한 액터를 제거하고 관련 선택 상태를 초기화합니다. */
bool FEditorContext::DestroySelectedActor() {
    AActor* Actor{GetSelectedActor()};

    if (Actor == nullptr || !CanEditWorld() || mChangingState || mEditingProperty || mEngine->IsDispatching()) {
        return false;
    }

    const FObjectHandle Handle{Actor->GetHandle()};
    const FString Name{Actor->GetName()};
    FOperationScope Scope{&mChangingState};
    UWorld* World{GetWorld()};

    if (World == nullptr || ResolveObject(Handle) == nullptr || !World->DestroyActor(Handle)) {
        return false;
    }

    AssignSelection({}, {});
    mDirty = true;
    AddLog("Deleted " + Name);
    return true;
}

/** @brief 편집 월드, 객체, 프로퍼티 소속과 편집 권한을 함께 검사합니다. */
bool FEditorContext::CanEditProperty(FObjectHandle Handle, const FProperty* Property) const {
    UObject* Object{ResolveObject(Handle)};
    return !mEditingProperty && CanEditWorld() && Object != nullptr && Property != nullptr && Object->GetClass()->FindClassProperty(Property->GetName()) == Property && Property->HasFlag(EPropertyFlags::Edit) && !Property->HasFlag(EPropertyFlags::ReadOnly);
}

/** @brief 프로퍼티 소속과 편집 권한을 확인하고 변경 전후 알림과 함께 값을 씁니다. */
bool FEditorContext::EditProperty(FObjectHandle Handle, const FProperty* Property, const FPropertyValue& Value, bool Interactive) {
    if (!CanEditProperty(Handle, Property)) {
        return false;
    }

    FOperationScope Editing{&mEditingProperty};
    UWorld* World{GetWorld()};
    UWorld::FDispatchScope Scope{World};
    UObject* Object{ResolveObject(Handle)};
    Object->PreEditChange(Property);

    if (ResolveObject(Handle) != Object) {
        return false;
    }

    if (!Property->WriteValue(Object, Value)) {
        AddLog("Rejected invalid property value: " + Property->GetName());
        return false;
    }

    Object->PostEditChangeProperty(FPropertyChangedEvent{Property, Interactive});
    mDirty = true;
    return true;
}

/** @brief 액터 루트의 변환 프로퍼티를 에디터 편집 경로로 변경합니다. */
bool FEditorContext::ApplyTransform(FObjectHandle Handle, const FTransform& Transform, bool Interactive) {
    AActor* Actor{Cast<AActor>(ResolveObject(Handle))};
    USceneComponent* Root{Actor != nullptr ? Actor->GetRootComponent() : nullptr};
    const FProperty* Property{Root != nullptr ? Root->GetClass()->FindClassProperty("Transform") : nullptr};
    return Property != nullptr && EditProperty(Root->GetHandle(), Property, FPropertyValue{Transform}, Interactive);
}

/** @brief 편집 월드를 지정한 장면 파일로 저장하고 변경 표시를 해제합니다. */
bool FEditorContext::SaveScene(const std::filesystem::path& Path) {
    if (!CanEditWorld() || Path.empty() || mChangingState || mEditingProperty || mEngine->IsDispatching()) {
        return false;
    }

    FOperationScope Scope{&mChangingState};
    const std::filesystem::path ScenePath{std::filesystem::absolute(Path.is_absolute() ? Path : std::filesystem::path{"Scenes"} / Path)};
    FString Error{};

    if (!FSceneSerializer::Save(ScenePath, mEngine->GetWorldContext(mWorld), Error)) {
        ErrorHandler::Fatal("Scene save", Error);
    }

    mScenePath = ScenePath;
    mDirty = false;
    AddLog("Saved " + PathText(ScenePath));
    return true;
}

/** @brief 새 컨텍스트를 엔진에 등록하고 기존 편집 월드만 교체합니다. */
bool FEditorContext::ReplaceScene(std::unique_ptr<FWorldContext> Context, const std::filesystem::path& Path) {
    const FWorldHandle Previous{mWorld};
    const FWorldHandle Current{mEngine->GetWorldHandle()};
    const FWorldHandle Replacement{mEngine->RegisterWorld(std::move(Context))};

    if (!Replacement.IsValid()) {
        return false;
    }

    if (Previous.IsValid() && mEngine->GetWorld(Previous) != nullptr && !mEngine->DestroyWorld(Previous)) {
        mEngine->DestroyWorld(Replacement);
        return false;
    }

    if (Current == Previous || !Current.IsValid()) {
        mEngine->SetCurrentWorld(Replacement);
    }

    mWorld = Replacement;
    AssignSelection({}, {});
    mScenePath = Path;
    mDirty = false;
    return true;
}

/** @brief 장면 파일을 새 컨텍스트에 읽고 편집 대상 월드만 교체합니다. */
bool FEditorContext::LoadScene(const std::filesystem::path& Path) {
    if (!CanEditWorld() || Path.empty() || mChangingState || mEditingProperty || mEngine->IsDispatching()) {
        return false;
    }

    FOperationScope Scope{&mChangingState};
    const std::filesystem::path ScenePath{std::filesystem::absolute(Path.is_absolute() ? Path : std::filesystem::path{"Scenes"} / Path)};
    FString Error{};
    auto Context{FSceneSerializer::Load(ScenePath, Error)};

    if (Context == nullptr) {
        ErrorHandler::Fatal("Scene load", Error);
    }

    if (!ReplaceScene(std::move(Context), ScenePath)) {
        return false;
    }

    AddLog("Loaded " + PathText(ScenePath));
    return true;
}

/** @brief 빈 편집 월드로 교체하고 선택 및 장면 파일 상태를 초기화합니다. */
bool FEditorContext::NewScene() {
    if (!CanEditWorld() || mChangingState || mEditingProperty || mEngine->IsDispatching()) {
        return false;
    }

    FOperationScope Scope{&mChangingState};
    return ReplaceScene(std::make_unique<FWorldContext>(), {});
}

/** @brief 현재 장면 파일 경로를 반환합니다. */
const std::filesystem::path& FEditorContext::GetScenePath() const {
    return mScenePath;
}

/** @brief 저장 후 변경되어 저장이 필요한 장면인지 검사합니다. */
bool FEditorContext::IsSceneDirty() const {
    return mDirty;
}

/** @brief 콘솔 로그 모델에 메시지를 추가합니다. */
void FEditorContext::AddLog(const FString& Message) {
    if (mLogs.size() >= 500) {
        mLogs.erase(mLogs.begin(), mLogs.begin() + 100);
    }

    mLogs.push_back(Message);
    ++mLogRevision;
    OutputDebugStringA((Message + "\n").c_str());
}

/** @brief 수집한 콘솔 로그를 모두 지웁니다. */
void FEditorContext::ClearLogs() {
    mLogs.clear();
    ++mLogRevision;
}

/** @brief UI가 표시할 로그 목록을 반환합니다. */
const TArray<FString>& FEditorContext::GetLogs() const {
    return mLogs;
}

/** @brief 로그의 변경 세대를 반환합니다. */
Uint64 FEditorContext::GetLogRevision() const {
    return mLogRevision;
}

/** @brief 입력한 콘솔 명령을 해석하여 공통 편집 동작을 실행합니다. */
void FEditorContext::ExecuteConsoleCommand(const FString& Command) {
    AddLog("> " + Command);
    std::istringstream Input{Command};
    FString Verb{};
    Input >> Verb;

    if (Verb == "clear") {
        ClearLogs();
    } else if (Verb == "help") {
        AddLog("help | clear | spawn plane/cube/sphere | save <path.json> | load <path.json>");
    } else if (Verb == "spawn") {
        FString Type{};
        Input >> Type;

        if (Type == "plane" || Type == "cube" || Type == "sphere") {
            SpawnPrimitive(Type == "plane" ? EPrimitiveType::Plane : Type == "sphere" ? EPrimitiveType::Sphere : EPrimitiveType::Cube);
        } else {
            AddLog("Expected plane, cube or sphere.");
        }
    } else if (Verb == "save" || Verb == "load") {
        FString Text{};
        std::getline(Input >> std::ws, Text);
        const std::filesystem::path Path{Text.empty() ? mScenePath : std::filesystem::path{std::u8string_view{reinterpret_cast<const char8_t*>(Text.data()), Text.size()}}};

        if (Path.empty()) {
            AddLog("Specify a scene path.");
        } else if (Verb == "save") {
            SaveScene(Path);
        } else {
            LoadScene(Path);
        }
    } else if (!Verb.empty()) {
        AddLog("Unknown command.");
    }
}

/** @brief 새 액터 배치 변환 제공자를 저장합니다. */
void FEditorContext::SetPlacementProvider(std::function<FTransform()> Provider) {
    mPlacementProvider = std::move(Provider);
}
