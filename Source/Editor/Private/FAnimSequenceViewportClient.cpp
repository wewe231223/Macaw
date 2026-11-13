/**
 * @file FAnimSequenceViewportClient.cpp
 * @brief 애니메이션 뷰어의 이미지 표시, 카메라 입력과 오버레이를 연결합니다.
 * @ingroup Editor
 */

#include "FAnimSequenceViewportClient.h"

#include "Viewer/FAnimationViewerContent.h"
#include "Viewer/FBonePreview.h"
#include "Asset/USkeleton.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Engine/FPrimitiveDrawInterface.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>

using namespace Editor;

FAnimSequenceViewportClient::FAnimSequenceViewportClient(RenderCore::IRendererModule* Renderer, std::unique_ptr<Viewer::FAnimationViewerContent> Scene)
	: mViewer{Renderer, nullptr},
	  mScene{Scene.get()} {
    mViewer.SetContent(std::move(Scene));
}

FAnimSequenceViewportClient::~FAnimSequenceViewportClient() = default;

void FAnimSequenceViewportClient::FrameMesh() {
    mViewer.GetCamera().SetRotation({-20.0f, 45.0f, 0.0f});
    mViewer.Fit();
    mNeedsFit = true;
}

void FAnimSequenceViewportClient::SetDisplay(bool ShowBones, bool ShowGrid, bool Wireframe) {
    mShowBones = ShowBones;
    mShowGrid = ShowGrid;
    mWireframe = Wireframe;
}

void FAnimSequenceViewportClient::DrawUI(const Core::FVector2& Size) {
    if (!mScene->IsReady()) {
        ImGui::TextWrapped("Select a skeletal mesh and its skeleton in Preview Settings to display this animation.");

        return;
    }

    if (Size.x < 1.0f || Size.y < 1.0f || !mViewer.Resize(static_cast<Core::Uint32>(Size.x), static_cast<Core::Uint32>(Size.y))) {
        return;
    }

    if (mNeedsFit) {
        mNeedsFit = !mViewer.Fit();
    }

    ImGui::InvisibleButton("##AnimationPreview", {Size.x, Size.y}, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);

    const bool Hovered{ImGui::IsItemHovered()};
    const bool Active{ImGui::IsItemActive()};
    const ImGuiIO& Input{ImGui::GetIO()};

    if (Active && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
        mViewer.GetCamera().Pan({Input.MouseDelta.x, Input.MouseDelta.y}, Size.y);
    } else if (Active && (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) || ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))) {
        mViewer.GetCamera().Orbit({Input.MouseDelta.x, Input.MouseDelta.y});
    }

    if (Hovered) {
        mViewer.GetCamera().Zoom(Input.MouseWheel);

        if (!Input.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F)) {
            FrameMesh();
        }
    }

    const ImVec2 Origin{ImGui::GetItemRectMin()};
    const ImVec2 End{ImGui::GetItemRectMax()};

    ImGui::GetWindowDrawList()->AddImage(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(mViewer.GetNativeTexture()))}, Origin, End);
    ImGui::GetWindowDrawList()->AddText({Origin.x + 12.0f, End.y - ImGui::GetTextLineHeight() - 10.0f}, IM_COL32(220, 225, 235, 255), "Drag: orbit | Middle: pan | Wheel: zoom | F: frame");
}

void FAnimSequenceViewportClient::Render() {
    Engine::USkeletalMeshComponent* Component{mScene->GetComponent()};

    if (Component == nullptr) {
        return;
    }

    const float Radius{mViewer.GetCamera().GetRadius()};
    Engine::FPrimitiveDrawInterface Bones{};

    if (mShowBones && Component->GetSkeleton() != nullptr) {
        for (const Viewer::FBoneShape& Shape : Viewer::BuildBoneShapes(Component->GetSkeleton()->GetBones(), Component->GetPose().GetGlobalTransforms(), Radius)) {
            Bones.DrawMesh({Shape.GetMesh(), Shape.GetWorld() * Component->GetWorldMatrix(), {0.45f, 0.8f, 1.0f, 1.0f}});
        }
    }

    mViewer.SetViewMode(mWireframe ? Engine::EViewMode::Wireframe : Engine::EViewMode::Lit);
    mViewer.SetGridSpacing(mShowGrid ? std::max(std::pow(10.0f, std::floor(std::log10(Radius)) - 1.0f), 0.001f) : 0.0f);
    mViewer.Render(nullptr, mShowBones ? &Bones : nullptr);
}
