/**
 * @file FEditorContext.cpp
 * @brief UI와 독립적인 선택, 장면 상태와 공통 편집 연산을 구현합니다.
 * @ingroup Editor
 */

#include "Editor/FEditorContext.h"
#include "Core/FilePath.h"
#include "Engine/FProject.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/Component/UBillboardTextComponent.h"
#include "Engine/Component/UBillboardComponent.h"
#include "Engine/Engine.h"
#include "RenderCore/IRendererModule.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/FWorldContext.h"
#include "Engine/AActor.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/ACameraActor.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/SceneSerializer.h"
#include "Engine/FWorldCloner.h"
#include <algorithm>
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/FPropertyChangedEvent.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Core/Base/ErrorHandler.h"

#include <Windows.h>
#include <cctype>
#include <sstream>
#include <utility>

namespace Editor {

    /**
     * @brief 범위 동안 재진입 제한 플래그를 설정합니다.
     */
    Editor::FEditorContext::FOperationScope::FOperationScope(bool* Flag)
		: mFlag{Flag} {
        *mFlag = true;
    }

    /**
     * @brief 범위가 끝나면 재진입 제한을 해제합니다.
     */
    Editor::FEditorContext::FOperationScope::~FOperationScope() {
        *mFlag = false;
    }

    /**
     * @brief 편집에 필요한 엔진의 비소유 연결을 구성합니다.
     */
    Editor::FEditorContext::FEditorContext(Engine::UEngine* Engine)
		: mEngine{Engine} {
    }

    /**
     * @brief 편집 상태와 콜백 저장소를 정리합니다.
     */
    Editor::FEditorContext::~FEditorContext() = default;

    /**
     * @brief 명시적인 편집 월드 연결을 변경합니다.
     */
    bool Editor::FEditorContext::SetWorld(Engine::FWorldHandle Handle) {
        if (mPlayLocked || mEngine == nullptr || mEngine->GetWorld(Handle) == nullptr || mChangingState || mEditingProperty || mEngine->IsDispatching()) {
            return false;
        }

        FOperationScope Scope{&mChangingState};

        mWorld = Handle;
        AssignSelection({}, {});
        mScenePath.clear();
        mDirty = false;

        return true;
    }

    /**
     * @brief UI와 월드 사이의 비소유 편집 연결을 해제합니다.
     */
    void Editor::FEditorContext::Reset() {
        if (mPlayLocked || mChangingState || mEditingProperty) {
            return;
        }

        FOperationScope Scope{&mChangingState};

        AssignSelection({}, {});
        mWorld = {};
        mScenePath.clear();
        mDirty = false;
        ClearLogs();
    }

    /**
     * @brief 편집 대상 월드의 식별자를 반환합니다.
     */
    Engine::FWorldHandle Editor::FEditorContext::GetWorldHandle() const {
        return mWorld;
    }

    /**
     * @brief 엔진에서 편집 월드의 세대 핸들을 해석합니다.
     */
    Engine::UWorld* Editor::FEditorContext::GetWorld() const {
        return mEngine != nullptr ? mEngine->GetWorld(mWorld) : nullptr;
    }

    /**
     * @brief 정지한 편집 용도 월드인지 검사합니다.
     */
    bool Editor::FEditorContext::CanEditWorld() const {
        const Engine::UWorld* World{GetWorld()};
        return !mPlayLocked && World != nullptr && (World->GetWorldType() == Engine::EWorldType::Editor || World->GetWorldType() == Engine::EWorldType::EditorPreview) && World->GetPlayState() == Engine::EWorldPlayState::Stopped;
    }

    bool Editor::FEditorContext::IsChangingState() const {
        return mChangingState || mEditingProperty;
    }

    /**
     * @brief 렌더러의 비소유 연결을 반환합니다.
     */
    RenderCore::IRendererModule* Editor::FEditorContext::GetRenderer() const {
        return mEngine != nullptr ? mEngine->GetRenderer() : nullptr;
    }

    /**
     * @brief 패널 순회 중 액터 수명 변경을 분리할 핸들 스냅샷을 만듭니다.
     */
    Core::TArray<CoreUObject::FObjectHandle> Editor::FEditorContext::GetActorHandles() const {
        Core::TArray<CoreUObject::FObjectHandle> Handles{};
        const Engine::UWorld* World{GetWorld()};

        if (World != nullptr && World->GetPersistentLevel() != nullptr) {
            for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
                if (!Actor->IsPendingDestroy()) {
                    Handles.push_back(Actor->GetHandle());
                }
            }
        }

