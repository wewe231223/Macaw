/**
 * @file FDetailsPanel.cpp
 * @brief 선택한 객체의 리플렉션 프로퍼티를 표시하고 편집합니다.
 * @ingroup Editor
 */

#include "Editor/Panel/FDetailsPanel.h"
#include "Editor/FEditorContext.h"
#include "Editor/FAssetDragDrop.h"
#include "Engine/AActor.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/Component/UMeshComponent.h"
#include "Engine/Component/ULightComponent.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Engine/Animation/UAnimInstance.h"
#include "Engine/Animation/UAnimGraphInstance.h"
#include "RenderCore/IRendererModule.h"
#include "Engine/FProject.h"
#include "Asset/FAssetManager.h"
#include "Asset/FAssetFile.h"
#include "Asset/UStaticMesh.h"
#include "Core/FilePath.h"
#include "CoreUObject/TObjectIterator.h"
#include "Editor/FEditorAssetCatalog.h"
#include "Editor/FAssetThumbnailCache.h"
#include "CoreUObject/UObject.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/NativeReflection.h"
#include "Editor/FScriptBuildService.h"
#include "Script/FScriptField.h"
#include "Script/FScriptRuntime.h"
#include "Script/UScriptComponent.h"
#include "Serialization/JsonArchive.h"
#include "EditorStyle.h"
#include "Panel/ObjectRow.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>

namespace Editor {

    /**
     * @brief Details 패널에서 사용하는 컴포넌트 표시 이름 변환입니다.
     */
    namespace {
        Core::FString DisplayName(std::string_view Name) {
            Core::FString Label{};

            for (std::size_t Index{}; Index < Name.size(); ++Index) {
                const auto Character{static_cast<unsigned char>(Name[Index])};

                if (Index > 0 && std::isupper(Character) && (std::islower(static_cast<unsigned char>(Name[Index - 1])) || (Index + 1 < Name.size() && std::islower(static_cast<unsigned char>(Name[Index + 1]))))) {
                    Label.push_back(' ');
                }

                Label.push_back(Name[Index]);
            }

            return Label;
        }

        std::string_view DetailCategory(const CoreUObject::UClass* Owner, std::string_view Name) {
            if (Name == "Transform") {
                return "Transform";
            }

            if (Name == "Material") {
                return "Materials";
            }

            if (Name == "Mesh" || Name == "SkeletalMesh" || Name == "Skeleton" || Name == "PrimitiveType") {
                return "Mesh";
            }

            if (Name == "AnimClass" || Name == "Animation" || Name == "AnimGraph" || (Owner == Engine::USkeletalMeshComponent::StaticClass() && (Name == "Looping" || Name == "PlayRate"))) {
                return "Animation";
            }

            if (Name == "EditorTickable") {
                return "Component Tick";
            }

            if (Owner->IsChildOf(Engine::ULightComponent::StaticClass()) || Owner->GetName() == "ULightComponentBase") {
                return "Light";
            }

            if (Name == "Visible" || Name == "CastShadow" || Name == "ReceiveShadow" || Name == "ReceivesDecals") {
                return "Rendering";
            }

            const std::string_view ClassName{Owner->GetName()};

            if (ClassName == "UCameraComponent") {
                return "Camera";
            }

            if (ClassName == "UHeightFogComponent") {
                return "Fog";
            }

            if (ClassName == "UShapeComponent" || ClassName == "UBoxComponent" || ClassName == "USphereComponent" || ClassName == "UCapsuleComponent") {
                return "Shape";
            }

            if (ClassName == "UProjectileMovementComponent" || ClassName == "URotationMovementComponent") {
                return "Movement";
            }

            return Owner == CoreUObject::UObject::StaticClass() || Owner == Engine::AActor::StaticClass() || Owner == Engine::UActorComponent::StaticClass() ? "General" : "Settings";
        }

        Core::FString PropertyLabel(const CoreUObject::FProperty* Property) {
            const Core::FString& Name{Property->GetName()};

            if (Name == "ShadowBiasPreset") {
                return "Depth Bias Preset";
            }

            if (Name == "ShadowSlopeBiasPreset") {
                return "Slope-Scaled Bias Preset";
            }

            if (Name == "Animation") {
                return "Anim to Play";
            }

            if (Name == "Material") {
                return "All Slots Override";
            }

            return DisplayName(Name);
        }

        bool IsAdvancedProperty(std::string_view Name) {
            return Name == "EditorTickable" || Name.starts_with("Shadow") || Name == "ReceivesDecals";
        }

        Engine::AActor* GetOwningActor(CoreUObject::UObject* Object) {
            if (auto* Actor{CoreUObject::Cast<Engine::AActor>(Object)}) {
                return Actor;
            }

            auto* Component{CoreUObject::Cast<Engine::UActorComponent>(Object)};

            return Component != nullptr ? Component->GetOwner() : nullptr;
        }

        bool EqualValues(const CoreUObject::FPropertyValue& First, const CoreUObject::FPropertyValue& Second) {
            return std::visit([&Second](const auto& Value) {
                using FValue = std::decay_t<decltype(Value)>;
                const auto* Other{std::get_if<FValue>(&Second)};

                if (Other == nullptr) {
                    return false;
                }

                if constexpr (std::is_same_v<FValue, Core::FTransform>) {
                    return Value.Equals(*Other, 0.0f);
                } else if constexpr (std::is_same_v<FValue, CoreUObject::FObjectReference>) {
                    return Value.GetGuid() == Other->GetGuid();
                } else if constexpr (requires { Value == *Other; }) {
                    return Value == *Other;
                } else {
                    return false;
                }
            }, First);
        }

        bool ArchiveDetailValue(Serialization::FJsonArchive& Archive, CoreUObject::FPropertyValue& Value) {
            if (Archive.IsLoading() && !Archive.HasField("Value")) {
                return false;
            }

            return std::visit([&Archive](auto& Field) {
                if constexpr (requires { Archive.Serialize("Value", Field); }) {
                    Archive.Serialize("Value", Field);

                    return !Archive.HasError();
                } else {
                    return false;
                }
            }, Value);
        }

        bool DrawResetButton() {
            return Editor::EditorStyle::IconButton("Reset to Default", Editor::EditorStyle::EIcon::Rotate);
        }

        bool BeginPropertyTable() {
            constexpr ImGuiTableFlags Flags{ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp};

            if (!ImGui::BeginTable("##Properties", 3, Flags)) {
                return false;
            }

            ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, 0.4f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.6f);
            ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, ImGui::GetFrameHeight());

