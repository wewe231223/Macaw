/**
 * @file FEditorContext.cpp
 * @brief UI와 독립적인 선택, 장면 상태와 공통 편집 연산을 구현합니다.
 * @ingroup Editor
 */

#include "Editor/FEditorContext.h"
#include "Core/FilePath.h"
#include "Engine/FProject.h"
#include "Engine/Component/UMeshComponent.h"
#include "Script/UScriptComponent.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Engine/ASkeletalMeshActor.h"
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
#include "Engine/ALevelSequenceActor.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/SceneSerializer.h"
#include "Engine/FWorldCloner.h"
#include "Engine/FWorldSerializer.h"
#include "Serialization/MemoryArchive.h"
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
#include <utility>

namespace Editor {

    Editor::FEditorContext::FSceneEditScope::FSceneEditScope(Editor::FEditorContext& Context)
		: mContext{Context},
		  mToken{Context.BeginSceneEdit()} {
    }

    Editor::FEditorContext::FSceneEditScope::~FSceneEditScope() {
        mContext.EndSceneEdit(mToken);
    }

    bool Editor::FEditorContext::FSceneEditCommand::Undo(Editor::FEditorContext& Context) const {
        return Context.RestoreSceneMemento(mBefore);
    }

    bool Editor::FEditorContext::FSceneEditCommand::Redo(Editor::FEditorContext& Context) const {
        return Context.RestoreSceneMemento(mAfter);
    }

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
        RegisterConsoleCommands();
    }

    /**
     * @brief 편집 상태와 콜백 저장소를 정리합니다.
     */
    Editor::FEditorContext::~FEditorContext() = default;

    /**
     * @brief 명시적인 편집 월드 연결을 변경합니다.
     */
    bool Editor::FEditorContext::SetWorld(Engine::FWorldHandle Handle) {
        if (mPlayLocked || IsChangingState() || mEngine == nullptr || mEngine->GetWorld(Handle) == nullptr || mEngine->IsDispatching()) {
            return false;
        }

        FOperationScope Scope{&mChangingState};

        mWorld = Handle;

        AssignSelection({}, {});
        mScenePath.clear();
        ClearSceneHistory();
        mSavedSceneRevision = mSceneRevision;

        return true;
    }

    /**
     * @brief UI와 월드 사이의 비소유 편집 연결을 해제합니다.
     */
    void Editor::FEditorContext::Reset() {
        if (mPlayLocked || IsChangingState()) {
            return;
        }

        FOperationScope Scope{&mChangingState};

        AssignSelection({}, {});

        mWorld = {};
        mScenePath.clear();
        ClearSceneHistory();
        mSavedSceneRevision = mSceneRevision;

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

    bool Editor::FEditorContext::CanModifyScene() const {
        return CanEditWorld() && !IsChangingState() && !mEngine->IsDispatching();
    }

    Core::Uint64 Editor::FEditorContext::BeginSceneEdit() {
        if (IsSceneEditActive() || !CanModifyScene()) {
            return {};
        }

        FOperationScope Scope{&mChangingState};
        RestorePreview();

        FSceneMemento State{};

        if (!CaptureSceneMemento(State)) {
            return {};
        }

        mPendingSceneEdit = std::move(State);
        mSceneEditToken = ++mSceneEditSerial;

        return mSceneEditToken;
    }

    void Editor::FEditorContext::EndSceneEdit(Core::Uint64 Token) {
        if (Token == 0 || Token != mSceneEditToken) {
            return;
        }

        const bool Changed{mSceneEditChanged};
        FSceneMemento Before{std::exchange(mPendingSceneEdit, {})};
        mSceneEditToken = 0;
        mSceneEditChanged = false;

        if (!Changed) {
            return;
        }

        FOperationScope Scope{&mChangingState};
        RestorePreview();

        FSceneMemento After{};

        if (!CaptureSceneMemento(After)) {
            ClearSceneHistory();

            return;
        }

        if (Before.mBytes == After.mBytes) {
            return;
        }

        mSceneHistory.resize(mSceneHistoryPosition);
        mSceneHistory.push_back({std::move(Before), std::move(After), mSceneRevision, Token});
        mSceneRevision = Token;
        mSceneHistoryPosition = mSceneHistory.size();
        TrimSceneHistory();
    }

    bool Editor::FEditorContext::IsSceneEditActive() const {
        return mSceneEditToken != 0;
    }

    bool Editor::FEditorContext::CanUndo() const {
        return mSceneHistoryPosition != 0 && !IsSceneEditActive() && CanModifyScene();
    }

    bool Editor::FEditorContext::CanRedo() const {
        return mSceneHistoryPosition < mSceneHistory.size() && !IsSceneEditActive() && CanModifyScene();
    }

    bool Editor::FEditorContext::Undo() {
        if (!CanUndo()) {
            return false;
        }

        const FSceneEditCommand& Command{mSceneHistory[mSceneHistoryPosition - 1]};

        if (!Command.Undo(*this)) {
            return false;
        }

        --mSceneHistoryPosition;
        mSceneRevision = Command.mBeforeRevision;

        return true;
    }

    bool Editor::FEditorContext::Redo() {
        if (!CanRedo()) {
            return false;
        }

        const FSceneEditCommand& Command{mSceneHistory[mSceneHistoryPosition]};

        if (!Command.Redo(*this)) {
            return false;
        }

        ++mSceneHistoryPosition;
        mSceneRevision = Command.mAfterRevision;

        return true;
    }

    bool Editor::FEditorContext::CaptureSceneMemento(FSceneMemento& State) {
        Serialization::FMemoryArchive Archive{State.mBytes};
        Engine::FWorldSerializer::Serialize(Archive, mEngine->GetWorldContext(mWorld));

        if (Archive.HasError()) {
            AddLog("Cannot capture scene history: " + Archive.GetError());

            return false;
        }

        for (CoreUObject::FObjectHandle Handle : GetSelectedActors()) {
            State.mSelectedActors.push_back(ResolveObject(Handle)->GetGuid());
        }

        if (const CoreUObject::UObject* Object{GetSelectedObject()}) {
            State.mSelectedObject = Object->GetGuid();
        }

        return true;
    }

    bool Editor::FEditorContext::RestoreSceneMemento(const FSceneMemento& State) {
        FOperationScope Scope{&mChangingState};
        Engine::UWorld* Previous{GetWorld()};
        auto Context{std::make_unique<Engine::FWorldContext>(Previous->GetWorldType())};
        Engine::UWorld* World{Context->GetWorld()};
        World->SetAssets(Previous->GetAssets());
        Context->GetEditorCamera() = mEngine->GetWorldContext(mWorld)->GetEditorCamera();

        Serialization::FMemoryArchive Archive{State.mBytes};
        Engine::FWorldSerializer::Serialize(Archive, Context.get());

        if (Archive.HasError() || Archive.GetRemainingBytes() != 0) {
            AddLog(Archive.HasError() ? "Cannot restore scene history: " + Archive.GetError() : "Cannot restore scene history: unread data");

            return false;
        }

        Core::TArray<CoreUObject::FObjectHandle> SelectedActors{};
        CoreUObject::FObjectHandle SelectedObject{};
        Core::TMap<Core::FGuid, CoreUObject::FObjectHandle, Core::FGuidHash> Actors{};

        for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
            Actors.emplace(Actor->GetGuid(), Actor->GetHandle());

            if (Actor->GetGuid() == State.mSelectedObject) {
                SelectedObject = Actor->GetHandle();
            }

            for (const auto& Component : Actor->GetComponents()) {
                if (Component->GetGuid() == State.mSelectedObject) {
                    SelectedObject = Component->GetHandle();
                }
            }
        }

        for (const Core::FGuid& Guid : State.mSelectedActors) {
            if (const auto Position{Actors.find(Guid)}; Position != Actors.end()) {
                SelectedActors.push_back(Position->second);
            }
        }

        RestorePreview();

        if (!ReplaceWorld(std::move(Context))) {
            AddLog("Cannot replace the editor world for scene history");

            return false;
        }

        AssignSelection(SelectedActors, SelectedObject);

        return true;
    }

    void Editor::FEditorContext::NoteSceneEdit() {
        if (IsSceneEditActive()) {
            mSceneEditChanged = true;
        } else {
            ClearSceneHistory();
        }
    }

    void Editor::FEditorContext::ClearSceneHistory() {
        mSceneHistory.clear();
        mPendingSceneEdit = {};
        mSceneHistoryPosition = 0;
        mSceneEditToken = 0;
        mSceneEditChanged = false;
        mSceneRevision = ++mSceneEditSerial;
    }

    void Editor::FEditorContext::TrimSceneHistory() {
        constexpr std::size_t MaxEntries{128};
        constexpr std::size_t MaxBytes{64 * 1024 * 1024};
        std::size_t First{mSceneHistory.size() - 1};
        std::size_t Bytes{mSceneHistory[First].mBefore.mBytes.size() + mSceneHistory[First].mAfter.mBytes.size()};

        while (First != 0 && mSceneHistory.size() - First < MaxEntries && Bytes < MaxBytes) {
            const FSceneEditCommand& Previous{mSceneHistory[First - 1]};
            const std::size_t Size{Previous.mBefore.mBytes.size() + Previous.mAfter.mBytes.size()};

            if (Size > MaxBytes - Bytes) {
                break;
            }

            Bytes += Size;
            --First;
        }

        mSceneHistory.erase(mSceneHistory.begin(), mSceneHistory.begin() + static_cast<std::ptrdiff_t>(First));
        mSceneHistoryPosition -= First;
    }

    /**
     * @brief 편집 상태 변경을 처리 중이어서 재진입이 제한되는지 반환합니다.
     */
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
        return SetSelection({Handle}, Handle);
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
        if (mPlayLocked || IsChangingState()) {
            return false;
        }

        FOperationScope Scope{&mChangingState};

        for (auto Position{Actors.begin()}; Position != Actors.end(); ++Position) {
            const CoreUObject::FObjectHandle Handle{*Position};

            if (CoreUObject::Cast<Engine::AActor>(ResolveObject(Handle)) == nullptr || std::find(Actors.begin(), Position, Handle) != Position) {
                return false;
            }
        }

        if (Object.IsValid() && ResolveObject(Object) == nullptr) {
            return false;
        }

        if (mSelectedActors != Actors || mSelectedObject != Object) {
            AssignSelection(Actors, Object);
        }

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
        for (auto Position{mSelectedActors.rbegin()}; Position != mSelectedActors.rend(); ++Position) {
            if (auto* Actor{CoreUObject::Cast<Engine::AActor>(ResolveObject(*Position))}) {
                return Actor;
            }
        }

        return nullptr;
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
        if (!CanModifyScene()) {
            return nullptr;
        }

        FSceneEditScope Edit{*this};
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

    /**
     * @brief 편집 월드에 기본 카메라 액터를 생성합니다.
     */
    Engine::ACameraActor* Editor::FEditorContext::SpawnCamera() {
        if (!CanModifyScene()) {
            return nullptr;
        }

        FSceneEditScope Edit{*this};
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

    /**
     * @brief 배치 위치에 빌보드 텍스트 액터를 생성하고 선택합니다.
     */
    Engine::AActor* Editor::FEditorContext::SpawnText() {
        if (!CanModifyScene()) {
            return nullptr;
        }

        FSceneEditScope Edit{*this};
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

    /**
     * @brief 배치 위치에 텍스처 빌보드 액터를 생성하고 선택과 장면 변경 상태를 갱신합니다.
     */
    Engine::AActor* Editor::FEditorContext::SpawnBillboard(const Core::FString& SpritePath) {
        if (!CanModifyScene()) {
            return nullptr;
        }

        FSceneEditScope Edit{*this};
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

    /**
     * @brief 새 액터에 배치 변환을 적용하고 실패하면 생성한 액터를 제거합니다.
     */
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

    /**
     * @brief 생성한 액터를 선택하고 장면 변경 상태와 로그를 갱신합니다.
     */
    void Editor::FEditorContext::CompleteActorSpawn(Engine::AActor* Actor, const char* Action) {
        AssignSelection({Actor->GetHandle()}, Actor->GetHandle());

        NoteSceneEdit();

        AddLog(Action + Actor->GetName());
    }

    /**
     * @brief 현재 선택 액터와 편집 상태에서 해당 컴포넌트를 생성할 수 있는지 검사합니다.
     */
    bool Editor::FEditorContext::CanAddComponent(const CoreUObject::UClass* Class) const {
        return CanModifyScene() && GetSelectedActors().size() == 1 && Class != nullptr && Class != Engine::UActorComponent::StaticClass() && Class->IsChildOf(Engine::UActorComponent::StaticClass()) && Class->CanCreate();
    }

    /**
     * @brief 선택한 액터에 리플렉션 클래스로 컴포넌트를 생성하여 등록합니다.
     */
    Engine::UActorComponent* Editor::FEditorContext::AddComponent(const CoreUObject::UClass* Class) {
        if (!CanAddComponent(Class)) {
            return nullptr;
        }

        FSceneEditScope Edit{*this};
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

        NoteSceneEdit();

        auto* Added{CoreUObject::Cast<Engine::UActorComponent>(ResolveObject(ComponentHandle))};

        if (Added == nullptr) {
            return nullptr;
        }

        AssignSelection({ActorHandle}, ComponentHandle);

        AddLog("Added " + Name);

        return Added;
    }

    /**
     * @brief 현재 선택에서 사용할 수 있는 카메라 컴포넌트를 조회합니다.
     */
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

    /**
     * @brief 선택한 카메라를 월드의 활성 카메라로 지정합니다.
     */
    bool Editor::FEditorContext::UseSelectedCamera() {
        Engine::UCameraComponent* Camera{GetSelectedCamera()};

        if (!CanModifyScene() || Camera == nullptr) {
            return false;
        }

        FSceneEditScope Edit{*this};

        if (!GetWorld()->SetActiveCamera(Camera)) {
            return false;
        }

        NoteSceneEdit();

        return GetWorld()->SetInputTarget(Camera->GetOwner());
    }

    bool Editor::FEditorContext::SetActorsVisible(const Core::TArray<CoreUObject::FObjectHandle>& Actors, bool Visible) {
        if (!CanModifyScene()) {
            return false;
        }

        FSceneEditScope Edit{*this};
        bool Changed{};

        for (CoreUObject::FObjectHandle Handle : Actors) {
            if (auto* Actor{CoreUObject::Cast<Engine::AActor>(ResolveObject(Handle))}; Actor != nullptr && Actor->IsVisible() != Visible) {
                Actor->SetVisible(Visible);
                Changed = true;
            }
        }

        if (Changed) {
            NoteSceneEdit();
        }

        return Changed;
    }

    /**
     * @brief 현재 선택한 액터를 제거하고 관련 선택 상태를 초기화합니다.
     */
    bool Editor::FEditorContext::DestroySelectedActor() {
        if (!CanModifyScene()) {
            return false;
        }

        const Core::TArray<CoreUObject::FObjectHandle> Actors{GetSelectedActors()};

        if (Actors.empty()) {
            return false;
        }

        FSceneEditScope Edit{*this};
        FOperationScope Scope{&mChangingState};
        Engine::UWorld* World{GetWorld()};
        bool Removed{};

        for (CoreUObject::FObjectHandle Handle : Actors) {
            Removed = World->DestroyActor(Handle) || Removed;
        }

        if (Removed) {
            AssignSelection({}, {});

            NoteSceneEdit();

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

        FSceneEditScope Edit{*this};
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

        NoteSceneEdit();

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
        if (mPlayLocked) {
            AddLog("Stop PIE before saving the scene");

            return false;
        }

        if (Path.empty() || IsSceneEditActive() || !CanModifyScene()) {
            return false;
        }

        RestorePreview();

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
        mSavedSceneRevision = mSceneRevision;

        AddLog("Saved " + Core::FilePath::ToUtf8(ScenePath));

        return true;
    }

    /**
     * @brief 새 컨텍스트를 엔진에 등록하고 기존 편집 월드만 교체합니다.
     */
    bool Editor::FEditorContext::ReplaceScene(std::unique_ptr<Engine::FWorldContext> Context, const std::filesystem::path& Path) {
        RestorePreview();

        if (!ReplaceWorld(std::move(Context))) {
            return false;
        }

        AssignSelection({}, {});
        mScenePath = Path;
        ClearSceneHistory();
        mSavedSceneRevision = mSceneRevision;

        if (mCameraRestorer) {
            mCameraRestorer(mEngine->GetWorldContext(mWorld)->GetEditorCamera());
        }

        return true;
    }

    bool Editor::FEditorContext::ReplaceWorld(std::unique_ptr<Engine::FWorldContext> Context) {
        const Engine::FWorldHandle Previous{mWorld};
        const Engine::FWorldHandle Current{mEngine->GetWorldHandle()};
        const Engine::FWorldHandle Replacement{mEngine->RegisterWorld(std::move(Context))};

        if (!Replacement.IsValid()) {
            return false;
        }

        if (mEngine->GetWorld(Previous) != nullptr && !mEngine->DestroyWorld(Previous)) {
            mEngine->DestroyWorld(Replacement);

            return false;
        }

        if (Current == Previous || !Current.IsValid()) {
            mEngine->SetCurrentWorld(Replacement);
        }

        mWorld = Replacement;

        return true;
    }

    /**
     * @brief 프로퍼티 편집을 거치지 않은 장면 변경을 수정 상태로 표시합니다.
     */
    void Editor::FEditorContext::MarkSceneDirty() {
        ClearSceneHistory();
    }

    /**
     * @brief 시퀀서 프리뷰가 월드에 쓴 값을 되돌리는 콜백을 설정합니다.
     */
    void Editor::FEditorContext::SetPreviewRestorer(std::function<void()> Restorer) {
        mPreviewRestorer = std::move(Restorer);
    }

    /**
     * @brief 시퀀서 프리뷰가 월드에 쓴 값을 되돌립니다.
     */
    void Editor::FEditorContext::RestorePreview() {
        if (mPreviewRestorer) {
            mPreviewRestorer();
        }
    }

    /**
     * @brief 시퀀스 액터를 시퀀서에서 여는 콜백을 설정합니다.
     */
    void Editor::FEditorContext::SetSequencerOpener(std::function<void(CoreUObject::FObjectHandle)> Opener) {
        mSequencerOpener = std::move(Opener);
    }

    /**
     * @brief 객체가 시퀀스 액터이면 시퀀서에서 엽니다.
     */
    bool Editor::FEditorContext::OpenInSequencer(CoreUObject::FObjectHandle Handle) {
        if (!mSequencerOpener || CoreUObject::Cast<Engine::ALevelSequenceActor>(ResolveObject(Handle)) == nullptr) {
            return false;
        }

        mSequencerOpener(Handle);

        return true;
    }

    /**
     * @brief 원근 편집 뷰포트가 대신 사용할 시퀀서 프리뷰 카메라를 설정합니다.
     */
    void Editor::FEditorContext::SetPreviewCamera(CoreUObject::FObjectHandle Camera) {
        mPreviewCamera = Camera;
    }

    /**
     * @brief 시퀀서 프리뷰 카메라를 조회합니다.
     */
    Engine::UCameraComponent* Editor::FEditorContext::GetPreviewCamera() const {
        return CoreUObject::Cast<Engine::UCameraComponent>(ResolveObject(mPreviewCamera));
    }

    /**
     * @brief 장면 저장·복원에 사용할 원근 카메라 콜백을 연결합니다.
     */
    void Editor::FEditorContext::SetCameraHandlers(std::function<Engine::FEditorCameraState()> Provider, std::function<void(const Engine::FEditorCameraState&)> Restorer) {
        mCameraProvider = std::move(Provider);
        mCameraRestorer = std::move(Restorer);
    }

    /**
     * @brief 슬롯 재질을 변경하고 장면을 수정 상태로 표시합니다.
     */
    bool Editor::FEditorContext::SetMaterialOverride(CoreUObject::FObjectHandle Handle, Core::Uint32 Slot, const Core::FString& Path) {
        if (!CanModifyScene()) {
            return false;
        }

        FSceneEditScope Edit{*this};
        auto* Component{CoreUObject::Cast<Engine::UMeshComponent>(ResolveObject(Handle))};

        if (Component == nullptr || !Component->SetSlotMaterialPath(Slot, Path)) {
            return false;
        }

        NoteSceneEdit();

        return true;
    }

    /**
     * @brief C# 스크립트 필드 값을 저장하고 장면을 수정 상태로 표시합니다.
     */
    bool Editor::FEditorContext::SetScriptField(CoreUObject::FObjectHandle Handle, const Script::FScriptField& Field) {
        auto* Component{CoreUObject::Cast<Script::UScriptComponent>(ResolveObject(Handle))};

        if (!CanModifyScene() || Component == nullptr) {
            return false;
        }

        FSceneEditScope Edit{*this};
        Component->SetField(Field);
        NoteSceneEdit();

        return true;
    }

    /**
     * @brief 저장된 C# 스크립트 필드를 지우고 장면을 수정 상태로 표시합니다.
     */
    bool Editor::FEditorContext::RemoveScriptField(CoreUObject::FObjectHandle Handle, const Core::FString& Name) {
        auto* Component{CoreUObject::Cast<Script::UScriptComponent>(ResolveObject(Handle))};

        if (!CanModifyScene() || Component == nullptr) {
            return false;
        }

        FSceneEditScope Edit{*this};
        Component->RemoveField(Name);
        NoteSceneEdit();

        return true;
    }

    /**
     * @brief 장면 파일을 새 컨텍스트에 읽고 편집 대상 월드만 교체합니다.
     */
    bool Editor::FEditorContext::LoadScene(const std::filesystem::path& Path) {
        if (Path.empty() || IsSceneEditActive() || !CanModifyScene()) {
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
        if (IsSceneEditActive() || !CanModifyScene()) {
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

        return mSceneEditChanged || mSceneRevision != mSavedSceneRevision || (Context != nullptr && mCameraProvider && Context->GetEditorCamera() != mCameraProvider());
    }

    /**
     * @brief 현재 연결된 프로젝트의 비소유 주소를 반환합니다.
     */
    Engine::FProject* Editor::FEditorContext::GetProject() const {
        return mEngine != nullptr ? mEngine->GetProject() : nullptr;
    }

    /**
     * @brief 배치 제공자가 반환한 위치에 메시 에셋 액터를 생성합니다.
     */
    Engine::AStaticMeshActor* Editor::FEditorContext::SpawnMesh(const std::filesystem::path& Path) {
        return SpawnMesh(Path, mPlacementProvider ? mPlacementProvider() : Core::FTransform{});
    }

    /**
     * @brief 지정한 변환에 메시 에셋 액터를 생성하고 선택합니다.
     */
    Engine::AStaticMeshActor* Editor::FEditorContext::SpawnMesh(const std::filesystem::path& Path, const Core::FTransform& Placement) {
        if (!CanModifyScene() || !Placement.IsValid()) {
            return nullptr;
        }

        FSceneEditScope Edit{*this};
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

    /**
     * @brief 애플리케이션에 프로젝트 전환을 요청할 콜백을 설정합니다.
     */
    void Editor::FEditorContext::SetProjectRequestHandler(std::function<void(const std::filesystem::path&, bool)> Handler) {
        mProjectRequestHandler = std::move(Handler);
    }

    /**
     * @brief 지정 위치에 스켈레탈 메시 액터를 배치하고 선택합니다.
     */
    Engine::ASkeletalMeshActor* Editor::FEditorContext::SpawnSkeletalMesh(const std::filesystem::path& Path, const Core::FTransform& Placement) {
        if (!CanModifyScene() || !Placement.IsValid() || GetProject() == nullptr) {
            return nullptr;
        }

        FSceneEditScope Edit{*this};
        FOperationScope Scope{&mChangingState};
        Core::FString Error{};
        Asset::FAssetManager* Assets{GetProject()->GetAssets()};

        if (Assets->LoadSkeletalMesh(Path, Error) == nullptr) {
            AddLog(Error);

            return nullptr;
        }

        auto* Actor{CoreUObject::Cast<Engine::ASkeletalMeshActor>(GetWorld()->SpawnActor(Engine::ASkeletalMeshActor::StaticClass()))};

        if (!ApplySpawnPlacement(Actor, Placement)) {
            return nullptr;
        }

        if (Actor->GetSkeletalMeshComponent() == nullptr || !Actor->GetSkeletalMeshComponent()->SetMeshPath(Assets->GetRelativePath(Path, Error))) {
            GetWorld()->DestroyActor(Actor->GetHandle());

            return nullptr;
        }

        Actor->SetName(Core::FilePath::ToUtf8(Path.stem()));
        CompleteActorSpawn(Actor, "Placed ");

        return Actor;
    }

    /**
     * @brief 등록된 처리기에 프로젝트 생성 또는 열기 요청을 전달합니다.
     */
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
     * @brief 그림자 아틀라스 검사 창의 표시 상태를 반환합니다.
     */
    bool Editor::FEditorContext::IsShadowAtlasOpen() const {
        return mShadowAtlasOpen;
    }

    /**
     * @brief 그림자 아틀라스 검사 창의 표시 상태를 변경합니다.
     */
    void Editor::FEditorContext::SetShadowAtlasOpen(bool Open) {
        mShadowAtlasOpen = Open;
    }

    /**
     * @brief 새 액터 배치 변환 제공자를 저장합니다.
     */
    void Editor::FEditorContext::SetPlacementProvider(std::function<Core::FTransform()> Provider) {
        mPlacementProvider = std::move(Provider);
    }

    /**
     * @brief 아직 살아 있는 선택 액터들의 세대 핸들을 수집합니다.
     */
    Core::TArray<CoreUObject::FObjectHandle> Editor::FEditorContext::GetSelectedActors() const {
        Core::TArray<CoreUObject::FObjectHandle> Result{};

        for (CoreUObject::FObjectHandle Handle : mSelectedActors) {
            if (CoreUObject::Cast<Engine::AActor>(ResolveObject(Handle)) != nullptr) {
                Result.push_back(Handle);
            }
        }

        return Result;
    }

    /**
     * @brief 선택 컴포넌트 또는 선택 액터들의 루트 변환 대상을 수집합니다.
     */
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

    /**
     * @brief 유효한 액터 핸들들로 현재 다중 선택을 교체합니다.
     */
    bool Editor::FEditorContext::SetSelectedActors(const Core::TArray<CoreUObject::FObjectHandle>& Actors) {
        return SetSelection(Actors, Actors.empty() ? CoreUObject::FObjectHandle{} : Actors.back());
    }

    /**
     * @brief 지정 액터의 다중 선택 포함 상태를 반전합니다.
     */
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

    /**
     * @brief 선택 대상 변경을 추적할 개정 번호를 반환합니다.
     */
    Core::Uint64 Editor::FEditorContext::GetSelectionRevision() const {
        return mSelectionRevision;
    }

    /**
     * @brief 현재 선택 액터들을 새 GUID로 복제하고 복제본들을 선택합니다.
     */
    Core::TArray<CoreUObject::FObjectHandle> Editor::FEditorContext::DuplicateSelectedActors() {
        if (!CanModifyScene()) {
            return {};
        }

        FSceneEditScope Edit{*this};
        Core::FString Error{};
        Core::TArray<CoreUObject::FObjectHandle> Copies{};

        {
            FOperationScope Scope{&mChangingState};

            Copies = Engine::FWorldCloner::DuplicateActors(GetWorld(), GetSelectedActors(), Error);
        }

        if (!Copies.empty()) {
            SetSelectedActors(Copies);

            NoteSceneEdit();
        } else if (!Error.empty()) {
            AddLog(Error);
        }

        return Copies;
    }

    /**
     * @brief 지정한 프리셋의 액터를 생성·배치하고 선택과 장면 변경 상태를 갱신합니다.
     */
    Engine::AActor* Editor::FEditorContext::SpawnActorPreset(const Core::FString& Preset) {
        Core::FTransform Placement{mPlacementProvider ? mPlacementProvider() : Core::FTransform{}};

        if (Preset.find("DirectionalLight") != Core::FString::npos || Preset.find("SpotLight") != Core::FString::npos) {
            Placement.mRotation = Core::FRotator{-45.0f, 0.0f, 0.0f}.Quaternion();
        }

        return SpawnActorPreset(Preset, Placement);
    }

    /**
     * @brief 지정한 프리셋의 액터를 생성·배치하고 선택과 장면 변경 상태를 갱신합니다.
     */
    Engine::AActor* Editor::FEditorContext::SpawnActorPreset(const Core::FString& Preset, const Core::FTransform& Placement) {
        if (!CanModifyScene() || !Placement.IsValid()) {
            return nullptr;
        }

        FSceneEditScope Edit{*this};
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