        return Handles;
    }

    /**
     * @brief 선택에 대응하는 프로퍼티 표시 대상의 핸들 스냅샷을 만듭니다.
     */
    Core::TArray<CoreUObject::FObjectHandle> Editor::FEditorContext::GetDetailsObjects() const {
        Core::TArray<CoreUObject::FObjectHandle> Handles{};
        CoreUObject::UObject* Object{GetSelectedObject()};

        if (Object == nullptr) {
            return Handles;
        }

        Handles.push_back(Object->GetHandle());

        if (const auto* Actor{CoreUObject::Cast<Engine::AActor>(Object)}) {
            for (const auto& Component : Actor->GetComponents()) {
                Handles.push_back(Component->GetHandle());
            }
        }

        return Handles;
    }

    /**
     * @brief 객체의 전역 식별과 실제 편집 월드 소속을 함께 검사합니다.
     */
    CoreUObject::UObject* Editor::FEditorContext::ResolveObject(CoreUObject::FObjectHandle Handle) const {
        Engine::UWorld* World{GetWorld()};
        CoreUObject::UObject* Object{CoreUObject::FObjectRegistry::Get()->Resolve(Handle)};
        return World != nullptr && World->ContainsObject(Object) ? Object : nullptr;
    }

    /**
     * @brief 유효한 액터를 선택하고 프로퍼티 편집 대상으로 지정합니다.
     */
    bool Editor::FEditorContext::SelectActor(CoreUObject::FObjectHandle Handle) {
        return CoreUObject::Cast<Engine::AActor>(ResolveObject(Handle)) != nullptr && SetSelection({Handle}, Handle);
    }

    /**
     * @brief 객체 선택과 소유 액터 선택을 함께 갱신합니다.
     */
    bool Editor::FEditorContext::SelectObject(CoreUObject::FObjectHandle Handle) {
        CoreUObject::UObject* Object{ResolveObject(Handle)};

        if (Object == nullptr) {
            return false;
        }

        Engine::AActor* Actor{CoreUObject::Cast<Engine::AActor>(Object)};

        if (auto* Component{CoreUObject::Cast<Engine::UActorComponent>(Object)}) {
            Actor = Component->GetOwner();
        }

        return SetSelection(Actor != nullptr ? Core::TArray<CoreUObject::FObjectHandle>{Actor->GetHandle()} : Core::TArray<CoreUObject::FObjectHandle>{}, Handle);
    }

    /**
     * @brief 액터와 객체 선택 핸들을 모두 무효화합니다.
     */
    void Editor::FEditorContext::ClearSelection() {
        SetSelection({}, {});
    }

    /**
     * @brief 요청 대상이 유효한지 확인하고 선택을 적용합니다.
     */
    bool Editor::FEditorContext::SetSelection(const Core::TArray<CoreUObject::FObjectHandle>& Actors, CoreUObject::FObjectHandle Object) {
        if (mPlayLocked || mChangingState || mEditingProperty) {
            return false;
        }

        FOperationScope Scope{&mChangingState};
        Core::TArray<CoreUObject::FObjectHandle> Valid{};

        for (CoreUObject::FObjectHandle Handle : Actors) {
            const auto* Actor{CoreUObject::Cast<Engine::AActor>(ResolveObject(Handle))};

            if (Actor == nullptr || Actor->IsPendingDestroy() || std::find(Valid.begin(), Valid.end(), Handle) != Valid.end()) {
                return false;
            }

            Valid.push_back(Handle);
        }

        if (Object.IsValid() && ResolveObject(Object) == nullptr) {
            return false;
        }

        if (mSelectedActors == Valid && mSelectedObject == Object) {
            return true;
        }

        AssignSelection(Valid, Object);

        return true;
    }

    /**
     * @brief 검증한 선택 상태를 저장합니다.
     */
    void Editor::FEditorContext::AssignSelection(const Core::TArray<CoreUObject::FObjectHandle>& Actors, CoreUObject::FObjectHandle Object) {
        mSelectedActors = Actors;
        mSelectedObject = Object;
        ++mSelectionRevision;
    }

    /**
     * @brief 선택한 액터를 월드 소속과 함께 검증합니다.
     */
    Engine::AActor* Editor::FEditorContext::GetSelectedActor() const {
        const Core::TArray<CoreUObject::FObjectHandle> Actors{GetSelectedActors()};
        return Actors.empty() ? nullptr : CoreUObject::Cast<Engine::AActor>(ResolveObject(Actors.back()));
    }

    /**
     * @brief 프로퍼티 편집 대상을 월드 소속과 함께 검증합니다.
     */
    CoreUObject::UObject* Editor::FEditorContext::GetSelectedObject() const {
        return ResolveObject(mSelectedObject);
    }

    /**
     * @brief 지정한 기본 메시 종류의 액터를 생성하여 영속 레벨에 추가합니다.
     */
    Engine::AStaticMeshActor* Editor::FEditorContext::SpawnPrimitive(Engine::EPrimitiveType Type) {
        if (!CanSpawnActor()) {
            return nullptr;
        }

        FOperationScope Scope{&mChangingState};
        const Core::FTransform Placement{mPlacementProvider ? mPlacementProvider() : Core::FTransform{}};
        Engine::UWorld* World{GetWorld()};

        if (World == nullptr) {
            return nullptr;
        }

        Engine::AStaticMeshActor* Actor{World->SpawnPrimitive(Type)};

        if (!ApplySpawnPlacement(Actor, Placement)) {
            return nullptr;
        }

        CompleteActorSpawn(Actor, "Spawned ");

        return Actor;
    }

    Engine::ACameraActor* Editor::FEditorContext::SpawnCamera() {
        if (!CanSpawnActor()) {
            return nullptr;
        }

        FOperationScope Scope{&mChangingState};
        const Core::FTransform Placement{mPlacementProvider ? mPlacementProvider() : Core::FTransform{}};
        Engine::UWorld* World{GetWorld()};
        auto* Actor{World != nullptr ? CoreUObject::Cast<Engine::ACameraActor>(World->SpawnActor(Engine::ACameraActor::StaticClass())) : nullptr};

        if (!ApplySpawnPlacement(Actor, Placement)) {
            return nullptr;
        }

        if (World->GetActiveCamera() == nullptr) {
            World->SetActiveCamera(Actor->GetCameraComponent());

            if (World->GetInputTarget() == nullptr) {
                World->SetInputTarget(Actor);
            }
        }

        CompleteActorSpawn(Actor, "Spawned ");

        return Actor;
    }

    Engine::AActor* Editor::FEditorContext::SpawnText() {
        if (!CanSpawnActor()) {
            return nullptr;
        }

        FOperationScope Scope{&mChangingState};
        Engine::UWorld* World{GetWorld()};
        Engine::AActor* Actor{World != nullptr ? World->SpawnActor(Engine::AActor::StaticClass()) : nullptr};

        if (Actor == nullptr) {
            return nullptr;
        }

        auto Component{CoreUObject::NewObject<Engine::UBillboardTextComponent>()};
        Component->SetName("BillboardTextComponent");

        if (!Actor->AddComponent(std::move(Component))) {
            World->DestroyActor(Actor->GetHandle());
            return nullptr;
        }

        Actor->SetName("Text");

        const Core::FTransform Placement{mPlacementProvider ? mPlacementProvider() : Core::FTransform{}};

        if (!ApplySpawnPlacement(Actor, Placement)) {
            return nullptr;
        }

        CompleteActorSpawn(Actor, "Spawned ");

        return Actor;
    }

    Engine::AActor* Editor::FEditorContext::SpawnBillboard(const Core::FString& SpritePath) {
        if (!CanSpawnActor()) {
            return nullptr;
        }

        FOperationScope Scope{&mChangingState};
        Engine::UWorld* World{GetWorld()};
        Engine::AActor* Actor{World != nullptr ? World->SpawnActor(Engine::AActor::StaticClass()) : nullptr};

        if (Actor == nullptr) {
            return nullptr;
        }

        auto Component{CoreUObject::NewObject<Engine::UBillboardComponent>()};
        Component->SetName("BillboardComponent");

        Engine::UBillboardComponent* Billboard{Component.get()};

        if (!Actor->AddComponent(std::move(Component)) || !Billboard->SetTexturePath(SpritePath)) {
            World->DestroyActor(Actor->GetHandle());
            return nullptr;
        }

        Actor->SetName(SpritePath.empty() ? "Billboard" : Core::FilePath::ToUtf8(Core::FilePath::FromUtf8(SpritePath).stem()));

        const Core::FTransform Placement{mPlacementProvider ? mPlacementProvider() : Core::FTransform{}};

        if (!ApplySpawnPlacement(Actor, Placement)) {
            return nullptr;
        }

        CompleteActorSpawn(Actor, "Spawned ");

        return Actor;
    }

    bool Editor::FEditorContext::CanSpawnActor() const {
        return CanEditWorld() && !mChangingState && !mEditingProperty && !mEngine->IsDispatching();
    }

    bool Editor::FEditorContext::ApplySpawnPlacement(Engine::AActor* Actor, const Core::FTransform& Placement) {
        if (Actor == nullptr) {
            return false;
        }

        if (!Actor->SetActorTransform(Placement)) {
            GetWorld()->DestroyActor(Actor->GetHandle());
            return false;
        }

        return true;
    }

    void Editor::FEditorContext::CompleteActorSpawn(Engine::AActor* Actor, const char* Action) {
        AssignSelection({Actor->GetHandle()}, Actor->GetHandle());
        mDirty = true;
        AddLog(Action + Actor->GetName());
    }

    bool Editor::FEditorContext::CanAddComponent(const CoreUObject::UClass* Class) const {
        return CanEditWorld() && !mChangingState && !mEditingProperty && !mEngine->IsDispatching() && GetSelectedActors().size() == 1 && Class != nullptr && Class != Engine::UActorComponent::StaticClass() && Class->IsChildOf(Engine::UActorComponent::StaticClass()) && Class->CanCreate();
    }

    Engine::UActorComponent* Editor::FEditorContext::AddComponent(const CoreUObject::UClass* Class) {
        if (!CanAddComponent(Class)) {
            return nullptr;
        }

        FOperationScope Scope{&mChangingState};
        Engine::AActor* Actor{GetSelectedActor()};
        const CoreUObject::FObjectHandle ActorHandle{Actor->GetHandle()};
        auto Object{Class->CreateInstance()};

        if (Object == nullptr || Object->GetClass() != Class) {
            return nullptr;
        }

        std::unique_ptr<Engine::UActorComponent> Component{static_cast<Engine::UActorComponent*>(Object.release())};
        Core::FString BaseName{Class->GetName()};

        if (BaseName.starts_with("U")) {
            BaseName.erase(0, 1);
        }

        Core::FString Name{BaseName};
        Core::Uint32 Suffix{2};

        while (CoreUObject::FObjectRegistry::Get()->FindObject(Core::FName{Name}, Actor) != nullptr) {
            Name = BaseName + "_" + std::to_string(Suffix++);
        }

        Component->SetName(Name);

        if (auto* SceneComponent{CoreUObject::Cast<Engine::USceneComponent>(Component.get())}; SceneComponent != nullptr && Actor->GetRootComponent() != nullptr) {
            if (!SceneComponent->SetTransform(Actor->GetRootComponent()->GetTransform())) {
                return nullptr;
            }
        }

        const CoreUObject::FObjectHandle ComponentHandle{Component->GetHandle()};

        if (!Actor->AddComponent(std::move(Component))) {
            return nullptr;
        }

        auto* Added{CoreUObject::Cast<Engine::UActorComponent>(ResolveObject(ComponentHandle))};

        if (Added == nullptr) {
            return nullptr;
        }

        AssignSelection({ActorHandle}, ComponentHandle);
        mDirty = true;
        AddLog("Added " + Name);

        return Added;
    }

    Engine::UCameraComponent* Editor::FEditorContext::GetSelectedCamera() const {
        if (auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(GetSelectedObject())}) {
            return Camera;
        }

        if (const Engine::AActor* Actor{GetSelectedActor()}) {
            for (const auto& Component : Actor->GetComponents()) {
                if (auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(Component.get())}) {
                    return Camera;
                }
            }
        }

        return nullptr;
    }

    bool Editor::FEditorContext::UseSelectedCamera() {
        Engine::UCameraComponent* Camera{GetSelectedCamera()};

        if (!CanEditWorld() || mChangingState || mEditingProperty || mEngine->IsDispatching() || Camera == nullptr) {
            return false;
        }

        if (!GetWorld()->SetActiveCamera(Camera) || !GetWorld()->SetInputTarget(Camera->GetOwner())) {
            return false;
        }

        mDirty = true;

        return true;
    }

    /**
     * @brief 현재 선택한 액터를 제거하고 관련 선택 상태를 초기화합니다.
     */
    bool Editor::FEditorContext::DestroySelectedActor() {
        if (!CanEditWorld() || mChangingState || mEditingProperty || mEngine->IsDispatching()) {
            return false;
        }

        const Core::TArray<CoreUObject::FObjectHandle> Actors{GetSelectedActors()};

        if (Actors.empty()) {
            return false;
        }

        FOperationScope Scope{&mChangingState};
        Engine::UWorld* World{GetWorld()};
        bool Removed{};

        for (CoreUObject::FObjectHandle Handle : Actors) {
            Removed = World->DestroyActor(Handle) || Removed;
        }

        if (Removed) {
            AssignSelection({}, {});
            mDirty = true;
            AddLog("Deleted selected actors");
        }

        return Removed;
    }

    /**
     * @brief 편집 월드, 객체, 프로퍼티 소속과 편집 권한을 함께 검사합니다.
     */
    bool Editor::FEditorContext::CanEditProperty(CoreUObject::FObjectHandle Handle, const CoreUObject::FProperty* Property) const {
        CoreUObject::UObject* Object{ResolveObject(Handle)};
        return !mEditingProperty && CanEditWorld() && Object != nullptr && Property != nullptr && Object->GetClass()->FindClassProperty(Property->GetName()) == Property && Property->HasFlag(Core::EPropertyFlags::Edit) && !Property->HasFlag(Core::EPropertyFlags::ReadOnly);
    }

    /**
     * @brief 프로퍼티 소속과 편집 권한을 확인하고 변경 전후 알림과 함께 값을 씁니다.
     */
    bool Editor::FEditorContext::EditProperty(CoreUObject::FObjectHandle Handle, const CoreUObject::FProperty* Property, const CoreUObject::FPropertyValue& Value, bool Interactive) {
        if (!CanEditProperty(Handle, Property)) {
            return false;
        }

        FOperationScope Editing{&mEditingProperty};
        Engine::UWorld* World{GetWorld()};
        Engine::UWorld::FDispatchScope Scope{World};
        CoreUObject::UObject* Object{ResolveObject(Handle)};
        Object->PreEditChange(Property);

        if (ResolveObject(Handle) != Object) {
            return false;
        }

        if (!Property->WriteValue(Object, Value)) {
            AddLog("Rejected invalid property value: " + Property->GetName());
            return false;
        }

        Object->PostEditChangeProperty(CoreUObject::FPropertyChangedEvent{Property, Interactive});
        mDirty = true;

        return true;
    }

    /**
     * @brief 액터 루트의 변환 프로퍼티를 에디터 편집 경로로 변경합니다.
     */
    bool Editor::FEditorContext::ApplyTransform(CoreUObject::FObjectHandle Handle, const Core::FTransform& Transform, bool Interactive) {
        CoreUObject::UObject* Object{ResolveObject(Handle)};
        Engine::AActor* Actor{CoreUObject::Cast<Engine::AActor>(Object)};
        Engine::USceneComponent* Root{Actor != nullptr ? Actor->GetRootComponent() : CoreUObject::Cast<Engine::USceneComponent>(Object)};
        const CoreUObject::FProperty* Property{Root != nullptr ? Root->GetClass()->FindClassProperty("Transform") : nullptr};

        return Property != nullptr && EditProperty(Root->GetHandle(), Property, CoreUObject::FPropertyValue{Transform}, Interactive);
    }

    /**
     * @brief 편집 월드를 지정한 장면 파일로 저장하고 변경 표시를 해제합니다.
     */
    bool Editor::FEditorContext::SaveScene(const std::filesystem::path& Path) {
        if (!CanEditWorld() || Path.empty() || mChangingState || mEditingProperty || mEngine->IsDispatching()) {
            return false;
        }

        FOperationScope Scope{&mChangingState};
        Core::FString Error{};
        Engine::FProject* Project{GetProject()};
        const std::filesystem::path ScenePath{Project != nullptr ? Project->GetAssets()->ResolvePath(Path, Error) : std::filesystem::path{}};

        if (ScenePath.empty()) {
            AddLog(Error.empty() ? "Open a project before saving a scene" : Error);
            return false;
        }

        if (mCameraProvider) {
            mEngine->GetWorldContext(mWorld)->GetEditorCamera() = mCameraProvider();
        }

        if (!Engine::FSceneSerializer::Save(ScenePath, mEngine->GetWorldContext(mWorld), Error)) {
            AddLog(Error);
            return false;
        }

        mScenePath = ScenePath;
        mDirty = false;
        AddLog("Saved " + Core::FilePath::ToUtf8(ScenePath));

        return true;
    }

    /**
     * @brief 새 컨텍스트를 엔진에 등록하고 기존 편집 월드만 교체합니다.
     */
    bool Editor::FEditorContext::ReplaceScene(std::unique_ptr<Engine::FWorldContext> Context, const std::filesystem::path& Path) {
        const Engine::FWorldHandle Previous{mWorld};
        const Engine::FWorldHandle Current{mEngine->GetWorldHandle()};
        const Engine::FWorldHandle Replacement{mEngine->RegisterWorld(std::move(Context))};

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

        if (mCameraRestorer) {
            mCameraRestorer(mEngine->GetWorldContext(mWorld)->GetEditorCamera());
        }

        return true;
    }

    void Editor::FEditorContext::SetCameraHandlers(std::function<Engine::FEditorCameraState()> Provider, std::function<void(const Engine::FEditorCameraState&)> Restorer) {
        mCameraProvider = std::move(Provider);
        mCameraRestorer = std::move(Restorer);
    }

    bool Editor::FEditorContext::SetMaterialOverride(CoreUObject::FObjectHandle Handle, Core::Uint32 Slot, const Core::FString& Path) {
        if (!CanEditWorld() || IsChangingState() || mEngine->IsDispatching()) {
            return false;
        }

        auto* Component{CoreUObject::Cast<Engine::UStaticMeshComponent>(ResolveObject(Handle))};

        if (Component == nullptr || !Component->SetSlotMaterialPath(Slot, Path)) {
            return false;
        }

        mDirty = true;

        return true;
    }

    /**
     * @brief 장면 파일을 새 컨텍스트에 읽고 편집 대상 월드만 교체합니다.
     */
    bool Editor::FEditorContext::LoadScene(const std::filesystem::path& Path) {
        if (!CanEditWorld() || Path.empty() || mChangingState || mEditingProperty || mEngine->IsDispatching()) {
            return false;
        }

        FOperationScope Scope{&mChangingState};
        Core::FString Error{};
        Engine::FProject* Project{GetProject()};
        const std::filesystem::path ScenePath{Project != nullptr ? Project->GetAssets()->ResolvePath(Path, Error) : std::filesystem::path{}};

        if (ScenePath.empty()) {
            AddLog(Error.empty() ? "Open a project before loading a scene" : Error);
            return false;
        }

        auto Context{Engine::FSceneSerializer::Load(ScenePath, Error, Project->GetAssets())};

        if (Context == nullptr) {
            AddLog(Error);
            return false;
        }

        if (!ReplaceScene(std::move(Context), ScenePath)) {
            return false;
        }

        AddLog("Loaded " + Core::FilePath::ToUtf8(ScenePath));

        return true;
    }

    /**
     * @brief 빈 편집 월드로 교체하고 선택 및 장면 파일 상태를 초기화합니다.
     */
    bool Editor::FEditorContext::NewScene() {
        if (!CanEditWorld() || mChangingState || mEditingProperty || mEngine->IsDispatching()) {
            return false;
        }

        FOperationScope Scope{&mChangingState};

        return ReplaceScene(std::make_unique<Engine::FWorldContext>(), {});
    }

    /**
     * @brief 현재 장면 파일 경로를 반환합니다.
     */
    const std::filesystem::path& Editor::FEditorContext::GetScenePath() const {
        return mScenePath;
    }

    /**
     * @brief 저장 후 변경되어 저장이 필요한 장면인지 검사합니다.
     */
    bool Editor::FEditorContext::IsSceneDirty() const {
        const Engine::FWorldContext* Context{mEngine != nullptr ? mEngine->GetWorldContext(mWorld) : nullptr};
        return mDirty || (Context != nullptr && mCameraProvider && Context->GetEditorCamera() != mCameraProvider());
    }

    Engine::FProject* Editor::FEditorContext::GetProject() const {
        return mEngine != nullptr ? mEngine->GetProject() : nullptr;
    }

    Engine::AStaticMeshActor* Editor::FEditorContext::SpawnMesh(const std::filesystem::path& Path) {
        return SpawnMesh(Path, mPlacementProvider ? mPlacementProvider() : Core::FTransform{});
    }

    Engine::AStaticMeshActor* Editor::FEditorContext::SpawnMesh(const std::filesystem::path& Path, const Core::FTransform& Placement) {
        if (!CanSpawnActor() || !Placement.IsValid()) {
            return nullptr;
        }

        FOperationScope Scope{&mChangingState};
        Engine::FProject* Project{GetProject()};
        Core::FString Error{};

        if (Project == nullptr || Project->GetAssets()->LoadMesh(Path, Error) == nullptr) {
            AddLog(Error.empty() ? "Cannot place the mesh" : Error);
            return nullptr;
        }

        Engine::AStaticMeshActor* Actor{GetWorld()->SpawnPrimitive(Engine::EPrimitiveType::Cube)};

        if (!ApplySpawnPlacement(Actor, Placement)) {
            return nullptr;
        }

        if (!Actor->GetStaticMeshComponent()->SetMeshPath(Project->GetAssets()->GetRelativePath(Path, Error))) {
            GetWorld()->DestroyActor(Actor->GetHandle());
            return nullptr;
        }

        Actor->SetName(Core::FilePath::ToUtf8(Path.stem()));
        CompleteActorSpawn(Actor, "Placed ");

        return Actor;
    }

    void Editor::FEditorContext::SetProjectRequestHandler(std::function<void(const std::filesystem::path&, bool)> Handler) {
        mProjectRequestHandler = std::move(Handler);
    }

    void Editor::FEditorContext::RequestProject(const std::filesystem::path& Path, bool Create) {
        if (CanEditWorld() && mProjectRequestHandler) {
            mProjectRequestHandler(Path, Create);
        }
    }

    /**
     * @brief 콘솔 로그 모델에 메시지를 추가합니다.
     */
    void Editor::FEditorContext::AddLog(const Core::FString& Message) {
        if (mLogs.size() >= 500) {
            mLogs.erase(mLogs.begin(), mLogs.begin() + 100);
        }

        mLogs.push_back(Message);
        ++mLogRevision;
        OutputDebugStringA((Message + "\n").c_str());
    }

    /**
     * @brief 수집한 콘솔 로그를 모두 지웁니다.
     */
    void Editor::FEditorContext::ClearLogs() {
        mLogs.clear();
        ++mLogRevision;
    }

    /**
     * @brief UI가 표시할 로그 목록을 반환합니다.
     */
    const Core::TArray<Core::FString>& Editor::FEditorContext::GetLogs() const {
        return mLogs;
    }

    /**
     * @brief 로그의 변경 세대를 반환합니다.
     */
    Core::Uint64 Editor::FEditorContext::GetLogRevision() const {
        return mLogRevision;
    }

    /**
     * @brief 입력한 콘솔 명령을 해석하여 공통 편집 동작을 실행합니다.
     */
    void Editor::FEditorContext::ExecuteConsoleCommand(const Core::FString& Command) {
        AddLog("> " + Command);

        std::istringstream Input{Command};
        Core::FString Verb{};
        Input >> Verb;
        std::transform(Verb.begin(), Verb.end(), Verb.begin(), [](unsigned char Character) {
            return static_cast<char>(std::tolower(Character));
        });

        if (Verb == "shadow") {
            Core::FString Target{};
            Core::FString Extra{};
            Input >> Target >> Extra;
            std::transform(Target.begin(), Target.end(), Target.begin(), [](unsigned char Character) {
                return static_cast<char>(std::tolower(Character));
            });

            if (Target == "atlas" && Extra.empty()) {
                SetShadowAtlasOpen(true);
            } else {
                AddLog("Usage: Shadow Atlas");
            }
        } else if (Verb == "clear") {
            ClearLogs();
        } else if (Verb == "help") {
            AddLog("help | clear | spawn plane/cube/sphere | save <Maps/Scene.masset> | load <Maps/Scene.masset>");
            AddLog("Shadow Atlas | ShowFlag.Shadows [on/off] | stat shadow [on/off]");
            AddLog("Allow Tearing [on/off/status]");

            if (mViewportCommandHandler) {
                AddLog("stat <group>/all [on/off] | stat none | stat list | stat reset | View > Statistics");
            }
        } else if (Verb == "allow") {
            Core::FString Feature{};
            Core::FString State{};
            Core::FString Extra{};
            Input >> Feature >> State >> Extra;

            for (Core::FString* Token : {&Feature, &State}) {
                std::transform(Token->begin(), Token->end(), Token->begin(), [](unsigned char Character) {
                    return static_cast<char>(std::tolower(Character));
                });
            }

            if (Feature != "tearing" || !Extra.empty() || (!State.empty() && State != "on" && State != "off" && State != "status")) {
                AddLog("Usage: Allow Tearing [on/off/status]");
                return;
            }

            RenderCore::IRendererModule* Renderer{mEngine != nullptr ? mEngine->GetRenderer() : nullptr};

            if (Renderer == nullptr) {
                AddLog("Allow Tearing: renderer is unavailable.");
                return;
            }

            if (State == "status") {
                AddLog(Renderer->IsTearingSupported() ? Renderer->IsTearingAllowed() ? "Allow Tearing: on (VSync off in windowed mode)." : "Allow Tearing: off (VSync on)." : "Allow Tearing: unsupported (VSync on).");
                return;
            }

            if (!Renderer->SetAllowTearing(State != "off")) {
                AddLog("Allow Tearing: unsupported or renderer is not initialized.");
                return;
            }

            AddLog(Renderer->IsTearingAllowed() ? "Allow Tearing: on (VSync off in windowed mode)." : "Allow Tearing: off (VSync on).");
        } else if (Verb == "spawn") {
            Core::FString Type{};
            Input >> Type;

            if (Type == "plane" || Type == "cube" || Type == "sphere") {
                SpawnPrimitive(Type == "plane" ? Engine::EPrimitiveType::Plane : Type == "sphere" ? Engine::EPrimitiveType::Sphere : Engine::EPrimitiveType::Cube);
            } else {
                AddLog("Expected plane, cube or sphere.");
            }
        } else if (Verb == "save" || Verb == "load") {
            Core::FString Text{};
            std::getline(Input >> std::ws, Text);

            const std::filesystem::path Path{Text.empty() ? mScenePath : Core::FilePath::FromUtf8(Text)};

            if (Path.empty()) {
                AddLog("Specify a scene path.");
            } else if (Verb == "save") {
                SaveScene(Path);
            } else {
                LoadScene(Path);
            }
        } else if (!Verb.empty()) {
            Core::FString Output{};

            if (mViewportCommandHandler && mViewportCommandHandler(Command, Output)) {
                if (!Output.empty()) {
                    AddLog(Output);
                }
            } else {
                AddLog("Unknown command.");
            }
        }
    }

    void Editor::FEditorContext::SetViewportCommandHandler(std::function<bool(const Core::FString&, Core::FString&)> Handler) {
        mViewportCommandHandler = std::move(Handler);
    }

    bool Editor::FEditorContext::IsShadowAtlasOpen() const {
        return mShadowAtlasOpen;
    }

    void Editor::FEditorContext::SetShadowAtlasOpen(bool Open) {
        mShadowAtlasOpen = Open;
    }

    /**
     * @brief 새 액터 배치 변환 제공자를 저장합니다.
     */
    void Editor::FEditorContext::SetPlacementProvider(std::function<Core::FTransform()> Provider) {
        mPlacementProvider = std::move(Provider);
    }

    Core::TArray<CoreUObject::FObjectHandle> Editor::FEditorContext::GetSelectedActors() const {
        Core::TArray<CoreUObject::FObjectHandle> Result{};

        for (CoreUObject::FObjectHandle Handle : mSelectedActors) {
            if (const auto* Actor{CoreUObject::Cast<Engine::AActor>(ResolveObject(Handle))}; Actor != nullptr && !Actor->IsPendingDestroy()) {
                Result.push_back(Handle);
            }
        }

        return Result;
    }

    Core::TArray<CoreUObject::FObjectHandle> Editor::FEditorContext::GetTransformTargets() const {
        if (auto* Component{CoreUObject::Cast<Engine::USceneComponent>(GetSelectedObject())}; Component != nullptr) {
            return {Component->GetHandle()};
        }

        Core::TArray<CoreUObject::FObjectHandle> Result{};

        for (CoreUObject::FObjectHandle Handle : GetSelectedActors()) {
            const auto* Actor{CoreUObject::Cast<Engine::AActor>(ResolveObject(Handle))};

            if (Actor->GetRootComponent() != nullptr) {
                Result.push_back(Actor->GetRootComponent()->GetHandle());
            }
        }

        return Result;
    }

    bool Editor::FEditorContext::SetSelectedActors(const Core::TArray<CoreUObject::FObjectHandle>& Actors) {
        return SetSelection(Actors, Actors.empty() ? CoreUObject::FObjectHandle{} : Actors.back());
    }

    bool Editor::FEditorContext::ToggleActorSelection(CoreUObject::FObjectHandle Actor) {
        Core::TArray<CoreUObject::FObjectHandle> Actors{GetSelectedActors()};
        const auto Position{std::find(Actors.begin(), Actors.end(), Actor)};

        if (Position != Actors.end()) {
            Actors.erase(Position);
        } else {
            Actors.push_back(Actor);
        }

        return SetSelectedActors(Actors);
    }

    Core::Uint64 Editor::FEditorContext::GetSelectionRevision() const {
        return mSelectionRevision;
    }

    Core::TArray<CoreUObject::FObjectHandle> Editor::FEditorContext::DuplicateSelectedActors() {
        if (!CanEditWorld() || mChangingState || mEditingProperty || mEngine->IsDispatching()) {
            return {};
        }

        Core::FString Error{};
        Core::TArray<CoreUObject::FObjectHandle> Copies{};

        {
            FOperationScope Scope{&mChangingState};

            Copies = Engine::FWorldCloner::DuplicateActors(GetWorld(), GetSelectedActors(), Error);
        }

        if (!Copies.empty()) {
            SetSelectedActors(Copies);
            mDirty = true;
        } else if (!Error.empty()) {
            AddLog(Error);
        }

        return Copies;
    }

    Engine::AActor* Editor::FEditorContext::SpawnActorPreset(const Core::FString& Preset) {
        Core::FTransform Placement{mPlacementProvider ? mPlacementProvider() : Core::FTransform{}};

        if (Preset.find("DirectionalLight") != Core::FString::npos || Preset.find("SpotLight") != Core::FString::npos) {
            Placement.mRotation = Core::FRotator{-45.0f, 0.0f, 0.0f}.Quaternion();
        }

        return SpawnActorPreset(Preset, Placement);
    }

    Engine::AActor* Editor::FEditorContext::SpawnActorPreset(const Core::FString& Preset, const Core::FTransform& Placement) {
        if (!CanSpawnActor() || GetWorld() == nullptr || !Placement.IsValid()) {
            return nullptr;
        }

        FOperationScope Scope{&mChangingState};
        Engine::UWorld* World{GetWorld()};
        const bool Primitive{Preset == "Cube" || Preset == "Sphere" || Preset == "Plane"};
        const bool Text{Preset == "Text"};
        const bool Billboard{Preset == "Billboard"};
        Engine::AActor* Actor{Primitive ? World->SpawnPrimitive(Preset == "Sphere" ? Engine::EPrimitiveType::Sphere : Preset == "Plane" ? Engine::EPrimitiveType::Plane : Engine::EPrimitiveType::Cube) : World->SpawnActor(Text || Billboard ? Engine::AActor::StaticClass() : CoreUObject::FReflectionRegistry::FindClass(Preset))};

        if (Actor == nullptr) {
            return nullptr;
        }

        if (Actor->GetRootComponent() == nullptr) {
            std::unique_ptr<Engine::UActorComponent> Component{Text ? std::unique_ptr<Engine::UActorComponent>{CoreUObject::NewObject<Engine::UBillboardTextComponent>()} : Billboard ? std::unique_ptr<Engine::UActorComponent>{CoreUObject::NewObject<Engine::UBillboardComponent>()} : std::unique_ptr<Engine::UActorComponent>{CoreUObject::NewObject<Engine::USceneComponent>()}};
            Component->SetName(Text ? "BillboardTextComponent" : Billboard ? "BillboardComponent" : "SceneComponent");

            if (!Actor->AddComponent(std::move(Component))) {
                World->DestroyActor(Actor->GetHandle());
                return nullptr;
            }
        }

        if (!ApplySpawnPlacement(Actor, Placement)) {
            return nullptr;
        }

        if (auto* Sprite{CoreUObject::Cast<Engine::UBillboardComponent>(Actor->GetRootComponent())}) {
            Sprite->SetTexturePath("Editor/Icon/Pawn_64x.png");
        }

        CompleteActorSpawn(Actor, "Placed ");

        return Actor;
    }

}
