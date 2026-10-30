/**
 * @file FMeshViewer.cpp
 * @brief 독립 메시 미리보기와 궤도 카메라의 구현입니다.
 * @ingroup Editor
 */
#include "Editor/FMeshViewer.h"
#include "Core/FilePath.h"
#include "Core/Math/FBox.h"
#include "Core/Math/FQuat.h"
#include "Engine/FAssetManager.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/FLightSceneProxy.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/UStaticMesh.h"
#include "Engine/UMaterial.h"
#include "RenderCore/IRendererModule.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <d3d11.h>

FMeshViewer::FMeshViewer(IRendererModule* Renderer, FAssetManager* Assets)
	: mRenderer{Renderer},
	  mAssets{Assets} {
}

FMeshViewer::~FMeshViewer() = default;

bool FMeshViewer::Open(const std::filesystem::path& Path, FString& Error) {
    Error.clear();

    if (mAssets == nullptr || mRenderer == nullptr) {
        Error = "The mesh viewer requires a renderer and an asset manager";
        return false;
    }

    UStaticMesh* Mesh{mAssets->LoadMesh(Path, Error)};

    if (Mesh == nullptr) {
        return false;
    }

    if (!Mesh->CreateResources(static_cast<ID3D11Device*>(mRenderer->GetNativeDevice()))) {
        Error = "Cannot create the mesh preview resources";
        return false;
    }

    auto Scene{mRenderer->AllocateScene()};

    if (Scene == nullptr) {
        Error = "Cannot allocate the mesh preview scene";
        return false;
    }

    TArray<UMaterial*> Materials{};

    for (const FMeshMaterialSlot& Slot : Mesh->GetMeshData().GetMaterialSlots()) {
        UMaterial* Material{Slot.GetMaterialPath().empty() ? nullptr : mAssets->LoadMaterial(FilePath::FromUtf8(Slot.GetMaterialPath()), Error)};

        if (!Slot.GetMaterialPath().empty() && Material == nullptr) {
            return false;
        }

        Materials.push_back(Material);
    }

    mScene = std::move(Scene);

    if (mScene != nullptr) {
        FLightSceneProxy Ambient{};
        Ambient.mIntensity = 0.25f;
        mScene->AddLight(Ambient);

        FLightSceneProxy Directional{};
        Directional.mType = ELightType::Directional;
        Directional.mDirection = FVector{0.35f, 0.6f, -1.0f}.Normalized();
        Directional.mIntensity = 0.75f;
        mScene->AddLight(Directional);
    }

    mMesh = Mesh;
    mMaterials = std::move(Materials);
    mMaterialPaths.assign(mMaterials.size(), FString{});
    mPath = FilePath::ToGenericUtf8(Path);
    mError.clear();
    mOpen = true;
    mMouseCaptured = false;
    FrameMesh();
    UpdateScene();

    return true;
}

bool FMeshViewer::IsOpen() const {
    return mOpen;
}

void FMeshViewer::FrameMesh() {
    if (mMesh == nullptr) {
        return;
    }

    const FBox& Bounds{mMesh->GetBounds()};
    mCenter = (Bounds.mMinimum + Bounds.mMaximum) * 0.5f;
    mRadius = std::max(Bounds.mMinimum.Distance(Bounds.mMaximum) * 0.5f, 0.01f);
    mDistance = mRadius * 3.0f;
    mRotation = {-20.0f, 45.0f, 0.0f};
}

void FMeshViewer::UpdateScene() {
    FPrimitiveSceneProxy Proxy{};
    Proxy.mId = mPrimitiveId;
    Proxy.mMesh = mMesh;
    Proxy.mBounds = mMesh->GetBounds();
    Proxy.mMaterials = mMaterials;
    mScene->AddPrimitive(Proxy);
}

void FMeshViewer::DrawMaterials() {
    if (!ImGui::CollapsingHeader("Material Slots")) {
        return;
    }

    FString Error{};
    const auto Paths{mAssets->GetAssetPaths(Error)};
    const auto& Slots{mMesh->GetMeshData().GetMaterialSlots()};

    for (Uint32 Slot{}; Slot < Slots.size(); ++Slot) {
        ImGui::PushID(static_cast<int>(Slot));

        FString Path{mMaterialPaths[Slot]};
        bool Changed{};

        if (ImGui::BeginCombo(Slots[Slot].GetName().c_str(), Path.empty() ? "Mesh Default" : Path.c_str())) {
            if (ImGui::Selectable("Mesh Default", Path.empty())) {
                Path.clear();
                Changed = true;
            }

            for (const auto& Candidate : Paths) {
                if (Candidate.extension() == L".mtl") {
                    const FString Relative{FilePath::ToGenericUtf8(Candidate)};

                    if (ImGui::Selectable(Relative.c_str(), Path == Relative)) {
                        Path = Relative;
                        Changed = true;
                    }
                }
            }

            ImGui::EndCombo();
        }

        if (Changed) {
            const FString Resolved{Path.empty() ? Slots[Slot].GetMaterialPath() : Path};
            UMaterial* Material{Resolved.empty() ? nullptr : mAssets->LoadMaterial(FilePath::FromUtf8(Resolved), mError)};

            if (Resolved.empty() || Material != nullptr) {
                mMaterials[Slot] = Material;
                mMaterialPaths[Slot] = Path;
                UpdateScene();
            }
        }

        ImGui::PopID();
    }
}

