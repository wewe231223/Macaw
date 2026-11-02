#include "Panel/ObjectRow.h"
#include "EditorStyle.h"
#include "Engine/AActor.h"
#include "Engine/ALevelSequenceActor.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/UBillboardComponent.h"
#include "Engine/Component/UBillboardTextComponent.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/Component/UHeightFogComponent.h"
#include "Engine/Component/ULightComponentBase.h"
#include "Engine/Component/UMeshComponent.h"
#include "Engine/Component/UProjectileMovementComponent.h"
#include "Engine/Component/URotationMovementComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/Component/UShapeComponent.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Engine/Component/USubUVComponent.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/UClass.h"

#include <imgui.h>
#include <cctype>

namespace Editor {

    namespace {
        EditorStyle::EIcon ComponentIcon(CoreUObject::UObject* Component) {
            using EIcon = EditorStyle::EIcon;

            if (CoreUObject::Cast<Engine::ULightComponentBase>(Component) != nullptr) {
                return EIcon::Light;
            }

            if (CoreUObject::Cast<Engine::UCameraComponent>(Component) != nullptr) {
                return EIcon::Camera;
            }

            if (CoreUObject::Cast<Engine::USkeletalMeshComponent>(Component) != nullptr) {
                return EIcon::Skeleton;
            }

            if (CoreUObject::Cast<Engine::UMeshComponent>(Component) != nullptr) {
                return EIcon::Local;
            }

            if (CoreUObject::Cast<Engine::UShapeComponent>(Component) != nullptr) {
                return EIcon::Collision;
            }

            if (CoreUObject::Cast<Engine::UBillboardTextComponent>(Component) != nullptr) {
                return EIcon::Text;
            }

            if (CoreUObject::Cast<Engine::UBillboardComponent>(Component) != nullptr || CoreUObject::Cast<Engine::USubUVComponent>(Component) != nullptr) {
                return EIcon::Sprite;
            }

            if (CoreUObject::Cast<Engine::UHeightFogComponent>(Component) != nullptr) {
                return EIcon::Fog;
            }

            if (CoreUObject::Cast<Engine::UProjectileMovementComponent>(Component) != nullptr || CoreUObject::Cast<Engine::URotationMovementComponent>(Component) != nullptr) {
                return EIcon::Translate;
            }

            return EIcon::Component;
        }

        // 액터의 종류는 대개 루트 컴포넌트가 결정합니다.
        EditorStyle::EIcon ObjectIcon(CoreUObject::UObject* Object) {
            if (CoreUObject::Cast<Engine::ALevelSequenceActor>(Object) != nullptr) {
                return EditorStyle::EIcon::Sequence;
            }

            if (auto* Actor{CoreUObject::Cast<Engine::AActor>(Object)}) {
                Engine::USceneComponent* Root{Actor->GetRootComponent()};

                return Root != nullptr ? ComponentIcon(Root) : EditorStyle::EIcon::Component;
            }

            return ComponentIcon(Object);
        }

        ImU32 IconColor(EditorStyle::EIcon Icon) {
            switch (Icon) {
            case EditorStyle::EIcon::Light:
                return ImGui::GetColorU32(ImVec4{0.95f, 0.80f, 0.35f, 1.0f});
            case EditorStyle::EIcon::Camera:
                return ImGui::GetColorU32(ImVec4{0.45f, 0.70f, 0.95f, 1.0f});
            case EditorStyle::EIcon::Local:
                return ImGui::GetColorU32(ImVec4{0.60f, 0.85f, 0.85f, 1.0f});
            case EditorStyle::EIcon::Skeleton:
                return ImGui::GetColorU32(ImVec4{0.85f, 0.65f, 0.95f, 1.0f});
            case EditorStyle::EIcon::Collision:
                return ImGui::GetColorU32(ImVec4{0.50f, 0.85f, 0.50f, 1.0f});
            default:
                return ImGui::GetColorU32(ImGuiCol_Text);
            }
        }
    }

    Core::FString Editor::ObjectRow::GetTypeLabel(const CoreUObject::UObject* Object) {
        Core::FString Name{Object->GetClass()->GetName()};

        if (Name.size() > 1 && (Name[0] == 'A' || Name[0] == 'U') && std::isupper(static_cast<unsigned char>(Name[1]))) {
            Name.erase(0, 1);
        }

        return Name;
    }

    void Editor::ObjectRow::DrawLabel(CoreUObject::UObject* Object, bool Selected) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        const float X{ImGui::GetCursorScreenPos().x + ImGui::GetTreeNodeToLabelSpacing()};

        // 라벨 없는 잎 노드로 행 전체의 선택 영역을 만들고 아이콘과 이름을 덧그립니다.
        ImGui::TreeNodeEx("##Row", ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (Selected ? ImGuiTreeNodeFlags_Selected : 0));

        const EditorStyle::EIcon Icon{ObjectIcon(Object)};
        const float Size{ImGui::GetFontSize()};
        const float Y{(ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y - Size) * 0.5f};

        EditorStyle::DrawIcon(Icon, {X, Y}, Size, IconColor(Icon));
        ImGui::GetWindowDrawList()->AddText({X + Size + ImGui::GetStyle().ItemInnerSpacing.x, Y}, ImGui::GetColorU32(ImGuiCol_Text), Object->GetName().c_str());
    }

    void Editor::ObjectRow::DrawType(const CoreUObject::UObject* Object) {
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%s", GetTypeLabel(Object).c_str());
    }

}
