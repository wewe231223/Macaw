/**
 * @file ObjectRow.cpp
 * @brief Outliner·Details가 공유하는 객체 아이콘·색상·선택 행을 그립니다.
 * @ingroup Editor
 */

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

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 컴포넌트의 실제 클래스에 맞는 아이콘을 고릅니다.
         * @param[in] Component 아이콘을 결정할 컴포넌트입니다.
         * @return 컴포넌트 종류에 대응하는 아이콘입니다.
         */
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

        /**
         * @brief 액터 또는 컴포넌트의 대표 아이콘을 고릅니다.
         * @param[in] Object 조회하거나 호출할 엔진·COM 객체입니다.
         * @return 객체를 대표하는 에디터 아이콘입니다.
         */
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

        /**
         * @brief 객체 종류별 아이콘의 표시 색상을 구합니다.
         * @param[in] Icon 표시 색상을 구할 아이콘 종류입니다.
         * @return 현재 테마에서 선택한 아이콘의 32비트 표시 색상입니다.
         */
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

    /**
     * @brief 클래스 접두사를 뺀 타입 이름을 만듭니다.
     */
    Core::FString Editor::ObjectRow::GetTypeLabel(const CoreUObject::UObject* Object) {
        Core::FString Name{Object->GetClass()->GetName()};

        if (Name.size() > 1 && (Name[0] == 'A' || Name[0] == 'U') && std::isupper(static_cast<unsigned char>(Name[1]))) {
            Name.erase(0, 1);
        }

        return Name;
    }

    /**
     * @brief 현재 행의 다음 컬럼에 종류별 아이콘과 이름을 표시합니다.
     */
    void Editor::ObjectRow::DrawLabel(CoreUObject::UObject* Object, bool Selected, float Indent) {
        ImGui::TableNextColumn();

        const float X{ImGui::GetCursorScreenPos().x + ImGui::GetTreeNodeToLabelSpacing() + Indent};

        // 라벨 없는 잎 노드로 행 전체의 선택 영역을 만들고 아이콘과 이름을 덧그립니다.
        ImGui::TreeNodeEx("##Row", ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (Selected ? ImGuiTreeNodeFlags_Selected : 0));

        const EditorStyle::EIcon Icon{ObjectIcon(Object)};
        const float Size{ImGui::GetFontSize()};
        const float Y{(ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y - Size) * 0.5f};

        EditorStyle::DrawIcon(Icon, {X, Y}, Size, IconColor(Icon));
        ImGui::GetWindowDrawList()->AddText({X + Size + ImGui::GetStyle().ItemInnerSpacing.x, Y}, ImGui::GetColorU32(ImGuiCol_Text), Object->GetName().c_str());
    }

    /**
     * @brief 현재 행의 다음 컬럼에 타입 이름을 표시합니다.
     */
    void Editor::ObjectRow::DrawType(const CoreUObject::UObject* Object) {
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%s", GetTypeLabel(Object).c_str());
    }

}
