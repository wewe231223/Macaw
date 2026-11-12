#include "FAnimSequenceViewportClient.h"

#include "FAnimationPreviewScene.h"
#include "FBonePreview.h"
#include "Asset/USkeleton.h"
#include "Core/Math/FBox.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector4.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/FSceneViewport.h"
#include "Engine/UWorld.h"
#include "RenderCore/IRendererModule.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>

using namespace Editor;

FAnimSequenceViewportClient::FAnimSequenceViewportClient(RenderCore::IRendererModule* Renderer, Editor::FAnimationPreviewScene* Scene)
	: mRenderer{Renderer},
	  mScene{Scene},
	  mViewport{std::make_unique<Engine::FSceneViewport>(Renderer, this)} {
}

FAnimSequenceViewportClient::~FAnimSequenceViewportClient() = default;

void FAnimSequenceViewportClient::FrameMesh() {
    Engine::USkeletalMeshComponent* Component{mScene->GetComponent()};

    if (Component == nullptr) {
        return;
    }

    Component->SetLocalPose(Component->GetPose().GetLocalTransforms());
    Component->GetWorld()->SendAllEndOfFrameUpdates();

    const Core::FBox Bounds{Component->GetMeshData().GetBounds().Transformed(Component->GetWorldMatrix())};

    mCenter = (Bounds.mMinimum + Bounds.mMaximum) * 0.5f;
    mRadius = std::max(Bounds.mMinimum.Distance(Bounds.mMaximum) * 0.5f, 0.01f);
    mDistance = mRadius * 3.0f;
    mRotation = {-20.0f, 45.0f, 0.0f};
}

void FAnimSequenceViewportClient::SetDisplay(bool ShowBones, bool ShowGrid, bool Wireframe) {
    mShowBones = ShowBones;
    mShowGrid = ShowGrid;
    mWireframe = Wireframe;
}

void FAnimSequenceViewportClient::DrawUI(const Core::FVector2& Size) {
    if (mRenderer == nullptr || !mScene->IsReady()) {
        ImGui::TextWrapped("Select a skeletal mesh and its skeleton in Preview Settings to display this animation.");

        return;
    }

    if (Size.x < 1.0f || Size.y < 1.0f || !mViewport->Resize(static_cast<Core::Uint32>(Size.x), static_cast<Core::Uint32>(Size.y))) {
        return;
    }

    ImGui::InvisibleButton("##AnimationPreview", {Size.x, Size.y}, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);

    const bool Hovered{ImGui::IsItemHovered()};
    const bool Active{ImGui::IsItemActive()};
    const ImGuiIO& Input{ImGui::GetIO()};

    if (Active && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
        const Core::FQuat Rotation{mRotation.Quaternion()};

        mCenter += (Rotation.RightVector() * -Input.MouseDelta.x + Rotation.UpVector() * Input.MouseDelta.y) * (mDistance / std::max(Size.y, 1.0f));
    } else if (Active && (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) || ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))) {
        mRotation.yaw += Input.MouseDelta.x * 0.3f;
        mRotation.pitch = std::clamp(mRotation.pitch - Input.MouseDelta.y * 0.3f, -89.0f, 89.0f);
    }

    if (Hovered) {
        mDistance = std::clamp(mDistance * std::exp(-Input.MouseWheel * 0.15f), mRadius * 0.02f, mRadius * 1000.0f);

        if (!Input.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F)) {
            FrameMesh();
        }
    }

    const ImVec2 Origin{ImGui::GetItemRectMin()};
    const ImVec2 End{ImGui::GetItemRectMax()};
    ImGui::GetWindowDrawList()->AddImage(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(mViewport->GetRenderTarget()->GetNativeTexture()))}, Origin, End);
    ImGui::GetWindowDrawList()->AddText({Origin.x + 12.0f, End.y - ImGui::GetTextLineHeight() - 10.0f}, IM_COL32(220, 225, 235, 255), "Drag: orbit | Middle: pan | Wheel: zoom | F: frame");
}

void FAnimSequenceViewportClient::Render() {
    mViewport->Draw();
}

void FAnimSequenceViewportClient::Draw(Engine::FSceneViewport* Viewport) {
    Engine::UWorld* World{mScene->GetWorld()};
    Engine::USkeletalMeshComponent* Component{mScene->GetComponent()};

    if (mRenderer == nullptr || World == nullptr || World->GetScene() == nullptr || Component == nullptr) {
        return;
    }

    const Core::FVector2 Size{Viewport->GetSize()};
    const Core::FQuat Rotation{mRotation.Quaternion()};
    Engine::FSceneView View{};

    View.mSize = Size;
    View.mLocation = mCenter - Rotation.ForwardVector() * mDistance;
    View.mForward = Rotation.ForwardVector();
    View.mFieldOfView = 60.0f;
    View.mView = Core::FMatrix::LookTo(View.mLocation, View.mForward, Rotation.UpVector());
    View.mProjection = Core::FMatrix::Perspective(View.mFieldOfView, Size.x / Size.y, std::max(mRadius * 0.001f, 0.00001f), std::max(mDistance + mRadius * 3.0f, mRadius * 10.0f));
    View.mViewProjection = View.mView * View.mProjection;

    Engine::FPrimitiveDrawInterface Bones{};

    if (mShowBones && Component->GetSkeleton() != nullptr) {
        for (const Editor::FBoneShape& Shape : Editor::BuildBoneShapes(Component->GetSkeleton()->GetBones(), Component->GetPose().GetGlobalTransforms(), mRadius)) {
            Bones.DrawMesh({Shape.mMesh, Shape.mWorld * Component->GetWorldMatrix(), {0.45f, 0.8f, 1.0f, 1.0f}});
        }
    }

    Engine::FSceneViewFamily Family{World->GetScene(), Viewport->GetRenderTarget(), View};
    Family.mViewMode = mWireframe ? Engine::EViewMode::Wireframe : Engine::EViewMode::Lit;
    Family.mBackgroundOverlay = mShowBones ? &Bones : nullptr;
    Family.mShowFlags.mSky = false;
    Family.mShowFlags.mFog = false;
    Family.mShowFlags.mLightIcons = false;
    Family.mShowFlags.mFogIcons = false;
    Family.mShowFlags.mLightVisualizers = false;
    Family.mShowFlags.mSprites = false;
    Family.mShowFlags.mText = false;
    Family.mShowFlags.mGrid = mShowGrid;
    Family.mShowFlags.mAxes = mShowGrid;
    Family.mGridSpacing = mShowGrid ? std::max(std::pow(10.0f, std::floor(std::log10(mRadius)) - 1.0f), 0.001f) : 0.0f;

    mRenderer->BeginRenderingViewFamily(Family);
}