            return true;
        }

        /**
         * @brief 클래스 접두사와 Component 접미사를 제거해 컴포넌트 메뉴 이름을 만듭니다.
         * @param[in] Class 생성하거나 이름을 표시할 리플렉션 클래스입니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::FString ComponentLabel(const CoreUObject::UClass* Class) {
            Core::FString Name{Class->GetName()};

            if (Name.starts_with("U")) {
                Name.erase(0, 1);
            }

            if (Name.ends_with("Component")) {
                Name.resize(Name.size() - 9);
            }

            return DisplayName(Name);
        }

        /**
         * @brief Details에 표시할 비일시적 편집 가능 또는 읽기 전용 프로퍼티인지 검사합니다.
         * @param[in] Property 유효한 리플렉션 프로퍼티의 비소유 주소입니다.
         * @return Transient가 아니고 Edit 또는 ReadOnly 플래그가 있으면 true입니다.
         */
        bool IsVisibleProperty(const CoreUObject::FProperty* Property) {
            return !Property->HasFlag(Core::EPropertyFlags::Transient) && (Property->HasFlag(Core::EPropertyFlags::Edit) || Property->HasFlag(Core::EPropertyFlags::ReadOnly));
        }

        Core::FString DrawFieldLabel(const char* Label, float TrailingWidth = 0.0f) {
            const float LabelEnd{ImGui::GetCursorPosX() + std::max(1.0f, ImGui::GetContentRegionAvail().x * 0.4f)};

            ImGui::AlignTextToFramePadding();
            ImGui::PushTextWrapPos(LabelEnd);
            ImGui::TextUnformatted(Label);
            ImGui::PopTextWrapPos();

            ImGui::SameLine();
            ImGui::SetCursorPosX(LabelEnd + ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::SetNextItemWidth(std::max(1.0f, ImGui::GetContentRegionAvail().x - TrailingWidth));

            return Core::FString{"##"} + Label;
        }

        // 숫자 변환은 Macaw.Engine의 ScriptFields와 같은 형식이어야 하므로 로캘과 무관한 charconv를 씁니다.

        /**
         * @brief 스크립트 프로퍼티의 편집 문자열을 실수로 해석합니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @return 문자열에서 해석한 실수입니다.
         */
        float ParseScriptFloat(std::string_view Text) {
            float Value{};

            std::from_chars(Text.data(), Text.data() + Text.size(), Value);

            return Value;
        }

        /**
         * @brief 스크립트 프로퍼티 실수를 편집 가능한 문자열로 변환합니다.
         * @param[in] Value 검사·변환·저장할 값입니다.
         * @return 편집할 수 있는 실수 문자열입니다.
         */
        Core::FString FormatScriptFloat(float Value) {
            std::array<char, 64> Buffer{};
            const auto Result{std::to_chars(Buffer.data(), Buffer.data() + Buffer.size(), Value)};

            return Core::FString{Buffer.data(), Result.ptr};
        }

        /**
         * @brief 타입 태그에 맞는 위젯으로 C# 필드 값을 표시합니다.
         * @param[in] Info 필드 정보입니다.
         * @param[in,out] Value 문자열로 변환된 값입니다.
         * @return 값이 바뀌면 true입니다.
         */
        bool DrawScriptFieldValue(const Script::FScriptFieldInfo& Info, const char* Label, Core::FString& Value) {
            if (Info.mType == "Bool") {
                bool Current{Value == "true"};

                if (ImGui::Checkbox(Label, &Current)) {
                    Value = Current ? "true" : "false";

                    return true;
                }
            } else if (Info.mType == "Int") {
                int Current{};

                std::from_chars(Value.data(), Value.data() + Value.size(), Current);

                if (ImGui::InputInt(Label, &Current)) {
                    Value = std::to_string(Current);

                    return true;
                }
            } else if (Info.mType == "Float") {
                float Current{ParseScriptFloat(Value)};

                if (ImGui::DragFloat(Label, &Current, 0.025f)) {
                    Value = FormatScriptFloat(Current);

                    return true;
                }
            } else if (Info.mType == "Vector3") {
                std::array<float, 3> Current{};
                std::string_view Rest{Value};

                for (float& Component : Current) {
                    const std::size_t Space{Rest.find(' ')};

                    Component = ParseScriptFloat(Rest.substr(0, Space));
                    Rest = Space != std::string_view::npos ? Rest.substr(Space + 1) : std::string_view{};
                }

                if (ImGui::DragFloat3(Label, Current.data(), 0.025f)) {
                    Value = FormatScriptFloat(Current[0]) + " " + FormatScriptFloat(Current[1]) + " " + FormatScriptFloat(Current[2]);

                    return true;
                }
            } else if (Info.mType == "String") {
                std::array<char, 512> Buffer{};

                std::copy_n(Value.begin(), std::min(Value.size(), Buffer.size() - 1), Buffer.begin());

                if (ImGui::InputText(Label, Buffer.data(), Buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    Value = Buffer.data();

                    return true;
                }
            }

            return false;
        }
    }

    /**
     * @brief FDetailsPanel의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Editor::FDetailsPanel::FDetailsPanel(Editor::FEditorContext* Context)
		: Editor::FEditorPanel{Context, "Details"} {
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Editor::FDetailsPanel::~FDetailsPanel() = default;

    /**
     * @brief 브라우저와 공유할 에셋 표시 서비스를 연결합니다.
     */
    void Editor::FDetailsPanel::SetAssetServices(Editor::FEditorAssetCatalog* Catalog, Editor::FAssetThumbnailCache* Thumbnails) {
        mAssetCatalog = Catalog;
        mThumbnails = Thumbnails;
    }

    void Editor::FDetailsPanel::SetAssetNavigation(std::function<std::filesystem::path()> Selection, std::function<void(const std::filesystem::path&)> Browse) {
        mGetSelectedAsset = std::move(Selection);
        mBrowseAsset = std::move(Browse);
    }

    /**
     * @brief C# 프로젝트 생성과 빌드 상태를 제공할 서비스를 연결합니다.
     */
    void Editor::FDetailsPanel::SetScriptBuildService(Editor::FScriptBuildService* Builds) {
        mScriptBuilds = Builds;
    }

    /**
     * @brief 스크립트 클래스를 고르는 콤보를 표시합니다.
     */
    bool Editor::FDetailsPanel::DrawScriptClassPicker(const char* Label, Core::FString& ClassName) {
        bool Changed{};

        if (ImGui::BeginCombo(Label, ClassName.empty() ? "None" : ClassName.c_str())) {
            if (ImGui::Selectable("None", ClassName.empty())) {
                ClassName.clear();

                Changed = true;
            }

            for (const Script::FScriptClassInfo& Class : Script::FScriptRuntime::Get()->GetCatalog()) {
                if (ImGui::Selectable(Class.mName.c_str(), Class.mName == ClassName)) {
                    ClassName = Class.mName;
                    Changed = true;
                }
            }

            ImGui::EndCombo();
        }

        return Changed;
    }

    void Editor::FDetailsPanel::SetAnimGraphOpener(std::function<void()> Opener) {
        mAnimGraphOpener = std::move(Opener);
    }

    void Editor::FDetailsPanel::DrawAnimInstance(const Core::TArray<CoreUObject::FObjectHandle>& Targets) {
        Editor::FEditorContext* Context{GetEditorContext()};
        Core::TArray<CoreUObject::FObjectHandle> Instances{};

        for (const CoreUObject::FObjectHandle Target : Targets) {
            const auto* Component{CoreUObject::Cast<Engine::USkeletalMeshComponent>(Context->ResolveObject(Target))};
            const auto* Instance{Component != nullptr ? CoreUObject::Cast<Engine::UAnimGraphInstance>(Component->GetAnimInstance()) : nullptr};

            if (Instance == nullptr) {
                return;
            }

            Instances.push_back(Instance->GetHandle());
        }

        const auto* GraphProperty{Engine::UAnimGraphInstance::StaticClass()->FindClassProperty("AnimGraph")};

        if (GraphProperty != nullptr && MatchesSearch("Animation", "Anim Graph", "AnimGraph")) {
            DrawPropertyRow({GraphProperty, std::move(Instances), "Animation", "Anim Graph"});
        }

        const CoreUObject::UObject* Selected{Context->GetSelectedObject()};

        if (Targets.size() == 1 && Selected != nullptr && Selected->GetHandle() == Targets.front() && mAnimGraphOpener && !mOnlyModified && MatchesSearch("Animation", "Debug Anim Graph")) {
            BeginPropertyRow("Debug");
            ImGui::TableSetColumnIndex(1);

            if (ImGui::Button("Open Anim Graph")) {
                mAnimGraphOpener();
            }
        }
    }

    /**
     * @brief C# 프로젝트 상태와 스크립트의 [Field] 필드를 표시합니다.
     */
    void Editor::FDetailsPanel::DrawScriptFields(Script::UScriptComponent* Component) {
        Editor::FEditorContext* Context{GetEditorContext()};
        const CoreUObject::FObjectHandle Handle{Component->GetHandle()};

        if (mScriptBuilds != nullptr && !mScriptBuilds->HasProject()) {
            ImGui::TextWrapped("이 프로젝트에는 C# 프로젝트가 없습니다. Scripts 메뉴의 \"C# 프로젝트 열기\"로 만들 수 있으며, .NET 10 SDK가 필요합니다. 만든 스크립트는 콘텐츠 브라우저의 Scripts 폴더에서 더블클릭해 엽니다.");

            return;
        }

        const Core::FString& ClassName{Component->GetScriptClass()};

        if (ClassName.empty()) {
            return;
        }

        const Script::FScriptClassInfo* Class{Script::FScriptRuntime::Get()->FindClass(ClassName)};

        if (Class == nullptr) {
            ImGui::TextColored({1.0f, 0.75f, 0.3f, 1.0f}, "빌드된 스크립트에 %s 클래스가 없습니다.", ClassName.c_str());
        }

        // 그리는 도중 값이 바뀔 수 있으므로 저장된 필드는 복사해서 순회합니다.
        const Core::TArray<Script::FScriptField> Saved{Component->GetFields()};
        const auto FindSaved{[&Saved](const Script::FScriptFieldInfo& Info) -> const Script::FScriptField* {
            const auto Position{std::find_if(Saved.begin(), Saved.end(), [&Info](const Script::FScriptField& Field) {
                return Field.mName == Info.mName && Field.mType == Info.mType;
            })};

            return Position != Saved.end() ? &*Position : nullptr;
        }};

        ImGui::BeginDisabled(!Context->CanEditWorld());

        if (Class != nullptr) {
            for (const Script::FScriptFieldInfo& Info : Class->mFields) {
                const Script::FScriptField* Field{FindSaved(Info)};

                if (!MatchesSearch("C# Script", Info.mName, Info.mType) || (mOnlyModified && Field == nullptr)) {
                    continue;
                }

                Core::FString Value{Field != nullptr ? Field->mValue : Info.mDefault};

                ImGui::PushID(Info.mName.c_str());

                if (Info.mType == "Actor") {
                    const CoreUObject::UObject* Target{Field != nullptr ? Field->mObject.Resolve() : nullptr};

                    if (ImGui::BeginCombo(DrawFieldLabel(Info.mName.c_str()).c_str(), Target != nullptr ? Target->GetName().c_str() : "(없음)")) {
                        if (ImGui::Selectable("(없음)", Target == nullptr)) {
                            Context->RemoveScriptField(Handle, Info.mName);
                        }

                        for (const CoreUObject::FObjectHandle ActorHandle : Context->GetActorHandles()) {
                            const CoreUObject::UObject* Actor{Context->ResolveObject(ActorHandle)};

                            if (Actor != nullptr && ImGui::Selectable(Actor->GetName().c_str(), Actor == Target)) {
                                Script::FScriptField Selected{Info.mName, Info.mType};
                                Selected.mObject = CoreUObject::FObjectReference{Actor};

                                Context->SetScriptField(Handle, Selected);
                            }
                        }

                        ImGui::EndCombo();
                    }

                    ImGui::PopID();

                    continue;
                }

                const ImGuiStyle& Style{ImGui::GetStyle()};
                const float ResetWidth{Field != nullptr ? ImGui::GetFrameHeight() + Style.ItemSpacing.x : 0.0f};
                const Core::FString InputLabel{DrawFieldLabel(Info.mName.c_str(), ResetWidth)};

                if (DrawScriptFieldValue(Info, InputLabel.c_str(), Value)) {
                    BeginItemSceneEdit();

                    if (Value == Info.mDefault) {
                        Context->RemoveScriptField(Handle, Info.mName);
                    } else {
                        Context->SetScriptField(Handle, {Info.mName, Info.mType, Value});
                    }
                }

                if (Field != nullptr) {
                    ImGui::SameLine();

                    if (DrawResetButton()) {
                        Context->RemoveScriptField(Handle, Info.mName);
                    }
                }

                ImGui::PopID();
            }
        }

        // 스크립트에서 이름이나 타입이 바뀐 값은 지우기 전까지 보존합니다.
        for (const Script::FScriptField& Field : Saved) {
            const bool Known{Class != nullptr && std::any_of(Class->mFields.begin(), Class->mFields.end(), [&Field](const Script::FScriptFieldInfo& Info) {
                return Info.mName == Field.mName && Info.mType == Field.mType;
            })};

            if (Known || !MatchesSearch("C# Script", Field.mName, Field.mType)) {
                continue;
            }

            ImGui::PushID(Field.mName.c_str());
            ImGui::TextColored({1.0f, 0.75f, 0.3f, 1.0f}, "%s (%s) = %s", Field.mName.c_str(), Field.mType.c_str(), Field.mValue.c_str());
            ImGui::SameLine();

            if (ImGui::SmallButton("지우기")) {
                Context->RemoveScriptField(Handle, Field.mName);
            }

            ImGui::PopID();
        }

        ImGui::EndDisabled();
    }

    /**
     * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
     */
    void Editor::FDetailsPanel::DrawContent(float) {
        Editor::FEditorContext* Context{GetEditorContext()};
        Core::TArray<CoreUObject::FObjectHandle> Targets{};

        if (mLockedContext == Context) {
            std::erase_if(mLockedTargets, [Context](CoreUObject::FObjectHandle Target) {
                return Context->ResolveObject(Target) == nullptr;
            });
        }

        if (mLockedContext != Context || mLockedTargets.empty()) {
            mLockedContext = nullptr;
            mLockedTargets.clear();
            Targets = Context->GetSelectedActors();

            if (Targets.size() < 2) {
                Targets.clear();

                if (const CoreUObject::UObject* Selected{Context->GetSelectedObject()}) {
                    Targets.push_back(Selected->GetHandle());
                }
            }
        } else {
            Targets = mLockedTargets;
        }

        auto* Selected{Targets.size() == 1 ? Context->ResolveObject(Targets.front()) : nullptr};
        auto* Actor{GetOwningActor(Selected)};

        if (Actor != nullptr) {
            DrawComponentMenu(mLockedContext == nullptr && Context->CanEditWorld());
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(Actor->GetName().c_str());
            DrawComponentTree(Actor, Selected->GetHandle());

            if (mLockedContext != nullptr) {
                Targets = mLockedTargets;
            } else if (const CoreUObject::UObject* Current{Context->GetSelectedObject()}) {
                Targets.front() = Current->GetHandle();
            }
        } else if (Targets.size() > 1) {
            ImGui::Text("%zu Actors Selected", Targets.size());
        }

        DrawToolbar(Targets);

        if (ImGui::BeginChild("##DetailProperties")) {
            if (Targets.empty()) {
                ImGui::TextDisabled("Select an actor or component to view its details.");
            } else {
                DrawReflectedObjects(Targets);
            }
        }

        ImGui::EndChild();
        mExpandCategories.reset();
    }

    void Editor::FDetailsPanel::DrawToolbar(Core::TArray<CoreUObject::FObjectHandle>& Targets) {
        const bool Locked{mLockedContext != nullptr};
        const ImGuiStyle& Style{ImGui::GetStyle()};
        const float Buttons{ImGui::CalcTextSize("Unlock").x + ImGui::CalcTextSize("Options").x + Style.FramePadding.x * 4.0f + Style.ItemSpacing.x * 2.0f};
        ImGui::SetNextItemWidth(std::max(24.0f, ImGui::GetContentRegionAvail().x - Buttons));
        ImGui::InputTextWithHint("##SearchDetails", "Search Details", mPropertySearch.data(), mPropertySearch.size(), ImGuiInputTextFlags_EscapeClearsAll);
        ImGui::SameLine();
        ImGui::BeginDisabled(Targets.empty());

        if (ImGui::Button(Locked ? "Unlock###LockDetails" : "Lock###LockDetails")) {
            mLockedContext = Locked ? nullptr : GetEditorContext();
            mLockedTargets = Locked ? Core::TArray<CoreUObject::FObjectHandle>{} : Targets;
        }

        ImGui::EndDisabled();
        ImGui::SetItemTooltip("Keep this selection in Details");
        ImGui::SameLine();

        if (ImGui::Button("Options")) {
            ImGui::OpenPopup("##DetailOptions");
        }

        if (ImGui::BeginPopup("##DetailOptions")) {
            ImGui::MenuItem("Show Only Modified Properties", nullptr, &mOnlyModified);
            ImGui::MenuItem("Show All Advanced Details", nullptr, &mShowAdvanced);
            ImGui::Separator();

            if (ImGui::MenuItem("Expand All Categories")) {
                mExpandCategories = true;
            }

            if (ImGui::MenuItem("Collapse All Categories")) {
                mExpandCategories = false;
            }

            ImGui::EndPopup();
        }

        ImGui::Separator();
    }

    bool Editor::FDetailsPanel::MatchesSearch(std::string_view Category, std::string_view Label, std::string_view Name) const {
        std::string_view Search{mPropertySearch.data()};

        while (!Search.empty()) {
            const std::size_t Start{Search.find_first_not_of(" \t")};

            if (Start == std::string_view::npos) {
                break;
            }

            Search.remove_prefix(Start);
            const std::size_t End{Search.find_first_of(" \t")};
            const std::string_view Term{Search.substr(0, End)};
            const auto Matches{[Term](std::string_view Text) {
                return !Text.empty() && ImStristr(Text.data(), Text.data() + Text.size(), Term.data(), Term.data() + Term.size()) != nullptr;
            }};

            if (!Matches(Category) && !Matches(Label) && !Matches(Name)) {
                return false;
            }

            if (End == std::string_view::npos) {
                break;
            }

            Search.remove_prefix(End);
        }

        return true;
    }

    bool Editor::FDetailsPanel::BeginCategory(const char* Label, bool DefaultOpen) const {
        const bool Searching{mPropertySearch.front() != '\0'};

        if (Searching) {
            ImGui::PushID("Search");
        }

        if (mExpandCategories.has_value()) {
            ImGui::SetNextItemOpen(*mExpandCategories, ImGuiCond_Always);
        }

        const ImGuiTreeNodeFlags Flags{DefaultOpen || Searching ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None};
        const bool Open{ImGui::CollapsingHeader(Label, Flags)};

        if (Searching) {
            ImGui::PopID();
        }

        return Open;
    }

    void Editor::FDetailsPanel::BeginPropertyRow(const char* Label) const {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(Label);
        ImGui::PopTextWrapPos();
    }

    void Editor::FDetailsPanel::FinishInactiveSceneEdit() {
        if (mSceneEditToken == 0 || (IsVisible() && ImGui::GetActiveID() == mSceneEditItem && GetEditorContext()->IsSceneEditActive())) {
            return;
        }

        GetEditorContext()->EndSceneEdit(mSceneEditToken);
        mSceneEditToken = 0;
        mSceneEditItem = 0;
    }

    void Editor::FDetailsPanel::BeginItemSceneEdit() {
        FinishInactiveSceneEdit();
        const ImGuiID Item{ImGui::GetActiveID()};

        if (mSceneEditToken == 0 && Item != 0) {
            mSceneEditToken = GetEditorContext()->BeginSceneEdit();
            mSceneEditItem = Item;
        }
    }

    /**
     * @brief 생성 가능한 컴포넌트의 검색·추가 메뉴를 표시합니다.
     */
    void Editor::FDetailsPanel::DrawComponentMenu(bool CanAdd) {
        Editor::FEditorContext* Context{GetEditorContext()};

        ImGui::BeginDisabled(!CanAdd);

        if (ImGui::Button("+ Add")) {
            mComponentSearch.fill('\0');
            ImGui::OpenPopup("AddComponent");
        }

        ImGui::EndDisabled();

        if (!CanAdd && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("컴포넌트를 추가할 액터 하나를 선택하세요.");
        }

        if (ImGui::BeginPopup("AddComponent")) {
            if (ImGui::IsWindowAppearing()) {
                ImGui::SetKeyboardFocusHere();
            }

            ImGui::SetNextItemWidth(260.0f);
            ImGui::InputTextWithHint("##ComponentSearch", "컴포넌트 검색", mComponentSearch.data(), mComponentSearch.size());
            ImGui::Separator();

            const ImGuiTextFilter Filter{mComponentSearch.data()};
            bool HasEntries{};

            for (const CoreUObject::UClass* Class : CoreUObject::FReflectionRegistry::GetDerivedClasses(Engine::UActorComponent::StaticClass())) {
                const Core::FString Label{ComponentLabel(Class)};

                if (!Class->CanCreate() || (!Filter.PassFilter(Label.c_str()) && !Filter.PassFilter(Class->GetName().c_str()))) {
                    continue;
                }

                HasEntries = true;

                ImGui::PushID(Class);
                ImGui::BeginDisabled(!Context->CanAddComponent(Class));

                if (ImGui::Selectable(Label.c_str(), false, ImGuiSelectableFlags_DontClosePopups) && Context->AddComponent(Class) != nullptr) {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndDisabled();
                ImGui::PopID();
            }

            if (!HasEntries) {
                ImGui::TextDisabled("검색 결과가 없습니다.");
            }

            ImGui::EndPopup();
        }

        ImGui::Separator();
    }

    /**
     * @brief 선택한 액터 하나와 그 컴포넌트를 고르는 트리를 표시합니다.
     */
    void Editor::FDetailsPanel::DrawComponentTree(Engine::AActor* Actor, CoreUObject::FObjectHandle Selected) {
        Editor::FEditorContext* Context{GetEditorContext()};
        const CoreUObject::FObjectHandle ActorHandle{Actor->GetHandle()};
        Core::TArray<CoreUObject::FObjectHandle> Components{};

        if (const auto* Root{Actor->GetRootComponent()}) {
            Components.push_back(Root->GetHandle());
        }

        for (const auto& Component : Actor->GetComponents()) {
            if (Component.get() != Actor->GetRootComponent()) {
                Components.push_back(Component->GetHandle());
            }
        }

        constexpr std::size_t MaxVisibleRows{8};
        const float RowHeight{ImGui::GetTextLineHeight() + ImGui::GetStyle().CellPadding.y * 2.0f};
        const float Height{static_cast<float>(std::min(Components.size() + 1, MaxVisibleRows)) * RowHeight + 4.0f};
        constexpr ImGuiTableFlags Flags{ImGuiTableFlags_BordersOuter | ImGuiTableFlags_ScrollY};

        if (ImGui::BeginTable("##Components", 1, Flags, {0.0f, Height})) {
            const auto DrawRow{[this, Context, Selected](CoreUObject::FObjectHandle Handle, float Indent) {
                CoreUObject::UObject* Object{Context->ResolveObject(Handle)};

                if (Object == nullptr) {
                    return;
                }

                ImGui::PushID(Object);
                ImGui::TableNextRow();
                ObjectRow::DrawLabel(Object, Handle == Selected, Indent);

                if (ImGui::IsItemClicked()) {
                    if (mLockedContext != nullptr) {
                        mLockedTargets = {Handle};
                    } else {
                        Context->SelectObject(Handle);
                    }
                }

                if (CoreUObject::Cast<Engine::AActor>(Object) != nullptr && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    Context->OpenInSequencer(Handle);
                }

                ImGui::SetItemTooltip("%s", ObjectRow::GetTypeLabel(Object).c_str());
                ImGui::PopID();
            }};

            DrawRow(ActorHandle, 0.0f);

            for (CoreUObject::FObjectHandle Handle : Components) {
                DrawRow(Handle, ImGui::GetTreeNodeToLabelSpacing());
            }

            ImGui::EndTable();
        }
    }

    /**
     * @brief 객체 타입의 편집 가능한 리플렉션 프로퍼티를 순회하여 표시합니다.
     */
    void Editor::FDetailsPanel::DrawReflectedObjects(const Core::TArray<CoreUObject::FObjectHandle>& Targets) {
        Editor::FEditorContext* Context{GetEditorContext()};
        CoreUObject::UObject* Object{Context->ResolveObject(Targets.front())};

        if (Object == nullptr) {
            return;
        }

        const auto* CommonClass{Object->GetClass()};

        for (const CoreUObject::FObjectHandle Handle : Targets) {
            const CoreUObject::UObject* Other{Context->ResolveObject(Handle)};

            if (Other == nullptr) {
                return;
            }

            while (!Other->IsA(CommonClass)) {
                CommonClass = CommonClass->GetSuperClass();
            }
        }

        Core::TArray<const CoreUObject::UClass*> Classes{};
        Core::TArray<FDetailRow> Rows{};
        Core::TArray<CoreUObject::FObjectHandle> Roots{};

        const bool TransformMatches{MatchesSearch("Transform", "Location") || MatchesSearch("Transform", "Rotation") || MatchesSearch("Transform", "Scale")};

        if (TransformMatches && CommonClass->IsChildOf(Engine::AActor::StaticClass())) {
            for (const CoreUObject::FObjectHandle Handle : Targets) {
                const auto* Actor{CoreUObject::Cast<Engine::AActor>(Context->ResolveObject(Handle))};

                if (Actor != nullptr && Actor->GetRootComponent() != nullptr) {
                    Roots.push_back(Actor->GetRootComponent()->GetHandle());
                }
            }

            if (Roots.size() == Targets.size()) {
                const auto* Property{Engine::USceneComponent::StaticClass()->FindClassProperty("Transform")};
                Rows.push_back({Property, std::move(Roots), "Transform", "Transform"});
            }
        }

        const bool SkeletalMesh{CommonClass->IsChildOf(Engine::USkeletalMeshComponent::StaticClass())};
        const auto IsGraphMode{[Context](CoreUObject::FObjectHandle Target) {
            const auto* Component{CoreUObject::Cast<Engine::USkeletalMeshComponent>(Context->ResolveObject(Target))};

            return Component != nullptr && !Component->GetAnimClass().empty();
        }};
        const bool AllGraphMode{SkeletalMesh && std::all_of(Targets.begin(), Targets.end(), IsGraphMode)};
        const bool AllAssetMode{SkeletalMesh && std::none_of(Targets.begin(), Targets.end(), IsGraphMode)};

        for (const CoreUObject::UClass* Class{CommonClass}; Class != nullptr; Class = Class->GetSuperClass()) {
            Classes.push_back(Class);
        }

        for (auto Class{Classes.rbegin()}; Class != Classes.rend(); ++Class) {
            for (const CoreUObject::FProperty* Property : (*Class)->GetProperties()) {
                if (!IsVisibleProperty(Property)) {
                    continue;
                }

                const Core::FString& Name{Property->GetName()};

                if (Targets.size() > 1 && Name == "Name") {
                    continue;
                }

                if (SkeletalMesh && !AllAssetMode && (Name == "Animation" || Name == "Looping" || Name == "PlayRate")) {
                    continue;
                }

                const std::string_view Category{DetailCategory(*Class, Name)};
                const Core::FString Label{Name == "AnimClass" ? "Animation Mode" : PropertyLabel(Property)};

                if (Name == "Transform" ? TransformMatches : MatchesSearch(Category, Label, Name)) {
                    Rows.push_back({Property, Targets, Category, Label, IsAdvancedProperty(Name)});
                }
            }
        }

        if (mOnlyModified) {
            std::erase_if(Rows, [this](const FDetailRow& Row) {
                return !IsModified(Row);
            });
        }

        constexpr std::array Categories{"Transform", "Animation", "Mesh", "Materials", "Light", "Camera", "Fog", "Shape", "Movement", "Rendering", "General", "Settings", "Component Tick"};
        const bool Searching{mPropertySearch.front() != '\0'};
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {5.0f, 2.0f});

        for (const char* Category : Categories) {
            const std::string_view CategoryName{Category};
            const bool GraphRow{CategoryName == "Animation" && AllGraphMode && (MatchesSearch(CategoryName, "Anim Graph", "AnimGraph") || MatchesSearch(CategoryName, "Debug Anim Graph"))};
            auto* Mesh{CategoryName == "Materials" && Targets.size() == 1 ? CoreUObject::Cast<Engine::UMeshComponent>(Context->ResolveObject(Targets.front())) : nullptr};
            const auto* Geometry{Mesh != nullptr ? Mesh->GetStaticMesh() : nullptr};
            const bool MaterialRows{Geometry != nullptr && std::any_of(Geometry->GetMeshData().GetMaterialSlots().begin(), Geometry->GetMeshData().GetMaterialSlots().end(), [this, Mesh, Geometry](const Asset::FMeshMaterialSlot& Slot) {
                const auto Index{static_cast<Core::Uint32>(&Slot - Geometry->GetMeshData().GetMaterialSlots().data())};

                return MatchesSearch("Materials", Slot.GetName(), "Material Slots") && (!mOnlyModified || !Mesh->GetSlotMaterialPath(Index).empty());
            })};
            const bool HasRows{std::any_of(Rows.begin(), Rows.end(), [CategoryName](const FDetailRow& Row) {
                return Row.mCategory == CategoryName;
            })};

            if ((!HasRows && !GraphRow && !MaterialRows) || !BeginCategory(Category, CategoryName != "Component Tick")) {
                continue;
            }

            ImGui::PushID(Category);
            const bool HasAdvanced{!Searching && !mShowAdvanced && std::any_of(Rows.begin(), Rows.end(), [CategoryName](const FDetailRow& Row) {
                return Row.mCategory == CategoryName && Row.mAdvanced;
            })};
            const ImGuiID AdvancedId{ImGui::GetID("##Advanced")};
            const bool ShowAdvanced{!HasAdvanced || ImGui::GetStateStorage()->GetBool(AdvancedId)};
            bool DrawAdvancedToggle{};

            if (BeginPropertyTable()) {
                for (const FDetailRow& Row : Rows) {
                    if (Row.mCategory != CategoryName || (!ShowAdvanced && Row.mAdvanced)) {
                        continue;
                    }

                    if (Row.mProperty->GetName() == "Transform") {
                        DrawTransform(Row);
                    } else if (Row.mProperty->GetName() == "AnimClass") {
                        DrawAnimationMode(Row);
                    } else {
                        DrawPropertyRow(Row);
                    }
                }

                if (GraphRow) {
                    DrawAnimInstance(Targets);
                }

                if (MaterialRows) {
                    DrawMaterialSlots(Mesh);
                }

                ImGui::EndTable();
                DrawAdvancedToggle = HasAdvanced;
            }

            if (DrawAdvancedToggle && ImGui::SmallButton(ShowAdvanced ? "Hide Advanced" : "Show Advanced")) {
                ImGui::GetStateStorage()->SetBool(AdvancedId, !ShowAdvanced);
            }

            ImGui::PopID();
        }

        ImGui::PopStyleVar(2);

        if (Targets.size() == 1) {
            Object = Context->ResolveObject(Targets.front());

            if (const auto* Light{CoreUObject::Cast<Engine::ULightComponent>(Object)}; Light != nullptr && !mOnlyModified && MatchesSearch("Rendering", "Shadow Depth Maps") && BeginCategory("Shadow Depth Maps", false)) {
                DrawShadowMaps(Light);
            }

            if (auto* ScriptComponent{CoreUObject::Cast<Script::UScriptComponent>(Object)}; ScriptComponent != nullptr && BeginCategory("C# Script")) {
                DrawScriptFields(ScriptComponent);
            }
        }
    }

    void Editor::FDetailsPanel::DrawMaterialSlots(Engine::UMeshComponent* Component) {
        if (Component == nullptr || Component->GetStaticMesh() == nullptr) {
            return;
        }

        Editor::FEditorContext* Context{GetEditorContext()};
        const CoreUObject::FObjectHandle Handle{Component->GetHandle()};
        const Core::FString Override{Component->GetMaterialPath()};
        const Asset::FMeshData& Data{Component->GetStaticMesh()->GetMeshData()};
        const auto& Slots{Data.GetMaterialSlots()};

        for (Core::Uint32 Slot{}; Slot < Slots.size(); ++Slot) {
            const auto& SubMeshes{Data.GetSubMeshes()};

            if (std::none_of(SubMeshes.begin(), SubMeshes.end(), [Slot](const Asset::FSubMesh& SubMesh) {
                return SubMesh.GetMaterialIndex() == Slot;
            })) {
                continue;
            }

            const Core::FString& Label{Slots[Slot].GetName()};
            Core::FString Path{Component->GetSlotMaterialPath(Slot)};

            if ((!MatchesSearch("Materials", Label, "Material Slots") || (mOnlyModified && Path.empty()))) {
                continue;
            }

            ImGui::PushID(static_cast<int>(Slot));
            BeginPropertyRow(Label.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::BeginDisabled(!Context->CanEditWorld() || !Override.empty());
            const Core::FString& Inherited{Override.empty() ? Slots[Slot].GetMaterialPath() : Override};
            const bool Changed{DrawAssetPicker("##Slot", Path, Asset::EAssetType::Material, "Use mesh material", Inherited)};

            ImGui::TableSetColumnIndex(2);

            if ((!Path.empty() && DrawResetButton()) || Changed) {
                Context->SetMaterialOverride(Handle, Slot, Changed ? Path : Core::FString{});
            }

            ImGui::EndDisabled();
            ImGui::PopID();
        }
    }

    void Editor::FDetailsPanel::DrawShadowMaps(const Engine::ULightComponent* Light) {
        const auto* Renderer{GetEditorContext()->GetRenderer()};
        const RenderCore::FShadowAtlasView Atlas{Renderer != nullptr ? Renderer->GetShadowAtlasView() : RenderCore::FShadowAtlasView{}};
        bool Found{};

        for (const RenderCore::FShadowAtlasTile& Tile : Atlas.mTiles) {
            if (Tile.mLightId != Light->GetGuid() || Atlas.mTexture == nullptr) {
                continue;
            }

            Found = true;
            ImGui::Text("Map %u | %u x %u", Tile.mFace, Tile.mResolution, Tile.mResolution);
            const float Size{std::clamp(ImGui::GetContentRegionAvail().x, 64.0f, 320.0f)};
            ImGui::Image(reinterpret_cast<ImTextureID>(Atlas.mTexture), {Size, Size}, {Tile.mRectangle.x, Tile.mRectangle.y}, {Tile.mRectangle.x + Tile.mRectangle.z, Tile.mRectangle.y + Tile.mRectangle.w});
        }

        if (!Found) {
            ImGui::TextDisabled("No active shadow map in the last rendered view.");
        }
    }

    void Editor::FDetailsPanel::DrawAnimationMode(const FDetailRow& Row) {
        Editor::FEditorContext* Context{GetEditorContext()};
        const auto IsGraph{[Context, &Row](CoreUObject::FObjectHandle Target) {
            return !std::get<Core::FString>(Row.mProperty->ReadValue(Context->ResolveObject(Target))).empty();
        }};
        const bool Graph{IsGraph(Row.mTargets.front())};
        const bool Mixed{std::any_of(Row.mTargets.begin(), Row.mTargets.end(), [&IsGraph, Graph](CoreUObject::FObjectHandle Target) {
            return IsGraph(Target) != Graph;
        })};
        const bool Editable{std::all_of(Row.mTargets.begin(), Row.mTargets.end(), [Context, &Row](CoreUObject::FObjectHandle Target) {
            return Context->CanEditProperty(Target, Row.mProperty);
        })};

        if (mOnlyModified && !Graph && !Mixed) {
            return;
        }

        ImGui::PushID(Row.mProperty);
        BeginPropertyRow(Row.mLabel.c_str());
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::BeginDisabled(!Editable);

        if (ImGui::BeginCombo("##AnimationMode", Mixed ? "Multiple Values" : Graph ? "Use Anim Graph" : "Use Animation Asset")) {
            if (ImGui::Selectable("Use Anim Graph", Graph && !Mixed)) {
                EditProperty(Row, Engine::UAnimGraphInstance::StaticClass()->GetName(), false);
            }

            if (ImGui::Selectable("Use Animation Asset", !Graph && !Mixed)) {
                EditProperty(Row, Core::FString{}, false);
            }

            ImGui::EndCombo();
        }

        ImGui::EndDisabled();
        ImGui::PopID();
    }

    std::optional<CoreUObject::FPropertyValue> Editor::FDetailsPanel::GetDefaultValue(const CoreUObject::UClass* Class, const CoreUObject::FProperty* Property) {
        if (!Class->CanCreate() || Property->GetName() == "Name") {
            return {};
        }

        auto Position{mDefaultValues.find(Class)};

        if (Position == mDefaultValues.end()) {
            auto& Values{mDefaultValues[Class]};
            const auto Defaults{Class->CreateInstance()};

            if (Defaults != nullptr) {
                for (const CoreUObject::UClass* Current{Class}; Current != nullptr; Current = Current->GetSuperClass()) {
                    for (const CoreUObject::FProperty* Field : Current->GetProperties()) {
                        if (IsVisibleProperty(Field)) {
                            auto Value{Field->ReadValue(Defaults.get())};

                            if (!std::holds_alternative<const void*>(Value)) {
                                Values.emplace(Field, std::move(Value));
                            }
                        }
                    }
                }
            }

            Position = mDefaultValues.find(Class);
        }

        const auto Found{Position->second.find(Property)};

        return Found != Position->second.end() ? std::optional<CoreUObject::FPropertyValue>{Found->second} : std::nullopt;
    }

    bool Editor::FDetailsPanel::IsModified(const FDetailRow& Row) {
        Editor::FEditorContext* Context{GetEditorContext()};

        return std::any_of(Row.mTargets.begin(), Row.mTargets.end(), [this, Context, &Row](CoreUObject::FObjectHandle Target) {
            const CoreUObject::UObject* Object{Context->ResolveObject(Target)};

            if (Object == nullptr) {
                return false;
            }

            const auto Default{GetDefaultValue(Object->GetClass(), Row.mProperty)};

            return Default.has_value() && !EqualValues(Row.mProperty->ReadValue(Object), *Default);
        });
    }

    void Editor::FDetailsPanel::EditProperty(const FDetailRow& Row, const CoreUObject::FPropertyValue& Value, bool Interactive) {
        Editor::FEditorContext* Context{GetEditorContext()};

        if (Interactive) {
            BeginItemSceneEdit();
        }

        const Core::Uint64 Token{Context->BeginSceneEdit()};

        for (CoreUObject::FObjectHandle Target : Row.mTargets) {
            CoreUObject::UObject* Object{Context->ResolveObject(Target)};

            if (Object != nullptr && !EqualValues(Row.mProperty->ReadValue(Object), Value)) {
                Context->EditProperty(Target, Row.mProperty, Value, Interactive);
            }
        }

        Context->EndSceneEdit(Token);
    }

    void Editor::FDetailsPanel::DrawPropertyRow(const FDetailRow& Row) {
        Editor::FEditorContext* Context{GetEditorContext()};
        const CoreUObject::FObjectHandle First{Row.mTargets.front()};
        const CoreUObject::UObject* Object{Context->ResolveObject(First)};

        if (Object == nullptr) {
            return;
        }

        CoreUObject::FPropertyValue Value{Row.mProperty->ReadValue(Object)};
        Core::TArray<std::pair<CoreUObject::FObjectHandle, CoreUObject::FPropertyValue>> Defaults{};
        bool Mixed{};
        bool Editable{true};

        for (CoreUObject::FObjectHandle Target : Row.mTargets) {
            const CoreUObject::UObject* Current{Context->ResolveObject(Target)};

            if (Current == nullptr) {
                return;
            }

            const CoreUObject::FPropertyValue CurrentValue{Target == First ? Value : Row.mProperty->ReadValue(Current)};
            Mixed = Mixed || !EqualValues(Value, CurrentValue);
            Editable = Editable && Context->CanEditProperty(Target, Row.mProperty);

            if (auto Default{GetDefaultValue(Current->GetClass(), Row.mProperty)}; Default.has_value() && !EqualValues(CurrentValue, *Default)) {
                Defaults.emplace_back(Target, std::move(*Default));
            }
        }

        if (mOnlyModified && Defaults.empty()) {
            return;
        }

        ImGui::PushID(Row.mProperty);
        ImGui::PushID(Object);
        BeginPropertyRow(Row.mLabel.c_str());
        bool Reset{};
        bool Pasted{};

        if (ImGui::BeginPopupContextItem("##PropertyMenu")) {
            Serialization::FJsonArchive Copy{Core::EArchiveMode::Saving};
            const bool CanCopy{!Mixed && ArchiveDetailValue(Copy, Value)};

            if (ImGui::MenuItem("Copy", nullptr, false, CanCopy)) {
                ImGui::SetClipboardText(Copy.ToString().c_str());
            }

            const char* Clipboard{ImGui::GetClipboardText()};
            auto PastedValue{Value};
            Serialization::FJsonArchive Paste{Core::EArchiveMode::Loading};
            const bool CanPaste{Editable && Clipboard != nullptr && Paste.Parse(Clipboard) && ArchiveDetailValue(Paste, PastedValue)};

            if (ImGui::MenuItem("Paste", nullptr, false, CanPaste)) {
                Value = std::move(PastedValue);
                Pasted = true;
            }

            Reset = ImGui::MenuItem("Reset to Default", nullptr, false, Editable && !Defaults.empty());
            ImGui::EndPopup();
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::BeginDisabled(!Editable);

        if (Mixed) {
            ImGui::TextDisabled("Multiple Values");
        }

        ImGui::SetNextItemWidth(-1.0f);

        if (DrawPropertyValue(Row.mProperty, Value, "##Value") || Pasted) {
            EditProperty(Row, Value, !Pasted && ImGui::IsItemActive());
        }

        ImGui::TableSetColumnIndex(2);

        if (!Defaults.empty() && (DrawResetButton() || Reset) && Editable) {
            const Core::Uint64 Token{Context->BeginSceneEdit()};

            for (const auto& [Target, Default] : Defaults) {
                Context->EditProperty(Target, Row.mProperty, Default);
            }

            Context->EndSceneEdit(Token);
        }

        ImGui::EndDisabled();
        ImGui::PopID();
        ImGui::PopID();
    }

    void Editor::FDetailsPanel::DrawTransform(const FDetailRow& Row) {
        Editor::FEditorContext* Context{GetEditorContext()};
        const CoreUObject::UObject* Object{Context->ResolveObject(Row.mTargets.front())};

        if (Object == nullptr) {
            return;
        }

        const bool Editable{std::all_of(Row.mTargets.begin(), Row.mTargets.end(), [Context, &Row](CoreUObject::FObjectHandle Target) {
            return Context->CanEditProperty(Target, Row.mProperty);
        })};
        constexpr std::array Labels{"Location", "Rotation", "Scale"};
        constexpr std::array Axes{"X %.3f", "Y %.3f", "Z %.3f"};
        constexpr std::array AxisIds{"X", "Y", "Z"};
        const auto ReadPart{[](const Core::FTransform& Transform, std::size_t Part) {
            if (Part == 1) {
                const Core::FRotator Rotation{Transform.GetRotator()};

                return std::array<float, 3>{Rotation.roll, Rotation.pitch, Rotation.yaw};
            }

            return (Part == 0 ? Transform.mLocation : Transform.mScale).ToArray();
        }};

        ImGui::PushID(Object);
        ImGui::BeginDisabled(!Editable);

        for (std::size_t Part{}; Part < Labels.size(); ++Part) {
            if (!MatchesSearch("Transform", Labels[Part])) {
                continue;
            }

            const auto First{ReadPart(std::get<Core::FTransform>(Row.mProperty->ReadValue(Object)), Part)};
            std::array<bool, 3> Mixed{};
            bool Modified{};

            for (CoreUObject::FObjectHandle Target : Row.mTargets) {
                const CoreUObject::UObject* Current{Context->ResolveObject(Target)};

                if (Current == nullptr) {
                    continue;
                }

                const auto Values{ReadPart(std::get<Core::FTransform>(Row.mProperty->ReadValue(Current)), Part)};

                for (std::size_t Axis{}; Axis < Axes.size(); ++Axis) {
                    Mixed[Axis] = Mixed[Axis] || Values[Axis] != First[Axis];
                }

                if (const auto Default{GetDefaultValue(Current->GetClass(), Row.mProperty)}; Default.has_value()) {
                    Modified = Modified || Values != ReadPart(std::get<Core::FTransform>(*Default), Part);
                }
            }

            if (mOnlyModified && !Modified) {
                continue;
            }

            ImGui::PushID(Labels[Part]);
            BeginPropertyRow(Labels[Part]);
            ImGui::TableSetColumnIndex(1);
            auto Values{First};
            std::array<bool, 3> Changed{};
            bool Interactive{};
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {2.0f, ImGui::GetStyle().ItemSpacing.y});
            const float Width{std::max(1.0f, (ImGui::GetContentRegionAvail().x - 4.0f) / 3.0f)};

            for (std::size_t Axis{}; Axis < Axes.size(); ++Axis) {
                if (Axis > 0) {
                    ImGui::SameLine();
                }

                ImGui::PushID(AxisIds[Axis]);
                ImGui::SetNextItemWidth(Width);
                Changed[Axis] = ImGui::DragFloat("##Axis", &Values[Axis], Part == 1 ? 0.25f : 0.025f, 0.0f, 0.0f, Mixed[Axis] ? "Multiple" : Axes[Axis]);
                Interactive = Interactive || (Changed[Axis] && ImGui::IsItemActive());
                ImGui::PopID();
            }

            ImGui::PopStyleVar();
            ImGui::TableSetColumnIndex(2);
            const bool Reset{Modified && DrawResetButton()};

            if (Reset || std::any_of(Changed.begin(), Changed.end(), [](bool AxisChanged) {
                return AxisChanged;
            })) {
                if (Interactive) {
                    BeginItemSceneEdit();
                }

                const Core::Uint64 Token{Context->BeginSceneEdit()};

                for (CoreUObject::FObjectHandle Target : Row.mTargets) {
                    const CoreUObject::UObject* Current{Context->ResolveObject(Target)};

                    if (Current == nullptr) {
                        continue;
                    }

                    Core::FTransform Transform{std::get<Core::FTransform>(Row.mProperty->ReadValue(Current))};
                    auto NewPart{ReadPart(Transform, Part)};

                    if (Reset) {
                        const auto Default{GetDefaultValue(Current->GetClass(), Row.mProperty)};

                        if (!Default.has_value()) {
                            continue;
                        }

                        NewPart = ReadPart(std::get<Core::FTransform>(*Default), Part);
                    } else {
                        for (std::size_t Axis{}; Axis < Axes.size(); ++Axis) {
                            if (Changed[Axis]) {
                                NewPart[Axis] = Values[Axis];
                            }
                        }
                    }

                    if (Part == 0) {
                        Transform.mLocation = {NewPart[0], NewPart[1], NewPart[2]};
                    } else if (Part == 1) {
                        Transform.SetRotator({NewPart[1], NewPart[2], NewPart[0]});
                    } else {
                        Transform.mScale = {NewPart[0], NewPart[1], NewPart[2]};
                    }

                    Context->EditProperty(Target, Row.mProperty, Transform, Interactive);
                }

                Context->EndSceneEdit(Token);
            }

            ImGui::PopID();
        }

        ImGui::EndDisabled();
        ImGui::PopID();
    }

    /**
     * @brief 프로퍼티 타입에 맞는 편집 위젯을 표시하고 변경한 값을 돌려줍니다.
     */
    bool Editor::FDetailsPanel::DrawPropertyValue(const CoreUObject::FProperty* Property, CoreUObject::FPropertyValue& Value, const char* Label) {

        if (Property->GetType() == CoreUObject::EPropertyType::Enum) {
            const bool ShadowBias{Property->GetNativeType() == CoreUObject::GetNativeType<Engine::EShadowBiasPreset>()};
            const bool SlopeBias{ShadowBias && Property->GetName() == "ShadowSlopeBiasPreset"};
            Core::Int32 Number{std::get<Core::Int32>(Value)};
            const auto& Choices{Property->GetEnumValues()};
            const auto Current{std::find_if(Choices.begin(), Choices.end(), [Number](const CoreUObject::FEnumValue& Choice) {
                return Choice.mValue == Number;
            })};
            const auto FormatChoice{[ShadowBias, SlopeBias](const CoreUObject::FEnumValue& Choice) {
                if (!ShadowBias) {
                    return Choice.mName;
                }

                return SlopeBias ? std::format("Level {} ({:.2f})", Choice.mValue, static_cast<float>(Choice.mValue) * Engine::ShadowSlopeBiasStep) : std::format("Level {} ({})", Choice.mValue, Choice.mValue * Engine::ShadowDepthBiasStep);
            }};
            const Core::FString Preview{Current != Choices.end() ? FormatChoice(*Current) : "Invalid"};

            bool Changed{};

            if (ImGui::BeginCombo(Label, Preview.c_str())) {
                for (const CoreUObject::FEnumValue& Choice : Choices) {
                    const Core::FString Option{FormatChoice(Choice)};

                    if (ImGui::Selectable(Option.c_str(), Number == Choice.mValue)) {
                        Number = Choice.mValue;
                        Changed = true;
                    }
                }

                ImGui::EndCombo();
            }

            Value = Number;

            return Changed;
        }

        if (auto* Vector2{std::get_if<Core::FVector2>(&Value)}) {
            auto Values{Vector2->ToArray()};

            if (ImGui::DragFloat2(Label, Values.data(), 0.025f)) {
                *Vector2 = {Values[0], Values[1]};

                return true;
            }
        } else if (auto* Vector4{std::get_if<Core::FVector4>(&Value)}) {
            auto Values{Vector4->ToArray()};


            const bool Changed{Property->GetName().ends_with("Color") ? ImGui::ColorEdit4(Label, Values.data()) : ImGui::DragFloat4(Label, Values.data(), 0.025f)};

            if (Changed) {
                *Vector4 = {Values[0], Values[1], Values[2], Values[3]};

                return true;
            }
        } else if (auto* Matrix{std::get_if<Core::FMatrix>(&Value)}) {
            bool Changed{};

            if (ImGui::TreeNodeEx(Label, ImGuiTreeNodeFlags_DefaultOpen)) {
                for (Core::Int32 Row{}; Row < 4; ++Row) {
                    std::array<float, 4> Values{(*Matrix)(Row, 0), (*Matrix)(Row, 1), (*Matrix)(Row, 2), (*Matrix)(Row, 3)};

                    if (ImGui::DragFloat4(DrawFieldLabel(std::to_string(Row).c_str()).c_str(), Values.data(), 0.025f)) {
                        for (Core::Int32 Column{}; Column < 4; ++Column) {
                            (*Matrix)(Row, Column) = Values[static_cast<std::size_t>(Column)];
                        }

                        Changed = true;
                    }
                }

                ImGui::TreePop();
            }

            return Changed;
        } else if (auto* Vector{std::get_if<Core::FVector>(&Value)}) {
            auto Values{Vector->ToArray()};

            if (ImGui::DragFloat3(Label, Values.data(), 0.025f, 0.0f, 0.0f, "%.3f")) {
                *Vector = {Values[0], Values[1], Values[2]};

                return true;
            }
        } else if (auto* Rotation{std::get_if<Core::FRotator>(&Value)}) {
            auto Values{Rotation->ToArray()};

            if (ImGui::DragFloat3(Label, Values.data(), 0.25f, 0.0f, 0.0f, "%.2f")) {
                *Rotation = {Values[0], Values[1], Values[2]};

                return true;
            }
        } else if (auto* String{std::get_if<Core::FString>(&Value)}) {
            const std::string_view Name{Property->GetName()};

            if (Name == "ScriptClass") {
                return DrawScriptClassPicker(Label, *String);
            }

            if (Name == "SkeletalMesh") {
                return DrawAssetPicker(Label, *String, Asset::EAssetType::SkeletalMesh, "None");
            }

            if (Name == "Skeleton") {
                return DrawAssetPicker(Label, *String, Asset::EAssetType::Skeleton, "None");
            }

            if (Name == "Animation") {
                return DrawAssetPicker(Label, *String, Asset::EAssetType::AnimSequence, "None");
            }

            if (Name == "AnimGraph") {
                return DrawAssetPicker(Label, *String, Asset::EAssetType::AnimGraph, "None");
            }

            if (Name == "Mesh") {
                return DrawAssetPicker(Label, *String, Asset::EAssetType::Mesh, "Primitive mesh");
            }

            if (Name == "Material") {
                return DrawAssetPicker(Label, *String, Asset::EAssetType::Material, "Use mesh materials");
            }

            if (Name == "Texture" || Name == "NormalMap" || Name == "Sprite") {
                return DrawAssetPicker(Label, *String, Asset::EAssetType::Texture, Name == "Sprite" ? "Default atlas" : "None");
            }

            if (Property->GetName() == "Text") {
                Core::TArray<char> TextBuffer{};
                TextBuffer.resize(std::max<std::size_t>(4096, String->size() + 1024));
                std::copy(String->begin(), String->end(), TextBuffer.begin());

                if (ImGui::InputTextMultiline(Label, TextBuffer.data(), TextBuffer.size(), {0.0f, ImGui::GetTextLineHeight() * 4.0f}, ImGuiInputTextFlags_AllowTabInput)) {
                    *String = TextBuffer.data();

                    return true;
                }

                return false;
            }

            std::array<char, 512> Buffer{};

            std::copy_n(String->begin(), std::min(String->size(), Buffer.size() - 1), Buffer.begin());

            if (ImGui::InputText(Label, Buffer.data(), Buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
                *String = Buffer.data();

                return true;
            }
        } else if (auto* Boolean{std::get_if<bool>(&Value)}) {
            return ImGui::Checkbox(Label, Boolean);
        } else if (auto* FloatValue{std::get_if<float>(&Value)}) {
            return ImGui::DragFloat(Label, FloatValue, 0.025f);
        } else if (auto* IntegerValue{std::get_if<Core::Int32>(&Value)}) {
            return ImGui::InputInt(Label, IntegerValue);
        } else if (auto* UnsignedValue{std::get_if<Core::Uint32>(&Value)}) {
            return ImGui::InputScalar(Label, ImGuiDataType_U32, UnsignedValue);
        } else if (auto* Unsigned64Value{std::get_if<Core::Uint64>(&Value)}) {
            return ImGui::InputScalar(Label, ImGuiDataType_U64, Unsigned64Value);
        } else if (auto* Guid{std::get_if<Core::FGuid>(&Value)}) {

            ImGui::TextWrapped("%s", Guid->ToString().c_str());
        } else if (auto* Reference{std::get_if<CoreUObject::FObjectReference>(&Value)}) {

            ImGui::TextWrapped("%s", Reference->GetGuid().ToString().c_str());
        } else if (const CoreUObject::UStruct* Struct{Property->GetStruct()}) {
            bool Changed{};

            if (ImGui::TreeNodeEx(Label, ImGuiTreeNodeFlags_DefaultOpen)) {
                for (const CoreUObject::FProperty* Child : Struct->GetProperties()) {
                    if (Child->HasFlag(Core::EPropertyFlags::Transient)) {
                        continue;
                    }

                    CoreUObject::FPropertyValue ChildValue{Child->ReadValue(CoreUObject::GetPropertyData(Value))};
                    const bool ReadOnly{Child->HasFlag(Core::EPropertyFlags::ReadOnly)};

                    ImGui::BeginDisabled(ReadOnly);

                    const Core::FString ChildLabel{DrawFieldLabel(PropertyLabel(Child).c_str())};

                    if (DrawPropertyValue(Child, ChildValue, ChildLabel.c_str()) && !ReadOnly) {
                        Changed = Child->WriteValue(CoreUObject::GetPropertyData(Value), ChildValue) || Changed;
                    }

                    ImGui::EndDisabled();
                }

                ImGui::TreePop();
            }

            return Changed;
        }

        return false;
    }

    /**
     * @brief 메시 또는 재질 선택과 드래그 앤 드롭을 표시합니다.
     */
    bool Editor::FDetailsPanel::DrawAssetPicker(const char* Label, Core::FString& Path, Asset::EAssetType Type, const char* EmptyLabel, const Core::FString& InheritedPath) {
        if (mAssetCatalog == nullptr || mThumbnails == nullptr) {
            ImGui::TextDisabled("%s", Path.empty() ? EmptyLabel : Path.c_str());

            return false;
        }

        ImGui::PushID(Label);
        bool Changed{mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, "##AssetReference", Path, Type, InheritedPath, EmptyLabel, "Mesh", false)};

        if (ImGui::BeginPopupContextItem("##AssetActions")) {
            const std::filesystem::path Selection{mGetSelectedAsset ? mGetSelectedAsset() : std::filesystem::path{}};
            const auto* Entry{!Selection.empty() ? mAssetCatalog->Find(Selection) : nullptr};
            const Core::FString& Effective{Path.empty() ? InheritedPath : Path};

            if (ImGui::MenuItem("Use Selected Asset from Content Browser", nullptr, false, Entry != nullptr && Entry->GetType() == Type)) {
                Path = Core::FilePath::ToGenericUtf8(Entry->GetPath());
                Changed = true;
            }

            if (ImGui::MenuItem("Browse to Asset in Content Browser", nullptr, false, mBrowseAsset && !Effective.empty())) {
                mBrowseAsset(Core::FilePath::FromUtf8(Effective));
            }

            ImGui::EndPopup();
        }

        ImGui::PopID();

        return Changed;
    }

}