void FMeshViewer::DrawUI(bool Standalone) {
    if (!mOpen || mMesh == nullptr) {
        return;
    }

    ImGuiWindowFlags Flags{ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse};

    if (Standalone) {
        const ImGuiViewport* Viewport{ImGui::GetMainViewport()};
        ImGui::SetNextWindowPos(Viewport->WorkPos);
        ImGui::SetNextWindowSize(Viewport->WorkSize);
        Flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    } else {
        ImGui::SetNextWindowSize({860.0f, 700.0f}, ImGuiCond_FirstUseEver);
    }

    if (ImGui::Begin("Static Mesh Viewer", Standalone ? nullptr : &mOpen, Flags)) {
        ImGui::TextUnformatted(mPath.c_str());
        ImGui::Text("Vertices: %u | Triangles: %u | SubMeshes: %zu | Materials: %zu", mMesh->GetVertexCount(), mMesh->GetIndexCount() / 3, mMesh->GetMeshData().GetSubMeshes().size(), mMaterials.size());

        if (ImGui::Button("Frame Mesh")) {
            FrameMesh();
        }

        ImGui::SameLine();

        bool Wireframe{mViewMode == EViewMode::Wireframe};

        if (ImGui::Checkbox("Wireframe", &Wireframe)) {
            mViewMode = Wireframe ? EViewMode::Wireframe : EViewMode::Lit;
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Drag: orbit | Middle: pan | Wheel: zoom | F: frame");
        DrawMaterials();

        if (!mError.empty()) {
            ImGui::TextWrapped("%s", mError.c_str());
        }

        DrawPreview();
    } else {
        mMouseCaptured = false;
    }

    ImGui::End();
}

void FMeshViewer::DrawPreview() {
    const ImVec2 Size{ImGui::GetContentRegionAvail()};

    if (Size.x < 1.0f || Size.y < 1.0f) {
        mMouseCaptured = false;
        return;
    }

    const Uint32 Width{static_cast<Uint32>(Size.x)};
    const Uint32 Height{static_cast<Uint32>(Size.y)};

    if (mTarget == nullptr) {
        mTarget = mRenderer->CreateRenderTarget(Width, Height);
    } else if ((mTarget->GetWidth() != Width || mTarget->GetHeight() != Height) && !mRenderer->ResizeRenderTarget(mTarget.get(), Width, Height)) {
        return;
    }

    if (mTarget == nullptr) {
        return;
    }

    ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(mTarget->GetNativeTexture()))}, Size);

    const bool Hovered{ImGui::IsItemHovered()};
    const ImGuiIO& Input{ImGui::GetIO()};

    if (Hovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) {
        mMouseCaptured = true;
        ImGui::SetWindowFocus();
    }

    if (!ImGui::IsWindowFocused() || (!Input.MouseDown[0] && !Input.MouseDown[1] && !Input.MouseDown[2])) {
        mMouseCaptured = false;
    }

    if (mMouseCaptured) {
        if (Input.MouseDown[2]) {
            const FQuat Rotation{mRotation.Quaternion()};
            mCenter += (Rotation.RightVector() * -Input.MouseDelta.x + Rotation.UpVector() * Input.MouseDelta.y) * (mDistance / std::max(Size.y, 1.0f));
        } else {
            mRotation.yaw += Input.MouseDelta.x * 0.3f;
            mRotation.pitch = std::clamp(mRotation.pitch - Input.MouseDelta.y * 0.3f, -89.0f, 89.0f);
        }
    }

    if (Hovered && Input.MouseWheel != 0.0f) {
        mDistance = std::clamp(mDistance * std::exp(-Input.MouseWheel * 0.15f), mRadius * 0.02f, mRadius * 1000.0f);
    }

    if (Hovered && !Input.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F)) {
        FrameMesh();
    }

    const FQuat Rotation{mRotation.Quaternion()};
    FSceneView View{};
    View.mSize = {Size.x, Size.y};
    View.mLocation = mCenter - Rotation.ForwardVector() * mDistance;
    View.mForward = Rotation.ForwardVector();
    View.mView = FMatrix::LookTo(View.mLocation, View.mForward, Rotation.UpVector());
    View.mProjection = FMatrix::Perspective(60.0f, Size.x / Size.y, std::max(mRadius * 0.001f, 0.00001f), std::max(mDistance + mRadius * 3.0f, mRadius * 10.0f));
    View.mViewProjection = View.mView * View.mProjection;

    const FSceneViewFamily Family{mScene.get(), mTarget.get(), View, nullptr, mViewMode};
    mRenderer->BeginRenderingViewFamily(Family);
}
