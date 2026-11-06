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
#include "Engine/Component/UMeshComponent.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Asset/USkeletalMesh.h"
#include "Engine/Component/ULightComponent.h"
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
#include "Editor/FScriptBuildService.h"
#include "Script/FScriptField.h"
#include "Script/FScriptRuntime.h"
#include "Script/UScriptComponent.h"
#include "Panel/ObjectRow.h"

#include <imgui.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <string>
#include <string_view>

namespace Editor {

    /**
     * @brief Details 패널에서 사용하는 컴포넌트 표시 이름 변환입니다.
     */
    namespace {
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

        /**
         * @brief Details에 표시할 비일시적 편집 가능 또는 읽기 전용 프로퍼티인지 검사합니다.
         * @param[in] Property 유효한 리플렉션 프로퍼티의 비소유 주소입니다.
         * @return Transient가 아니고 Edit 또는 ReadOnly 플래그가 있으면 true입니다.
         */
        bool IsVisibleProperty(const CoreUObject::FProperty* Property) {
            return !Property->HasFlag(Core::EPropertyFlags::Transient) && (Property->HasFlag(Core::EPropertyFlags::Edit) || Property->HasFlag(Core::EPropertyFlags::ReadOnly));
        }

        // 숫자 변환은 Macaw.Engine의 ScriptFields와 같은 형식이어야 하므로 로캘과 무관한 charconv를 씁니다.
        float ParseScriptFloat(std::string_view Text) {
            float Value{};
            std::from_chars(Text.data(), Text.data() + Text.size(), Value);
            return Value;
        }

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
        bool DrawScriptFieldValue(const Script::FScriptFieldInfo& Info, Core::FString& Value) {
            const char* Label{Info.mName.c_str()};

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

    void Editor::FDetailsPanel::SetAssetServices(Editor::FEditorAssetCatalog* Catalog, Editor::FAssetThumbnailCache* Thumbnails) {
        mAssetCatalog = Catalog;
        mThumbnails = Thumbnails;
    }

    void Editor::FDetailsPanel::SetScriptBuildService(Editor::FScriptBuildService* Builds) {
        mScriptBuilds = Builds;
    }

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

    void Editor::FDetailsPanel::DrawScriptFields(Script::UScriptComponent* Component) {
        Editor::FEditorContext* Context{GetEditorContext()};
        const CoreUObject::FObjectHandle Handle{Component->GetHandle()};

        ImGui::SeparatorText("C# Script");

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
                Core::FString Value{Field != nullptr ? Field->mValue : Info.mDefault};

                ImGui::PushID(Info.mName.c_str());

                if (Info.mType == "Actor") {
                    const CoreUObject::UObject* Target{Field != nullptr ? Field->mObject.Resolve() : nullptr};

                    if (ImGui::BeginCombo(Info.mName.c_str(), Target != nullptr ? Target->GetName().c_str() : "(없음)")) {
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

                if (DrawScriptFieldValue(Info, Value)) {
                    if (Value == Info.mDefault) {
                        Context->RemoveScriptField(Handle, Info.mName);
                    } else {
                        Context->SetScriptField(Handle, {Info.mName, Info.mType, Value});
                    }
                }

                if (Field != nullptr) {
                    ImGui::SameLine();

                    if (ImGui::SmallButton("기본값")) {
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

            if (Known) {
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

        DrawComponentMenu();
        DrawComponentTree();

        const auto Objects{Context->GetDetailsObjects()};

        for (CoreUObject::FObjectHandle Handle : Objects) {
            if (CoreUObject::UObject* Object{Context->ResolveObject(Handle)}) {
                ImGui::PushID(Object);

                if (ImGui::CollapsingHeader(CoreUObject::Cast<Engine::AActor>(Object) != nullptr ? "Actor" : Object->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    DrawReflectedObject(Object);
                }

                ImGui::PopID();
            }
        }
    }

    void Editor::FDetailsPanel::DrawComponentMenu() {
        Editor::FEditorContext* Context{GetEditorContext()};
        const bool CanAdd{Context->CanEditWorld() && Context->GetSelectedActors().size() == 1};
        ImGui::BeginDisabled(!CanAdd);

        if (ImGui::Button("+ 추가")) {
            mComponentSearch.fill('\0');
            ImGui::OpenPopup("AddComponent");
        }

        ImGui::EndDisabled();

        if (!CanAdd && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("컴포넌트를 추가할 액터 하나를 선택하세요.");
        }

        if (const Engine::AActor* Actor{Context->GetSelectedActor()}) {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", Actor->GetName().c_str());
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

    void Editor::FDetailsPanel::DrawComponentTree() {
        Editor::FEditorContext* Context{GetEditorContext()};
        Engine::AActor* Actor{Context->GetSelectedActor()};

        if (Actor == nullptr || Context->GetSelectedActors().size() != 1) {
            return;
        }

        const CoreUObject::FObjectHandle ActorHandle{Actor->GetHandle()};
        const CoreUObject::UObject* Selected{Context->GetSelectedObject()};
        Core::TArray<CoreUObject::FObjectHandle> Components{};

        for (const auto& Component : Actor->GetComponents()) {
            Components.push_back(Component->GetHandle());
        }

        // 컴포넌트가 많으면 프로퍼티 영역을 밀어내지 않도록 트리 안에서 스크롤합니다.
        constexpr std::size_t MaxVisibleRows{8};
        const float RowHeight{ImGui::GetFontSize() + ImGui::GetStyle().CellPadding.y * 2.0f};
        const float Height{static_cast<float>(std::min(Components.size() + 1, MaxVisibleRows)) * RowHeight + 4.0f};
        constexpr ImGuiTableFlags TableFlags{ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_ScrollY};

        if (ImGui::BeginTable("ComponentTree", 2, TableFlags, {0.0f, Height})) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("StaticMeshComponent").x);

            ImGui::PushID(Actor);
            ImGui::TableNextRow();
            ObjectRow::DrawLabel(Actor, Selected == Actor);

            if (ImGui::IsItemClicked()) {
                Context->SelectObject(ActorHandle);
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                Context->OpenInSequencer(ActorHandle);
            }

            ObjectRow::DrawType(Actor);
            ImGui::PopID();
            ImGui::Indent();

            for (CoreUObject::FObjectHandle Handle : Components) {
                if (CoreUObject::UObject* Component{Context->ResolveObject(Handle)}) {
                    ImGui::PushID(Component);
                    ImGui::TableNextRow();
                    ObjectRow::DrawLabel(Component, Selected == Component);

                    if (ImGui::IsItemClicked()) {
                        Context->SelectObject(Handle);
                    }

                    ObjectRow::DrawType(Component);
                    ImGui::PopID();
                }
            }

            ImGui::Unindent();
            ImGui::EndTable();
        }

        ImGui::Separator();
    }

    /**
     * @brief 객체 타입의 편집 가능한 리플렉션 프로퍼티를 순회하여 표시합니다.
     */
    void Editor::FDetailsPanel::DrawReflectedObject(CoreUObject::UObject* Object) {
        Editor::FEditorContext* Context{GetEditorContext()};
        const CoreUObject::FObjectHandle Handle{Object->GetHandle()};

        ImGui::PushID(Object);
        ImGui::TextDisabled("%s", Object->GetClass()->GetName().c_str());

        Core::TArray<const CoreUObject::UClass*> Classes{};

        for (const CoreUObject::UClass* Class{Object->GetClass()}; Class != nullptr; Class = Class->GetSuperClass()) {
            Classes.push_back(Class);
        }

        std::reverse(Classes.begin(), Classes.end());

        for (const CoreUObject::UClass* Class : Classes) {
            const auto& Properties{Class->GetProperties()};

            if (std::none_of(Properties.begin(), Properties.end(), IsVisibleProperty)) {
                continue;
            }

            if (ImGui::TreeNodeEx(Class->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                for (const CoreUObject::FProperty* Property : Properties) {
                    if (!IsVisibleProperty(Property)) {
                        continue;
                    }

                    CoreUObject::FPropertyValue Value{Property->ReadValue(Object)};
                    const bool ReadOnly{!Context->CanEditProperty(Handle, Property)};

                    ImGui::BeginDisabled(ReadOnly);

                    if (DrawPropertyValue(Property, Value) && !ReadOnly) {
                        Context->EditProperty(Handle, Property, Value, ImGui::IsMouseDown(ImGuiMouseButton_Left));
                    }

                    ImGui::EndDisabled();

                    if (Context->ResolveObject(Handle) != Object) {
                        break;
                    }
                }

                ImGui::TreePop();
            }

            if (Context->ResolveObject(Handle) != Object) {
                break;
            }
        }

        if (const auto* Light{Context->ResolveObject(Handle) == Object ? CoreUObject::Cast<Engine::ULightComponent>(Object) : nullptr}; Light != nullptr && ImGui::CollapsingHeader("Shadow Depth Maps")) {
            const RenderCore::FShadowAtlasView Atlas{Context->GetRenderer() != nullptr ? Context->GetRenderer()->GetShadowAtlasView() : RenderCore::FShadowAtlasView{}};
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

        auto* Component{Context->ResolveObject(Handle) == Object ? CoreUObject::Cast<Engine::UMeshComponent>(Object) : nullptr};
        auto* Skeletal{Context->ResolveObject(Handle) == Object ? CoreUObject::Cast<Engine::USkeletalMeshComponent>(Object) : nullptr};

        if (Component != nullptr && Component->GetStaticMesh() != nullptr) {
            const Core::FString& Override{Component->GetMaterialPath()};
            const bool AllSlotsOverride{!Override.empty()};

            ImGui::SeparatorText("Material Slots");

            if (AllSlotsOverride) {
                ImGui::TextWrapped("Clear the all-slots override to edit individual slots.");
            }

            ImGui::BeginDisabled(!Context->CanEditWorld() || AllSlotsOverride);

            const Asset::FMeshData& Data{Component->GetStaticMesh()->GetMeshData()};
            const auto& Slots{Data.GetMaterialSlots()};

            for (Core::Uint32 Slot{}; Slot < Slots.size(); ++Slot) {
                const auto& SubMeshes{Data.GetSubMeshes()};

                if (std::none_of(SubMeshes.begin(), SubMeshes.end(), [Slot](const Asset::FSubMesh& SubMesh) {
                    return SubMesh.GetMaterialIndex() == Slot;
                })) {
                    continue;
                }

                ImGui::PushID(static_cast<int>(Slot));

                Core::FString Path{AllSlotsOverride ? Core::FString{} : Component->GetSlotMaterialPath(Slot)};
                const Core::FString& Inherited{AllSlotsOverride ? Override : Slots[Slot].GetMaterialPath()};

                if (mAssetCatalog != nullptr && mThumbnails != nullptr && mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, Slots[Slot].GetName().c_str(), Path, Asset::EAssetType::Material, Inherited, "Use mesh material", AllSlotsOverride ? "All slots" : "Mesh")) {
                    Context->SetMaterialOverride(Handle, Slot, Path);
                }

                ImGui::PopID();
            }

            ImGui::EndDisabled();
        }

        if (Skeletal != nullptr && Skeletal->GetSkeletalMesh() != nullptr) {
            DrawSkeletalPose(Skeletal);
        }

        if (auto* ScriptComponent{Context->ResolveObject(Handle) == Object ? CoreUObject::Cast<Script::UScriptComponent>(Object) : nullptr}) {
            DrawScriptFields(ScriptComponent);
        }

        ImGui::PopID();
    }

    /**
     * @brief 프로퍼티 타입에 맞는 편집 위젯을 표시하고 변경한 값을 돌려줍니다.
     */
    bool Editor::FDetailsPanel::DrawPropertyValue(const CoreUObject::FProperty* Property, CoreUObject::FPropertyValue& Value) {
        const char* Label{Property->GetName().c_str()};

        if (Property->GetType() == CoreUObject::EPropertyType::Enum) {
            Core::Int32 Number{std::get<Core::Int32>(Value)};
            const auto& Choices{Property->GetEnumValues()};
            const auto Current{std::find_if(Choices.begin(), Choices.end(), [Number](const CoreUObject::FEnumValue& Choice) {
                return Choice.mValue == Number;
            })};

            bool Changed{};

            if (ImGui::BeginCombo(Label, Current != Choices.end() ? Current->mName.c_str() : "Invalid")) {
                for (const CoreUObject::FEnumValue& Choice : Choices) {
                    if (ImGui::Selectable(Choice.mName.c_str(), Number == Choice.mValue)) {
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

                    if (ImGui::DragFloat4(std::to_string(Row).c_str(), Values.data(), 0.025f)) {
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
            if (Property->GetName() == "ScriptClass") {
                return DrawScriptClassPicker(Label, *String);
            }

            if (Property->GetName() == "SkeletalMesh" && mAssetCatalog != nullptr && mThumbnails != nullptr) {
                return mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, Label, *String, Asset::EAssetType::SkeletalMesh, {}, "None");
            }

            if (Property->GetName() == "Mesh") {
                return DrawAssetPicker(Label, *String, true);
            }

            if (mAssetCatalog != nullptr && mThumbnails != nullptr && (Property->GetName() == "Material" || Property->GetName() == "Texture" || Property->GetName() == "Sprite")) {
                return mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, Property->GetName() == "Material" ? "All Slots Override" : Label, *String, Property->GetName() == "Material" ? Asset::EAssetType::Material : Asset::EAssetType::Texture, {}, Property->GetName() == "Material" ? "Use mesh materials" : Property->GetName() == "Sprite" ? "Default atlas" : "None");
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

            bool Changed{ImGui::InputText(Label, Buffer.data(), Buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)};

            if (Changed) {
                *String = Buffer.data();
            }

            const char* AssetPayload{Property->GetName() == "Material" ? Editor::EditorAssetDragDrop::MaterialPayload : Property->GetName() == "Texture" || Property->GetName() == "Sprite" ? Editor::EditorAssetDragDrop::TexturePayload : Property->GetName() == "Mesh" ? Editor::EditorAssetDragDrop::MeshPayload : nullptr};

            if (AssetPayload != nullptr && ImGui::BeginDragDropTarget()) {
                const ImGuiPayload* Payload{ImGui::AcceptDragDropPayload(AssetPayload)};

                if (Payload != nullptr && Payload->DataSize > 0 && static_cast<const char*>(Payload->Data)[Payload->DataSize - 1] == '\0') {
                    *String = Core::FString{static_cast<const char*>(Payload->Data), static_cast<std::size_t>(Payload->DataSize - 1)};
                    Changed = true;
                }

                ImGui::EndDragDropTarget();
            }

            return Changed;
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
            ImGui::TextDisabled("%s", Label);
            ImGui::TextWrapped("%s", Guid->ToString().c_str());
        } else if (auto* Reference{std::get_if<CoreUObject::FObjectReference>(&Value)}) {
            ImGui::TextDisabled("%s", Label);
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

                    if (DrawPropertyValue(Child, ChildValue) && !ReadOnly) {
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

    bool Editor::FDetailsPanel::DrawAssetPicker(const char* Label, Core::FString& Path, bool Mesh) {
        if (mAssetCatalog == nullptr || mThumbnails == nullptr) {
            ImGui::TextDisabled("%s: %s", Label, Path.c_str());
            return false;
        }

        return mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, Label, Path, Mesh ? Asset::EAssetType::Mesh : Asset::EAssetType::Material, {}, Mesh ? "Primitive mesh" : "Default material");
    }

    void Editor::FDetailsPanel::DrawSkeletalPose(Engine::USkeletalMeshComponent* Component) {
        if (!ImGui::CollapsingHeader("Bone Local Pose")) {
            return;
        }

        ImGui::BeginDisabled(!GetEditorContext()->CanEditWorld());
        const auto& Bones{Component->GetSkeletalMesh()->GetBones()};

        for (Core::Uint32 Bone{}; Bone < Bones.size(); ++Bone) {
            ImGui::PushID(static_cast<int>(Bone));

            if (ImGui::TreeNode(Bones[Bone].GetName().c_str())) {
                // 적용 뒤 자식 포즈와 CPU 경계 및 그림자가 함께 갱신됩니다.
                Core::FMatrix Transform{Component->GetPose().GetLocalTransforms()[Bone]};
                bool Changed{};

                for (Core::Int32 Row{}; Row < 4; ++Row) {
                    std::array<float, 4> Values{Transform(Row, 0), Transform(Row, 1), Transform(Row, 2), Transform(Row, 3)};

                    if (ImGui::DragFloat4(std::to_string(Row).c_str(), Values.data(), 0.025f)) {
                        for (Core::Int32 Column{}; Column < 4; ++Column) {
                            Transform(Row, Column) = Values[static_cast<std::size_t>(Column)];
                        }

                        Changed = true;
                    }
                }

                if (ImGui::Button("Reset Bone")) {
                    Transform = Bones[Bone].GetBindPose();
                    Changed = true;
                }

                if (Changed) {
                    GetEditorContext()->SetBoneLocalTransform(Component->GetHandle(), Bone, Transform);
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::EndDisabled();
    }

}
