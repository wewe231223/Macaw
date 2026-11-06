/**
 * @file FSequencerPanel.cpp
 * @brief 시퀀스 액터의 타임라인 편집과 프리뷰를 제공하는 에디터 패널입니다.
 * @ingroup Editor
 */

#include "Editor/Panel/FSequencerPanel.h"
#include "Editor/FEditorContext.h"
#include "EditorStyle.h"
#include "Asset/FAssetManager.h"
#include "Asset/Sequence/FSequencePropertyTrack.h"
#include "Asset/Sequence/FSequenceTransformTrack.h"
#include "Asset/Sequence/ULevelSequence.h"
#include "Core/FilePath.h"
#include "Core/Math/FRotator.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Engine/ALevelSequenceActor.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/FProject.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"

#include <imgui.h>
#include <imgui_neo_sequencer.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace Editor {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 시퀀서에서 선택할 표시 프레임률입니다.
         */
        constexpr std::array<Core::Int32, 6> DisplayRates{24, 25, 30, 48, 50, 60};

        /**
         * @brief 시퀀서 보간 모드의 표시 이름입니다.
         */
        constexpr std::array<const char*, 3> InterpolationNames{"Constant", "Linear", "Cubic"};

        /**
         * @brief 시퀀서 UI의 마지막 프레임 상한입니다.
         */
        constexpr Core::Int32 MaximumEndFrame{1000000};

        // 채널 번호는 변환 채널 다음에 프로퍼티 트랙의 순서를 이어 붙입니다.

        /**
         * @brief 변환 채널 뒤 첫 프로퍼티 채널 인덱스입니다.
         */
        constexpr Core::Int32 FirstPropertyChannel{static_cast<Core::Int32>(Asset::FSequenceTransformSection::ChannelCount)};

        // 키 선택의 바인딩 번호 자리에 넣어 카메라 컷 줄을 구분합니다.

        /**
         * @brief 카메라 컷 행을 구분하는 특수 채널 값입니다.
         */
        constexpr Core::Int32 CameraCutRow{-2};

        /**
         * @brief 바인딩의 프로퍼티 트랙 중 지정 순번의 트랙을 찾습니다.
         * @param[in] Binding 조회할 시퀀스 바인딩입니다.
         * @param[in] PropertyIndex 찾을 프로퍼티 트랙 순번입니다.
         * @return 지정 순번의 프로퍼티 트랙이며 없으면 nullptr입니다.
         */
        Asset::FSequencePropertyTrack* FindPropertyTrack(const Asset::FSequenceBinding& Binding, Core::Int32 PropertyIndex) {
            Core::Int32 Index{};

            for (const auto& Track : Binding.GetTracks()) {
                if (Track->GetType() == Asset::ESequenceTrackType::Property && Index++ == PropertyIndex) {
                    return static_cast<Asset::FSequencePropertyTrack*>(Track.get());
                }
            }

            return nullptr;
        }

        /**
         * @brief 시퀀스와 바인딩 인덱스가 유효한지 검사합니다.
         * @param[in] Sequence 바인딩·트랙을 조회할 레벨 시퀀스입니다.
         * @param[in] Binding 조회할 시퀀스 바인딩입니다.
         * @return 시퀀스가 있고 바인딩 인덱스가 유효하면 true입니다.
         */
        bool IsValidBinding(const Asset::ULevelSequence* Sequence, Core::Int32 Binding) {
            return Binding >= 0 && Binding < static_cast<Core::Int32>(Sequence->GetBindings().size());
        }

        /**
         * @brief 변환·프로퍼티 채널 인덱스에 대응하는 트랙을 찾습니다.
         * @param[in] Sequence 바인딩·트랙을 조회할 레벨 시퀀스입니다.
         * @param[in] Binding 조회할 시퀀스 바인딩입니다.
         * @param[in] Channel 바인딩 내부의 변환·프로퍼티 채널 인덱스입니다.
         * @return 채널이 속한 트랙이며 없으면 nullptr입니다.
         */
        Asset::FSequenceTrack* FindTrack(Asset::ULevelSequence* Sequence, Core::Int32 Binding, Core::Int32 Channel) {
            if (!IsValidBinding(Sequence, Binding)) {
                return nullptr;
            }

            const Asset::FSequenceBinding& Owner{Sequence->GetBindings()[Binding]};

            if (Channel >= FirstPropertyChannel) {
                return FindPropertyTrack(Owner, Channel - FirstPropertyChannel);
            }

            return Owner.FindTransformTrack();
        }

        /**
         * @brief 바인딩·채널 인덱스로 편집 가능한 채널을 찾습니다.
         * @param[in] Sequence 바인딩·트랙을 조회할 레벨 시퀀스입니다.
         * @param[in] Binding 조회할 시퀀스 바인딩입니다.
         * @param[in] Channel 바인딩 내부의 변환·프로퍼티 채널 인덱스입니다.
         * @return 해당 채널의 비소유 주소이며 없으면 nullptr입니다.
         */
        Asset::FSequenceChannel* FindChannel(Asset::ULevelSequence* Sequence, Core::Int32 Binding, Core::Int32 Channel) {
            Asset::FSequenceTrack* Track{FindTrack(Sequence, Binding, Channel)};

            if (Track == nullptr) {
                return nullptr;
            }

            if (Channel >= FirstPropertyChannel) {
                return &static_cast<Asset::FSequencePropertyTrack*>(Track)->GetChannel();
            }

            return &static_cast<Asset::FSequenceTransformTrack*>(Track)->FindOrAddEditSection().GetChannel(static_cast<std::size_t>(Channel));
        }

        /**
         * @brief 시퀀서 행에 표시할 채널 이름을 구합니다.
         * @param[in] Sequence 바인딩·트랙을 조회할 레벨 시퀀스입니다.
         * @param[in] Binding 조회할 시퀀스 바인딩입니다.
         * @param[in] Channel 바인딩 내부의 변환·프로퍼티 채널 인덱스입니다.
         * @return 변환 또는 프로퍼티 채널의 표시 이름입니다.
         */
        const char* GetChannelLabel(Asset::ULevelSequence* Sequence, Core::Int32 Binding, Core::Int32 Channel) {
            if (Channel >= FirstPropertyChannel) {
                const auto* Track{static_cast<Asset::FSequencePropertyTrack*>(FindTrack(Sequence, Binding, Channel))};

                return Track != nullptr ? Track->GetPropertyName().c_str() : "";
            }

            return Asset::FSequenceTransformSection::GetChannelName(static_cast<std::size_t>(Channel));
        }

        /**
         * @brief 지정 틱에 배치한 시퀀스 키를 찾습니다.
         * @param[in] Channel 바인딩 내부의 변환·프로퍼티 채널 인덱스입니다.
         * @param[in] Tick 조회할 시퀀스 틱입니다.
         * @return 지정 틱의 키이며 없으면 nullptr입니다.
         */
        const Asset::FSequenceKey* FindKey(const Asset::FSequenceChannel& Channel, Core::Int32 Tick) {
            for (const Asset::FSequenceKey& Key : Channel.GetKeys()) {
                if (Key.mTick == Tick) {
                    return &Key;
                }
            }

            return nullptr;
        }

        /**
         * @brief 바인딩 GUID에 대응하는 표시 이름을 찾습니다.
         * @param[in] Sequence 바인딩·트랙을 조회할 레벨 시퀀스입니다.
         * @param[in] Id 조회할 시퀀스 바인딩 GUID입니다.
         * @return 바인딩 표시 이름이며 찾지 못하면 Missing 문자열입니다.
         */
        const char* FindBindingName(const Asset::ULevelSequence* Sequence, const Core::FGuid& Id) {
            for (const Asset::FSequenceBinding& Binding : Sequence->GetBindings()) {
                if (Binding.GetId() == Id) {
                    return Binding.GetName().c_str();
                }
            }

            return "Missing";
        }

        /**
         * @brief 재생 시간을 시퀀스 해상도로 환산해 현재 틱을 구합니다.
         * @param[in] Sequence 바인딩·트랙을 조회할 레벨 시퀀스입니다.
         * @param[in] Player 재생 시간을 제공하는 시퀀스 플레이어입니다.
         * @return 현재 재생 틱을 표시 프레임 경계에 맞춘 틱입니다.
         */
        Core::Int32 FindCurrentTick(const Asset::ULevelSequence* Sequence, const Engine::FSequencePlayer& Player) {
            return Sequence->FrameToTick(Sequence->TickToFrame(Player.GetTick()));
        }
    }

    /**
     * @brief 패널을 구성하고 컨텍스트에 프리뷰 복원 콜백을 등록합니다.
     */
    Editor::FSequencerPanel::FSequencerPanel(Editor::FEditorContext* Context)
		: Editor::FEditorPanel{Context, "Sequencer", true} {
        Context->SetPreviewRestorer([this] {
            RestorePreview();
        });
        Context->SetSequencerOpener([this](CoreUObject::FObjectHandle Actor) {
            mActor = Actor;
            Open();
        });
        SetVisible(false);
    }

    /**
     * @brief 프리뷰 복원 콜백을 해제합니다.
     */
    Editor::FSequencerPanel::~FSequencerPanel() {
        GetEditorContext()->SetPreviewCamera({});
        GetEditorContext()->SetPreviewRestorer({});
        GetEditorContext()->SetSequencerOpener({});
    }

    /**
     * @brief 프리뷰를 끝내고 월드에 쓴 값을 원래대로 되돌립니다.
     */
    void Editor::FSequencerPanel::RestorePreview() {
        GetEditorContext()->SetPreviewCamera({});

        if (!mPreviewActive) {
            return;
        }

        mPlayer.Stop();
        mPlayer.RestoreState();
        mPreviewActive = false;
    }

    /**
     * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
     */
    void Editor::FSequencerPanel::DrawContent(float DeltaTime) {
        Editor::FEditorContext* Context{GetEditorContext()};

        if (!Context->CanEditWorld() || Context->GetProject() == nullptr) {
            ImGui::TextDisabled("The sequencer is available while editing a stopped world.");

            return;
        }

        Engine::ALevelSequenceActor* Actor{DrawActorPicker()};

        if (Actor == nullptr) {
            Context->SetPreviewCamera({});

            mPreviewActive = false;
            mBoundSequence = nullptr;
            mPlayer.Reset();

            return;
        }

        Asset::ULevelSequence* Sequence{DrawSequencePicker(Actor)};

        if (Sequence == nullptr) {
            RestorePreview();

            mBoundSequence = nullptr;
            mPlayer.Reset();

            return;
        }

        if (Sequence != mBoundSequence || mActor != mBoundActor) {
            RestorePreview();

            mSelection = {};
            mActiveBinding = 0;

            RebindPlayer(Actor, Sequence);
        }

        mPlayer.SetLooping(mLoop);

        if (mPreviewActive) {
            mPlayer.Advance(DeltaTime);
        }

        DrawToolbar(Actor, Sequence);
        DrawBindingBar(Actor, Sequence);

        const float EditorHeight{ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y};

        DrawTimeline(Sequence, std::max(ImGui::GetContentRegionAvail().y - EditorHeight, 120.0f));

        if (mSelection.mBinding == CameraCutRow) {
            DrawCameraCutEditor(Sequence);
        } else {
            DrawKeyEditor(Sequence);
        }

        const Engine::UCameraComponent* CutCamera{mPreviewActive && mPreviewCuts ? mPlayer.GetCutCamera() : nullptr};

        Context->SetPreviewCamera(CutCamera != nullptr ? CutCamera->GetHandle() : CoreUObject::FObjectHandle{});
    }

    /**
     * @brief 편집할 시퀀스 액터를 고르거나 만드는 위젯을 그립니다.
     */
    Engine::ALevelSequenceActor* Editor::FSequencerPanel::DrawActorPicker() {
        Editor::FEditorContext* Context{GetEditorContext()};
        Engine::UWorld* World{Context->GetWorld()};

        // 뷰포트나 아웃라이너에서 시퀀스 액터를 새로 고르면 패널의 편집 대상도 따라갑니다.
        if (Context->GetSelectionRevision() != mSelectionRevision) {
            mSelectionRevision = Context->GetSelectionRevision();

            if (auto* Selected{CoreUObject::Cast<Engine::ALevelSequenceActor>(Context->GetSelectedActor())}; Selected != nullptr) {
                mActor = Selected->GetHandle();
            }
        }

        auto* Actor{CoreUObject::Cast<Engine::ALevelSequenceActor>(CoreUObject::FObjectRegistry::Get()->Resolve(mActor))};

        if (Actor != nullptr && (Actor->IsPendingDestroy() || !World->ContainsObject(Actor))) {
            Actor = nullptr;
        }

        ImGui::SetNextItemWidth(220.0f * ImGui::GetFontSize() / 16.0f);

        if (ImGui::BeginCombo("Actor", Actor != nullptr ? Actor->GetName().c_str() : "None")) {
            for (const auto& Candidate : World->GetPersistentLevel()->GetActors()) {
                auto* SequenceActor{CoreUObject::Cast<Engine::ALevelSequenceActor>(Candidate.get())};

                if (SequenceActor != nullptr && !SequenceActor->IsPendingDestroy() && ImGui::Selectable(SequenceActor->GetName().c_str(), SequenceActor == Actor)) {
                    Actor = SequenceActor;
                    mActor = Actor->GetHandle();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();

        if (ImGui::Button("New Actor")) {
            if (auto* Created{CoreUObject::Cast<Engine::ALevelSequenceActor>(Context->SpawnActorPreset("ALevelSequenceActor"))}; Created != nullptr) {
                Actor = Created;
                mActor = Actor->GetHandle();
            }
        }

        return Actor;
    }

    /**
     * @brief 액터가 재생할 시퀀스 에셋을 고르거나 만드는 위젯을 그립니다.
     */
    Asset::ULevelSequence* Editor::FSequencerPanel::DrawSequencePicker(Engine::ALevelSequenceActor* Actor) {
        Editor::FEditorContext* Context{GetEditorContext()};
        Asset::FAssetManager* Assets{Context->GetProject()->GetAssets()};
        const CoreUObject::FProperty* Property{Actor->GetClass()->FindClassProperty("Sequence")};
        const Core::FString& Path{Actor->GetSequencePath()};
        Core::FString Error{};

        ImGui::SameLine();
        ImGui::SetNextItemWidth(260.0f * ImGui::GetFontSize() / 16.0f);

        if (ImGui::BeginCombo("Sequence", Path.empty() ? "None" : Path.c_str())) {
            if (ImGui::IsWindowAppearing()) {
                mSequencePaths.clear();

                for (const std::filesystem::path& Candidate : Assets->GetAssetPaths(Error)) {
                    Asset::EAssetType Type{};
                    Core::FString TypeError{};

                    if (Asset::FAssetFile::GetType(Assets->GetContentRoot() / Candidate, Type, TypeError) && Type == Asset::EAssetType::Sequence) {
                        mSequencePaths.push_back(Core::FilePath::ToGenericUtf8(Candidate));
                    }
                }
            }

            for (const Core::FString& Candidate : mSequencePaths) {
                if (ImGui::Selectable(Candidate.c_str(), Candidate == Path)) {
                    Context->EditProperty(Actor->GetHandle(), Property, CoreUObject::FPropertyValue{Candidate});
                }
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();

        if (ImGui::Button("New Sequence")) {
            std::filesystem::path Created{};

            if (Assets->CreateSequence(Actor->GetName(), Created, Error) != nullptr) {
                Context->EditProperty(Actor->GetHandle(), Property, CoreUObject::FPropertyValue{Core::FilePath::ToGenericUtf8(Created)});
            } else {
                Context->AddLog(Error);
            }
        }

        if (Path.empty()) {
            ImGui::TextDisabled("Create or choose a sequence asset.");

            return nullptr;
        }

        if (Path == mFailedPath) {
            ImGui::TextDisabled("Cannot load the sequence asset.");

            return nullptr;
        }

        Asset::ULevelSequence* Sequence{Assets->LoadSequence(Core::FilePath::FromUtf8(Path), Error)};

        if (Sequence == nullptr) {
            mFailedPath = Path;

            Context->AddLog("Cannot load sequence '" + Path + "': " + Error);
        }

        return Sequence;
    }

    /**
     * @brief 재생 컨트롤과 시퀀스 설정 위젯을 그립니다.
     */
    void Editor::FSequencerPanel::DrawToolbar(Engine::ALevelSequenceActor* Actor, Asset::ULevelSequence* Sequence) {
        if (Editor::EditorStyle::IconButton("SequenceJumpStart", Editor::EditorStyle::EIcon::JumpStart, false, true, false, "Go to start")) {
            SeekPreview(Sequence->GetStartTick());
        }

        ImGui::SameLine();

        const bool Playing{mPreviewActive && mPlayer.GetState() == Engine::ESequencePlayState::Playing};

        if (Editor::EditorStyle::IconButton("SequencePlayPause", Playing ? Editor::EditorStyle::EIcon::Pause : Editor::EditorStyle::EIcon::Play, false, true, false, Playing ? "Pause" : "Play")) {
            if (Playing) {
                mPlayer.Pause();
            } else {
                mPlayer.Play();
                mPreviewActive = true;
            }
        }

        ImGui::SameLine();

        if (Editor::EditorStyle::IconButton("SequenceStop", Editor::EditorStyle::EIcon::Stop, false, true, false, "Stop and restore")) {
            RestorePreview();
            RebindPlayer(Actor, Sequence);
        }

        ImGui::SameLine();

        if (Editor::EditorStyle::IconButton("SequenceJumpEnd", Editor::EditorStyle::EIcon::JumpEnd, false, true, false, "Go to end")) {
            SeekPreview(Sequence->GetEndTick());
        }

        ImGui::SameLine();
        ImGui::Checkbox("Loop", &mLoop);
        ImGui::SameLine();
        ImGui::Checkbox("Preview Cuts", &mPreviewCuts);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f * ImGui::GetFontSize() / 16.0f);

        Core::Int32 Frame{Sequence->TickToFrame(mPlayer.GetTick())};

        if (ImGui::InputInt("Frame", &Frame)) {
            SeekPreview(Sequence->FrameToTick(Frame));
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f * ImGui::GetFontSize() / 16.0f);

        Core::Int32 EndFrame{Sequence->TickToFrame(Sequence->GetEndTick())};

        if (ImGui::InputInt("End", &EndFrame)) {
            Sequence->SetEndTick(Sequence->FrameToTick(std::clamp(EndFrame, 1, MaximumEndFrame)));

            // 재생 구간을 늘리면 섹션도 함께 늘려 새 구간에서 값이 계속 평가되게 합니다.
            for (Asset::FSequenceBinding& Binding : Sequence->GetBindings()) {
                if (Asset::FSequenceTransformTrack* Track{Binding.FindTransformTrack()}; Track != nullptr) {
                    Track->ExpandSections(Sequence->GetEndTick());
                }
            }

            NotifySequenceChanged();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(70.0f * ImGui::GetFontSize() / 16.0f);

        if (ImGui::BeginCombo("FPS", std::to_string(Sequence->GetDisplayRate()).c_str())) {
            for (const Core::Int32 Rate : DisplayRates) {
                if (ImGui::Selectable(std::to_string(Rate).c_str(), Rate == Sequence->GetDisplayRate()) && Sequence->SetDisplayRate(Rate)) {
                    mSequenceDirty = true;
                }
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();

        if (ImGui::Button(mSequenceDirty ? "Save*" : "Save")) {
            Core::FString Error{};
            Editor::FEditorContext* Context{GetEditorContext()};

            if (Context->GetProject()->GetAssets()->SaveSequence(Core::FilePath::FromUtf8(Actor->GetSequencePath()), *Sequence, Error)) {
                mSequenceDirty = false;

                Context->AddLog("Saved " + Actor->GetSequencePath());
            } else {
                Context->AddLog(Error);
            }
        }
    }

    /**
     * @brief 바인딩을 고르고 추가·제거하며 키를 찍는 위젯을 그립니다.
     */
    void Editor::FSequencerPanel::DrawBindingBar(Engine::ALevelSequenceActor* Actor, Asset::ULevelSequence* Sequence) {
        Editor::FEditorContext* Context{GetEditorContext()};
        auto& Bindings{Sequence->GetBindings()};
        Engine::AActor* Selected{Context->GetSelectedActor()};
        const bool CanAdd{Selected != nullptr && Selected != Actor};

        ImGui::BeginDisabled(!CanAdd);

        if (ImGui::Button("Add Selected Actor") && CanAdd) {
            bool Bound{};

            for (const Engine::FSequenceBindingTarget& Target : Actor->GetBindingTargets()) {
                Bound = Bound || Target.mObject.GetGuid() == Selected->GetGuid();
            }

            if (!Bound) {
                Asset::FSequenceBinding& Binding{Bindings.emplace_back()};
                Binding.SetName(Selected->GetName());
                Actor->SetBindingTarget(Binding.GetId(), Selected->GetGuid());
                Context->MarkSceneDirty();

                mActiveBinding = static_cast<Core::Int32>(Bindings.size()) - 1;
                mSequenceDirty = true;

                RebindPlayer(Actor, Sequence);
            }
        }

        ImGui::EndDisabled();

        if (Bindings.empty()) {
            ImGui::SameLine();
            ImGui::TextDisabled("Select an actor in the viewport and add it to the sequence.");

            return;
        }

        mActiveBinding = std::clamp(mActiveBinding, 0, static_cast<Core::Int32>(Bindings.size()) - 1);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(180.0f * ImGui::GetFontSize() / 16.0f);

        if (ImGui::BeginCombo("Binding", Bindings[mActiveBinding].GetName().c_str())) {
            for (Core::Int32 Index{}; Index < static_cast<Core::Int32>(Bindings.size()); ++Index) {
                ImGui::PushID(Index);

                if (ImGui::Selectable(Bindings[Index].GetName().c_str(), Index == mActiveBinding)) {
                    mActiveBinding = Index;
                }

                ImGui::PopID();
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();

        if (ImGui::Button("Remove Binding")) {
            const Core::FGuid Id{Bindings[mActiveBinding].GetId()};

            Actor->RemoveBindingTarget(Id);
            Context->MarkSceneDirty();

            // 삭제되는 바인딩이 쓴 값도 되돌린 뒤 남은 바인딩으로 프리뷰를 다시 연결합니다.
            mPlayer.RestoreState();
            Sequence->RemoveCameraCuts(Id);
            Bindings.erase(Bindings.begin() + mActiveBinding);

            mSelection = {};
            mSequenceDirty = true;

            RebindPlayer(Actor, Sequence);

            return;
        }

        const Asset::FSequenceBinding& ActiveBinding{Bindings[mActiveBinding]};
        Engine::USceneComponent* Component{mPlayer.FindBoundComponent(ActiveBinding.GetId())};

        if (Component == nullptr) {
            ImGui::SameLine();
            ImGui::TextDisabled("(target missing)");

            return;
        }

        if (ImGui::Button("Key Transform")) {
            AddTransformKeys(Sequence);
        }

        ImGui::SameLine();
        ImGui::BeginDisabled(FindPropertyTrack(ActiveBinding, 0) == nullptr);

        if (ImGui::Button("Key Properties")) {
            AddPropertyKeys(Sequence);
        }

        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f * ImGui::GetFontSize() / 16.0f);

        if (ImGui::BeginCombo("##AddProperty", "Add Property Track")) {
            for (const CoreUObject::FProperty* Property : Component->GetClass()->GetAllProperties()) {
                if (Property->GetType() != CoreUObject::EPropertyType::Float || !Property->HasFlag(Core::EPropertyFlags::Edit) || Property->HasFlag(Core::EPropertyFlags::ReadOnly)) {
                    continue;
                }

                if (ImGui::Selectable(Property->GetName().c_str())) {
                    Bindings[mActiveBinding].FindOrAddPropertyTrack(Property->GetName());
                    AddPropertyKeys(Sequence);
                }
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();

        const bool IsCamera{CoreUObject::Cast<Engine::UCameraComponent>(Component) != nullptr};

        ImGui::BeginDisabled(!IsCamera);

        if (ImGui::Button("Add Camera Cut")) {
            const Core::Int32 Tick{FindCurrentTick(Sequence, mPlayer)};

            Sequence->AddCameraCut(Tick, ActiveBinding.GetId());

            mSelection.mBinding = CameraCutRow;
            mSelection.mChannel = 0;
            mSelection.mTick = Tick;
            mPreviewActive = true;

            NotifySequenceChanged();
        }

        ImGui::EndDisabled();
    }

    /**
     * @brief 바인딩별 채널의 키를 타임라인으로 그리고 스크럽과 드래그를 처리합니다.
     */
    void Editor::FSequencerPanel::DrawTimeline(Asset::ULevelSequence* Sequence, float Height) {
        auto& Bindings{Sequence->GetBindings()};
        const Core::Int32 CurrentFrame{Sequence->TickToFrame(mPlayer.GetTick())};
        Core::Int32 Frame{CurrentFrame};
        Core::Int32 StartFrame{Sequence->TickToFrame(Sequence->GetStartTick())};
        Core::Int32 EndFrame{Sequence->TickToFrame(Sequence->GetEndTick())};

        // 위젯은 남은 영역 전체를 내부 Child로 차지하고 지정 높이를 넘는 줄에는 배경을 그리지 않습니다.
        // 높이를 지정하지 않아 배경이 모든 줄을 덮게 하고, 바깥 Child로 영역을 제한해 스크롤합니다.
        ImGui::BeginChild("##SequenceTimelineArea", ImVec2{0.0f, Height});

        if (ImGui::BeginNeoSequencer("##SequenceTimeline", &Frame, &StartFrame, &EndFrame, ImVec2{}, ImGuiNeoSequencerFlags_AlwaysShowHeader)) {
            DrawCameraCutRow(Sequence, EndFrame);

            for (Core::Int32 BindingIndex{}; BindingIndex < static_cast<Core::Int32>(Bindings.size()); ++BindingIndex) {
                Asset::FSequenceBinding& Binding{Bindings[BindingIndex]};
                Asset::FSequenceTransformTrack* Track{Binding.FindTransformTrack()};
                bool& Open{mOpenBindings.try_emplace(Binding.GetId().ToString(), true).first->second};

                ImGui::PushID(BindingIndex);

                if (ImGui::BeginNeoGroup(Binding.GetName().c_str(), &Open)) {
                    for (std::size_t ChannelIndex{}; Track != nullptr && ChannelIndex < Asset::FSequenceTransformSection::ChannelCount; ++ChannelIndex) {
                        DrawChannelRow(Sequence, BindingIndex, static_cast<Core::Int32>(ChannelIndex), Asset::FSequenceTransformSection::GetChannelName(ChannelIndex), Track->FindOrAddEditSection().GetChannel(ChannelIndex));
                    }

                    for (Core::Int32 PropertyIndex{}; const Asset::FSequencePropertyTrack* PropertyTrack{FindPropertyTrack(Binding, PropertyIndex)}; ++PropertyIndex) {
                        DrawChannelRow(Sequence, BindingIndex, FirstPropertyChannel + PropertyIndex, PropertyTrack->GetPropertyName().c_str(), PropertyTrack->GetChannel());
                    }

                    ImGui::EndNeoGroup();
                }

                ImGui::PopID();
            }

            ImGui::EndNeoSequencer();
        }

        ImGui::EndChild();

        if (Frame != CurrentFrame) {
            SeekPreview(Sequence->FrameToTick(Frame));
        }

        // 키 배열을 순회하는 동안 순서가 바뀌지 않도록 드래그 결과는 타임라인을 다 그린 뒤 적용합니다.
        if (mDragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            mDragging = false;
        }

        if (mDragging) {
            const Core::Int32 Offset{static_cast<Core::Int32>(std::lround((ImGui::GetMousePos().x - mDragStartMouseX) / mDragFrameWidth))};
            const Core::Int32 TargetTick{Sequence->FrameToTick(std::clamp(mDragStartFrame + Offset, StartFrame, EndFrame))};
            Asset::FSequenceChannel* Channel{mSelection.mBinding != CameraCutRow ? FindChannel(Sequence, mSelection.mBinding, mSelection.mChannel) : nullptr};

            if (TargetTick != mSelection.mTick && (mSelection.mBinding == CameraCutRow ? Sequence->MoveCameraCut(mSelection.mTick, TargetTick) : Channel != nullptr && Channel->MoveKey(mSelection.mTick, TargetTick))) {
                mSelection.mTick = TargetTick;

                NotifySequenceChanged();
            }
        }
    }

    /**
     * @brief 카메라 컷 구간과 시작 마커를 타임라인 한 줄로 그립니다.
     */
    void Editor::FSequencerPanel::DrawCameraCutRow(Asset::ULevelSequence* Sequence, Core::Int32 EndFrame) {
        if (!ImGui::BeginNeoTimelineEx("Camera Cuts")) {
            return;
        }

        const auto& Cuts{Sequence->GetCameraCuts()};

        for (std::size_t Index{}; Index < Cuts.size(); ++Index) {
            const Core::Int32 From{Sequence->TickToFrame(Cuts[Index].mTick)};
            const Core::Int32 To{Index + 1 < Cuts.size() ? Sequence->TickToFrame(Cuts[Index + 1].mTick) : EndFrame};

            ImGui::NeoDrawTimelineRect(From, To, Index % 2 == 0 ? ImVec4{0.20f, 0.50f, 0.80f, 0.35f} : ImVec4{0.80f, 0.50f, 0.20f, 0.35f});
        }

        for (std::size_t Index{}; Index < Cuts.size(); ++Index) {
            if (DrawKeyMarker(Sequence, CameraCutRow, 0, Cuts[Index].mTick, static_cast<Core::Int32>(Index))) {
                ImGui::SetTooltip("%s", FindBindingName(Sequence, Cuts[Index].mCameraBinding));
            }
        }

        ImGui::EndNeoTimeLine();
    }

    /**
     * @brief 채널의 키를 타임라인 한 줄로 그립니다.
     */
    void Editor::FSequencerPanel::DrawChannelRow(const Asset::ULevelSequence* Sequence, Core::Int32 Binding, Core::Int32 ChannelId, const char* Label, const Asset::FSequenceChannel& Channel) {
        ImGui::PushID(ChannelId);

        if (ImGui::BeginNeoTimelineEx(Label)) {
            if (ImGui::IsNeoTimelineSelected()) {
                mActiveBinding = Binding;
            }

            const auto& Keys{Channel.GetKeys()};

            for (std::size_t Index{}; Index < Keys.size(); ++Index) {
                DrawKeyMarker(Sequence, Binding, ChannelId, Keys[Index].mTick, static_cast<Core::Int32>(Index));
            }

            ImGui::EndNeoTimeLine();
        }

        ImGui::PopID();
    }

    /**
     * @brief 키 마커를 그리고 클릭하면 선택과 드래그를 시작합니다.
     */
    bool Editor::FSequencerPanel::DrawKeyMarker(const Asset::ULevelSequence* Sequence, Core::Int32 Binding, Core::Int32 ChannelId, Core::Int32 Tick, Core::Int32 Index) {
        const bool IsSelected{mSelection.mBinding == Binding && mSelection.mChannel == ChannelId && mSelection.mTick == Tick};
        Core::Int32 KeyFrame{Sequence->TickToFrame(Tick)};

        if (IsSelected) {
            ImGui::PushNeoSequencerStyleColor(ImGuiNeoSequencerCol_Keyframe, IM_COL32(255, 190, 60, 255));
        }

        ImGui::PushID(Index);
        ImGui::NeoKeyframe(&KeyFrame);

        const bool Hovered{ImGui::IsNeoKeyframeHovered()};

        if (Hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            mSelection.mBinding = Binding;
            mSelection.mChannel = ChannelId;
            mSelection.mTick = Tick;
            mDragging = true;
            mDragStartFrame = KeyFrame;
            mDragStartMouseX = ImGui::GetMousePos().x;
            mDragFrameWidth = std::max(ImGui::NeoGetFrameWidth(), 0.001f);

            if (Binding != CameraCutRow) {
                mActiveBinding = Binding;
            }
        }

        ImGui::PopID();

        if (IsSelected) {
            ImGui::PopNeoSequencerStyleColor();
        }

        return Hovered;
    }

    /**
     * @brief 선택한 키의 값과 보간 방식을 편집하는 위젯을 그립니다.
     */
    void Editor::FSequencerPanel::DrawKeyEditor(Asset::ULevelSequence* Sequence) {
        Asset::FSequenceChannel* Channel{FindChannel(Sequence, mSelection.mBinding, mSelection.mChannel)};
        const Asset::FSequenceKey* Selected{Channel != nullptr ? FindKey(*Channel, mSelection.mTick) : nullptr};

        if (Selected == nullptr) {
            return;
        }

        Asset::FSequenceKey Key{*Selected};
        bool Changed{};

        ImGui::Text("%s  Frame %d", GetChannelLabel(Sequence, mSelection.mBinding, mSelection.mChannel), Sequence->TickToFrame(Key.mTick));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110.0f * ImGui::GetFontSize() / 16.0f);

        Changed = ImGui::DragFloat("Value", &Key.mValue, 0.05f, 0.0f, 0.0f, "%.3f");

        ImGui::SameLine();
        ImGui::SetNextItemWidth(110.0f * ImGui::GetFontSize() / 16.0f);

        int Interpolation{static_cast<int>(Key.mInterpolation)};

        if (ImGui::Combo("Interpolation", &Interpolation, InterpolationNames.data(), static_cast<int>(InterpolationNames.size()))) {
            Key.mInterpolation = static_cast<Asset::ESequenceInterpolation>(Interpolation);
            Changed = true;
        }

        if (Changed && Channel->AddKey(Key)) {
            NotifySequenceChanged();
        }

        ImGui::SameLine();

        if (ImGui::Button("Delete Key") && Channel->RemoveKey(Key.mTick)) {
            mSelection = {};
            mDragging = false;

            NotifySequenceChanged();

            return;
        }

        ImGui::SameLine();

        if (ImGui::Button("Remove Track")) {
            // 트랙이 쓴 값을 되돌린 뒤 제거해야 원래 값이 월드에 남습니다.
            mPlayer.RestoreState();
            Sequence->GetBindings()[mSelection.mBinding].RemoveTrack(FindTrack(Sequence, mSelection.mBinding, mSelection.mChannel));

            mSelection = {};
            mDragging = false;

            NotifySequenceChanged();
        }
    }

    /**
     * @brief 선택한 카메라 컷의 카메라를 바꾸거나 컷을 지우는 위젯을 그립니다.
     */
    void Editor::FSequencerPanel::DrawCameraCutEditor(Asset::ULevelSequence* Sequence) {
        const auto& Cuts{Sequence->GetCameraCuts()};
        const auto Cut{std::find_if(Cuts.begin(), Cuts.end(), [this](const Asset::FSequenceCameraCut& Candidate) {
            return Candidate.mTick == mSelection.mTick;
        })};

        if (Cut == Cuts.end()) {
            return;
        }

        const Core::FGuid Camera{Cut->mCameraBinding};

        ImGui::Text("Camera Cut  Frame %d", Sequence->TickToFrame(mSelection.mTick));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(180.0f * ImGui::GetFontSize() / 16.0f);

        if (ImGui::BeginCombo("Camera", FindBindingName(Sequence, Camera))) {
            for (const Asset::FSequenceBinding& Binding : Sequence->GetBindings()) {
                if (CoreUObject::Cast<Engine::UCameraComponent>(mPlayer.FindBoundComponent(Binding.GetId())) != nullptr && ImGui::Selectable(Binding.GetName().c_str(), Binding.GetId() == Camera)) {
                    Sequence->AddCameraCut(mSelection.mTick, Binding.GetId());
                    NotifySequenceChanged();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();

        if (ImGui::Button("Delete Cut")) {
            Sequence->RemoveCameraCut(mSelection.mTick);

            mSelection = {};
            mDragging = false;

            NotifySequenceChanged();
        }
    }

    /**
     * @brief 선택한 바인딩의 현재 변환으로 현재 프레임에 키를 찍습니다.
     */
    void Editor::FSequencerPanel::AddTransformKeys(Asset::ULevelSequence* Sequence) {
        Asset::FSequenceBinding& Binding{Sequence->GetBindings()[mActiveBinding]};
        const Engine::USceneComponent* Component{mPlayer.FindBoundComponent(Binding.GetId())};

        if (Component == nullptr) {
            GetEditorContext()->AddLog("Cannot key a sequence binding without a target: " + Binding.GetName());

            return;
        }

        const Core::FTransform& Transform{Component->GetTransform()};
        const Core::FRotator Rotation{Transform.mRotation.Rotator()};
        const std::array<float, Asset::FSequenceTransformSection::ChannelCount> Values{Transform.mLocation.x, Transform.mLocation.y, Transform.mLocation.z, Rotation.pitch, Rotation.yaw, Rotation.roll, Transform.mScale.x, Transform.mScale.y, Transform.mScale.z};
        const Core::Int32 Tick{FindCurrentTick(Sequence, mPlayer)};
        Asset::FSequenceTransformSection& Section{Binding.FindOrAddTransformTrack().FindOrAddEditSection()};
        Section.ExpandToInclude(Sequence->GetStartTick());
        Section.ExpandToInclude(Sequence->GetEndTick());

        for (std::size_t Index{}; Index < Values.size(); ++Index) {
            Asset::FSequenceKey Key{};
            Key.mTick = Tick;
            Key.mValue = Values[Index];

            Section.GetChannel(Index).AddKey(Key);
        }

        mPreviewActive = true;

        NotifySequenceChanged();
    }

    /**
     * @brief 선택한 바인딩의 현재 프로퍼티 값으로 현재 프레임에 키를 찍습니다.
     */
    void Editor::FSequencerPanel::AddPropertyKeys(Asset::ULevelSequence* Sequence) {
        const Asset::FSequenceBinding& Binding{Sequence->GetBindings()[mActiveBinding]};
        const Engine::USceneComponent* Component{mPlayer.FindBoundComponent(Binding.GetId())};

        if (Component == nullptr) {
            return;
        }

        const Core::Int32 Tick{FindCurrentTick(Sequence, mPlayer)};

        for (Core::Int32 PropertyIndex{}; Asset::FSequencePropertyTrack* Track{FindPropertyTrack(Binding, PropertyIndex)}; ++PropertyIndex) {
            const CoreUObject::FProperty* Property{Component->GetClass()->FindClassProperty(Track->GetPropertyName())};

            if (Property != nullptr && Property->GetType() == CoreUObject::EPropertyType::Float) {
                Asset::FSequenceKey Key{};
                Key.mTick = Tick;
                Key.mValue = std::get<float>(Property->ReadValue(Component));

                Track->GetChannel().AddKey(Key);
            }
        }

        mPreviewActive = true;

        NotifySequenceChanged();
    }

    /**
     * @brief 현재 복원 상태를 정리하고 플레이어를 시퀀스와 액터에 다시 연결합니다.
     */
    void Editor::FSequencerPanel::RebindPlayer(Engine::ALevelSequenceActor* Actor, const Asset::ULevelSequence* Sequence) {
        const double Tick{mPlayer.GetTick()};

        mPlayer.RestoreState();
        mPlayer.Initialize(Sequence, GetEditorContext()->GetWorld(), Actor->GetBindingTargets());
        mBoundSequence = Sequence;
        mBoundActor = mActor;

        if (mPreviewActive) {
            mPlayer.Seek(Tick);
        }
    }

    /**
     * @brief 지정한 시간으로 이동하여 프리뷰를 켭니다.
     */
    void Editor::FSequencerPanel::SeekPreview(double Tick) {
        mPlayer.Seek(Tick);
        mPreviewActive = true;
    }

    /**
     * @brief 시퀀스 변경을 표시하고 프리뷰 중이면 현재 시간의 값을 다시 적용합니다.
     */
    void Editor::FSequencerPanel::NotifySequenceChanged() {
        mSequenceDirty = true;

        if (mPreviewActive) {
            mPlayer.Seek(mPlayer.GetTick());
        }
    }

}
