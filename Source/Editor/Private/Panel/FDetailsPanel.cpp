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
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/FProject.h"
#include "Engine/FAssetManager.h"
#include "Engine/FAssetFile.h"
#include "Engine/UStaticMesh.h"
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

#include <imgui.h>
#include <algorithm>
#include <array>
#include <cctype>

/** @brief Details 패널에서 사용하는 컴포넌트 표시 이름 변환입니다. */
namespace {
    /**
     * @brief 클래스 접두사와 Component 접미사를 제거해 컴포넌트 메뉴 이름을 만듭니다.
     * @param[in] Class 생성하거나 이름을 표시할 리플렉션 클래스입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FString ComponentLabel(const UClass* Class) {
        FString Name{Class->GetName()};
        if (Name.starts_with("U")) {
            Name.erase(0, 1);
        }
        if (Name.ends_with("Component")) {
            Name.resize(Name.size() - 9);
        }
        FString Label{};
        for (std::size_t Index{}; Index < Name.size(); ++Index) {
            const auto Character{static_cast<unsigned char>(Name[Index])};
            if (Index > 0 && std::isupper(Character) && (std::islower(static_cast<unsigned char>(Name[Index - 1])) || (Index + 1 < Name.size() && std::islower(static_cast<unsigned char>(Name[Index + 1]))))) {
                Label.push_back(' ');
            }
            Label.push_back(Name[Index]);
        }
        return Label;
    }
}

/**
 * @brief FDetailsPanel의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FDetailsPanel::FDetailsPanel(FEditorContext* Context)
	: FEditorPanel{Context, "Details"} {
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FDetailsPanel::~FDetailsPanel() = default;

void FDetailsPanel::SetAssetServices(FEditorAssetCatalog* Catalog, FAssetThumbnailCache* Thumbnails) {
    mAssetCatalog = Catalog;
    mThumbnails = Thumbnails;
}

/**
 * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
 */
void FDetailsPanel::DrawContent(float) {
    FEditorContext* Context{GetEditorContext()};
    DrawComponentMenu();
    const auto Objects{Context->GetDetailsObjects()};

    for (FObjectHandle Handle : Objects) {
        if (UObject* Object{Context->ResolveObject(Handle)}) {
            ImGui::PushID(Object);

            if (ImGui::CollapsingHeader(Cast<AActor>(Object) != nullptr ? "Actor" : Object->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawReflectedObject(Object);
            }

            ImGui::PopID();
        }
    }
}

void FDetailsPanel::DrawComponentMenu() {
    FEditorContext* Context{GetEditorContext()};
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
    if (const AActor* Actor{Context->GetSelectedActor()}) {
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
        for (const UClass* Class : FReflectionRegistry::GetDerivedClasses(UActorComponent::StaticClass())) {
            const FString Label{ComponentLabel(Class)};
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
 * @brief 객체 타입의 편집 가능한 리플렉션 프로퍼티를 순회하여 표시합니다.
 */
void FDetailsPanel::DrawReflectedObject(UObject* Object) {
    FEditorContext* Context{GetEditorContext()};
    const FObjectHandle Handle{Object->GetHandle()};
    ImGui::PushID(Object);
    ImGui::TextDisabled("%s", Object->GetClass()->GetName().c_str());

    for (const FProperty* Property : Object->GetClass()->GetAllProperties()) {
        if (Property->HasFlag(EPropertyFlags::Transient) || (!Property->HasFlag(EPropertyFlags::Edit) && !Property->HasFlag(EPropertyFlags::ReadOnly))) {
            continue;
        }

        FPropertyValue Value{Property->ReadValue(Object)};
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

    if (auto* Component{Context->ResolveObject(Handle) == Object ? Cast<UStaticMeshComponent>(Object) : nullptr}; Component != nullptr && Component->GetStaticMesh() != nullptr) {
        const bool AllSlotsOverride{!Component->GetMaterialPath().empty()};
        ImGui::SeparatorText("Material Slots");
        if (AllSlotsOverride) {
            ImGui::TextWrapped("Clear the all-slots override to edit individual slots.");
        }
        ImGui::BeginDisabled(!Context->CanEditWorld() || AllSlotsOverride);
        const auto& Slots{Component->GetStaticMesh()->GetMeshData().GetMaterialSlots()};
        for (Uint32 Slot{}; Slot < Slots.size(); ++Slot) {
            const auto& SubMeshes{Component->GetStaticMesh()->GetMeshData().GetSubMeshes()};
            if (std::none_of(SubMeshes.begin(), SubMeshes.end(), [Slot](const FSubMesh& SubMesh) {
                return SubMesh.GetMaterialIndex() == Slot;
            })) {
                continue;
            }
            ImGui::PushID(static_cast<int>(Slot));
            FString Path{AllSlotsOverride ? FString{} : Component->GetSlotMaterialPath(Slot)};
            const FString& Inherited{AllSlotsOverride ? Component->GetMaterialPath() : Slots[Slot].GetMaterialPath()};
            if (mAssetCatalog != nullptr && mThumbnails != nullptr && mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, Slots[Slot].GetName().c_str(), Path, EAssetType::Material, Inherited, "Use mesh material", AllSlotsOverride ? "All slots" : "Mesh")) {
                Context->SetMaterialOverride(Handle, Slot, Path);
            }
            ImGui::PopID();
        }
        ImGui::EndDisabled();
    }
    ImGui::PopID();
}

/**
 * @brief 프로퍼티 타입에 맞는 편집 위젯을 표시하고 변경한 값을 돌려줍니다.
 */
bool FDetailsPanel::DrawPropertyValue(const FProperty* Property, FPropertyValue& Value) {
    const char* Label{Property->GetName().c_str()};

    if (Property->GetType() == EPropertyType::Enum) {
        Int32 Number{std::get<Int32>(Value)};
        const auto& Choices{Property->GetEnumValues()};
        const auto Current{std::find_if(Choices.begin(), Choices.end(), [Number](const FEnumValue& Choice) {
            return Choice.mValue == Number;
        })};
        bool Changed{};

        if (ImGui::BeginCombo(Label, Current != Choices.end() ? Current->mName.c_str() : "Invalid")) {
            for (const FEnumValue& Choice : Choices) {
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

    if (auto* Vector2{std::get_if<FVector2>(&Value)}) {
        auto Values{Vector2->ToArray()};

        if (ImGui::DragFloat2(Label, Values.data(), 0.025f)) {
            *Vector2 = {Values[0], Values[1]};

            return true;
        }
    } else if (auto* Vector4{std::get_if<FVector4>(&Value)}) {
        auto Values{Vector4->ToArray()};

        if (ImGui::DragFloat4(Label, Values.data(), 0.025f)) {
            *Vector4 = {Values[0], Values[1], Values[2], Values[3]};

            return true;
        }
    } else if (auto* Matrix{std::get_if<FMatrix>(&Value)}) {
        bool Changed{};

        if (ImGui::TreeNodeEx(Label, ImGuiTreeNodeFlags_DefaultOpen)) {
            for (Int32 Row{}; Row < 4; ++Row) {
                std::array<float, 4> Values{(*Matrix)(Row, 0), (*Matrix)(Row, 1), (*Matrix)(Row, 2), (*Matrix)(Row, 3)};

                if (ImGui::DragFloat4(std::to_string(Row).c_str(), Values.data(), 0.025f)) {
                    for (Int32 Column{}; Column < 4; ++Column) {
                        (*Matrix)(Row, Column) = Values[static_cast<std::size_t>(Column)];
                    }

                    Changed = true;
                }
            }

            ImGui::TreePop();
        }

        return Changed;
    } else if (auto* Vector{std::get_if<FVector>(&Value)}) {
        auto Values{Vector->ToArray()};

        if (ImGui::DragFloat3(Label, Values.data(), 0.025f, 0.0f, 0.0f, "%.3f")) {
            *Vector = {Values[0], Values[1], Values[2]};

            return true;
        }
    } else if (auto* Rotation{std::get_if<FRotator>(&Value)}) {
        auto Values{Rotation->ToArray()};

        if (ImGui::DragFloat3(Label, Values.data(), 0.25f, 0.0f, 0.0f, "%.2f")) {
            *Rotation = {Values[0], Values[1], Values[2]};

            return true;
        }
    } else if (auto* String{std::get_if<FString>(&Value)}) {
        if (Property->GetName() == "Mesh") {
            return DrawAssetPicker(Label, *String, true);
        }
        if (mAssetCatalog != nullptr && mThumbnails != nullptr && (Property->GetName() == "Material" || Property->GetName() == "Texture" || Property->GetName() == "Sprite")) {
            return mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, Property->GetName() == "Material" ? "All Slots Override" : Label, *String, Property->GetName() == "Material" ? EAssetType::Material : EAssetType::Texture, {}, Property->GetName() == "Material" ? "Use mesh materials" : Property->GetName() == "Sprite" ? "Default atlas" : "None");
        }
        if (Property->GetName() == "Text") {
            TArray<char> TextBuffer{};
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
        const char* AssetPayload{Property->GetName() == "Material" ? EditorAssetDragDrop::MaterialPayload : Property->GetName() == "Texture" || Property->GetName() == "Sprite" ? EditorAssetDragDrop::TexturePayload : Property->GetName() == "Mesh" ? EditorAssetDragDrop::MeshPayload : nullptr};
        if (AssetPayload != nullptr && ImGui::BeginDragDropTarget()) {
            const ImGuiPayload* Payload{ImGui::AcceptDragDropPayload(AssetPayload)};
            if (Payload != nullptr && Payload->DataSize > 0 && static_cast<const char*>(Payload->Data)[Payload->DataSize - 1] == '\0') {
                *String = FString{static_cast<const char*>(Payload->Data), static_cast<std::size_t>(Payload->DataSize - 1)};
                Changed = true;
            }
            ImGui::EndDragDropTarget();
        }
        return Changed;
    } else if (auto* Boolean{std::get_if<bool>(&Value)}) {
        return ImGui::Checkbox(Label, Boolean);
    } else if (auto* FloatValue{std::get_if<float>(&Value)}) {
        return ImGui::DragFloat(Label, FloatValue, 0.025f);
    } else if (auto* IntegerValue{std::get_if<Int32>(&Value)}) {
        return ImGui::InputInt(Label, IntegerValue);
    } else if (auto* UnsignedValue{std::get_if<Uint32>(&Value)}) {
        return ImGui::InputScalar(Label, ImGuiDataType_U32, UnsignedValue);
    } else if (auto* Unsigned64Value{std::get_if<Uint64>(&Value)}) {
        return ImGui::InputScalar(Label, ImGuiDataType_U64, Unsigned64Value);
    } else if (auto* Guid{std::get_if<FGuid>(&Value)}) {
        ImGui::TextDisabled("%s", Label);
        ImGui::TextWrapped("%s", Guid->ToString().c_str());
    } else if (const UStruct* Struct{Property->GetStruct()}) {
        bool Changed{};

        if (ImGui::TreeNodeEx(Label, ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const FProperty* Child : Struct->GetProperties()) {
                if (Child->HasFlag(EPropertyFlags::Transient)) {
                    continue;
                }

                FPropertyValue ChildValue{Child->ReadValue(GetPropertyData(Value))};
                const bool ReadOnly{Child->HasFlag(EPropertyFlags::ReadOnly)};
                ImGui::BeginDisabled(ReadOnly);

                if (DrawPropertyValue(Child, ChildValue) && !ReadOnly) {
                    Changed = Child->WriteValue(GetPropertyData(Value), ChildValue) || Changed;
                }

                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }

        return Changed;
    }

    return false;
}

bool FDetailsPanel::DrawAssetPicker(const char* Label, FString& Path, bool Mesh) {
    if (mAssetCatalog == nullptr || mThumbnails == nullptr) {
        ImGui::TextDisabled("%s: %s", Label, Path.c_str());
        return false;
    }
    return mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, Label, Path, Mesh ? EAssetType::Mesh : EAssetType::Material, {}, Mesh ? "Primitive mesh" : "Default material");
}
