/**
 * @file FMeshViewer.cpp
 * @brief 독립 메시 미리보기와 궤도 카메라의 구현입니다.
 * @ingroup Editor
 */
#include "Editor/FMeshViewer.h"
#include "Core/FilePath.h"
#include "Core/Math/FBox.h"
#include "Core/Math/FQuat.h"
#include "Asset/FAssetManager.h"
#include "Asset/FAssetFile.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/FLightSceneProxy.h"
#include "Engine/FSceneViewFamily.h"
#include "Asset/UStaticMesh.h"
#include "Asset/USkeletalMesh.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Asset/UMaterial.h"
#include "RenderCore/IRendererModule.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <d3d11.h>

namespace Editor {

    Editor::FMeshViewer::FMeshViewer(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets)
		: mRenderer{Renderer},
		  mAssets{Assets} {
    }

    Editor::FMeshViewer::~FMeshViewer() = default;

    bool Editor::FMeshViewer::Open(const std::filesystem::path& Path, Core::FString& Error) {
        Error.clear();

        if (mAssets == nullptr || mRenderer == nullptr) {
            Error = "The mesh viewer requires a renderer and an asset manager";
            return false;
        }

        Asset::EAssetType Type{};

        if (!Asset::FAssetFile::GetType(mAssets->ResolvePath(Path, Error), Type, Error)) {
            return false;
        }

        Asset::USkeletalMesh* Skeletal{Type == Asset::EAssetType::SkeletalMesh ? mAssets->LoadSkeletalMesh(Path, Error) : nullptr};
        Asset::UStaticMesh* Mesh{Skeletal != nullptr ? Skeletal->GetGeometry() : Type == Asset::EAssetType::Mesh ? mAssets->LoadMesh(Path, Error) : nullptr};
        Engine::FSkeletalMeshPose Pose{};

        if (Skeletal != nullptr && (!Pose.Reset(*Skeletal) || !Skeletal->CreateResources(static_cast<ID3D11Device*>(mRenderer->GetNativeDevice())))) {
            Error = "Cannot prepare skeletal mesh pose or resources";
            return false;
        }

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

        Core::TArray<Asset::UMaterial*> Materials{};

        for (const Asset::FMeshMaterialSlot& Slot : Mesh->GetMeshData().GetMaterialSlots()) {
            Asset::UMaterial* Material{Slot.GetMaterialPath().empty() ? nullptr : mAssets->LoadMaterial(Core::FilePath::FromUtf8(Slot.GetMaterialPath()), Error)};

            if (!Slot.GetMaterialPath().empty() && Material == nullptr) {
                return false;
            }

            Materials.push_back(Material);
        }

        mScene = std::move(Scene);

        if (mScene != nullptr) {
            Engine::FLightSceneProxy Ambient{};
            Ambient.mIntensity = 0.25f;
            mScene->AddLight(Ambient);

            Engine::FLightSceneProxy Directional{};
            Directional.mType = Engine::ELightType::Directional;
            Directional.mDirection = Core::FVector{0.35f, 0.6f, -1.0f}.Normalized();
            Directional.mIntensity = 0.75f;
            mScene->AddLight(Directional);
        }

        mMesh = Mesh;
        mSkeletalMesh = Skeletal;
        mPose = std::move(Pose);
        mPoseBone = 0;
        mGizmo.EndDrag();
        mMaterials = std::move(Materials);
        mMaterialPaths.assign(mMaterials.size(), Core::FString{});
        mPath = Core::FilePath::ToGenericUtf8(Path);
        mError.clear();
        mOpen = true;
        mMouseCaptured = false;
        FrameMesh();
        UpdateScene();

        return true;
    }

    bool Editor::FMeshViewer::IsOpen() const {
        return mOpen;
    }

    void Editor::FMeshViewer::FrameMesh() {
        if (mMesh == nullptr) {
            return;
        }

        const Core::FBox& Bounds{mSkeletalMesh != nullptr ? mPose.GetMeshData().GetBounds() : mMesh->GetBounds()};
        mCenter = (Bounds.mMinimum + Bounds.mMaximum) * 0.5f;
        mRadius = std::max(Bounds.mMinimum.Distance(Bounds.mMaximum) * 0.5f, 0.01f);
        mDistance = mRadius * 3.0f;
        mRotation = {-20.0f, 45.0f, 0.0f};
    }

    void Editor::FMeshViewer::UpdateScene() {
        Engine::FPrimitiveSceneProxy Proxy{};
        Proxy.mId = mPrimitiveId;
        Proxy.mMesh = mMesh;
        Proxy.mBounds = mSkeletalMesh != nullptr ? mPose.GetMeshData().GetBounds() : mMesh->GetBounds();
        Proxy.mSkeletalMesh = mSkeletalMesh;
        Proxy.mSkinMatrices = mPose.GetSkinMatrices();
        Proxy.mPoseRevision = mPose.GetRevision();
        Proxy.mMaterials = mMaterials;
        mScene->AddPrimitive(Proxy);
    }

    void Editor::FMeshViewer::DrawMaterials() {
        if (!ImGui::CollapsingHeader("Material Slots")) {
            return;
        }

        Core::FString Error{};
        const auto Paths{mAssets->GetAssetPaths(Error)};
        const auto& Slots{mMesh->GetMeshData().GetMaterialSlots()};

        for (Core::Uint32 Slot{}; Slot < Slots.size(); ++Slot) {
            ImGui::PushID(static_cast<int>(Slot));

            Core::FString Path{mMaterialPaths[Slot]};
            bool Changed{};

            if (ImGui::BeginCombo(Slots[Slot].GetName().c_str(), Path.empty() ? "Mesh Default" : Path.c_str())) {
                if (ImGui::Selectable("Mesh Default", Path.empty())) {
                    Path.clear();
                    Changed = true;
                }

                for (const auto& Candidate : Paths) {
                    Asset::EAssetType Type{};

                    if (Asset::FAssetFile::GetType(mAssets->ResolvePath(Candidate, Error), Type, Error) && Type == Asset::EAssetType::Material) {
                        const Core::FString Relative{Core::FilePath::ToGenericUtf8(Candidate)};

                        if (ImGui::Selectable(Relative.c_str(), Path == Relative)) {
                            Path = Relative;
                            Changed = true;
                        }
                    }
                }

                ImGui::EndCombo();
            }

            if (Changed) {
                const Core::FString Resolved{Path.empty() ? Slots[Slot].GetMaterialPath() : Path};
                Asset::UMaterial* Material{Resolved.empty() ? nullptr : mAssets->LoadMaterial(Core::FilePath::FromUtf8(Resolved), mError)};

                if (Resolved.empty() || Material != nullptr) {
                    mMaterials[Slot] = Material;
                    mMaterialPaths[Slot] = Path;
                    UpdateScene();
                }
            }

            ImGui::PopID();
        }
    }

    void Editor::FMeshViewer::DrawUI(bool Standalone) {
        // 열린 에셋이 재임포트되면 본 번호와 재질 슬롯을 새 데이터에 맞춰 다시 만듭니다.
        if (mOpen && mSkeletalMesh != nullptr && mPose.GetMeshRevision() != mSkeletalMesh->GetRevision()) {
            const std::filesystem::path Path{Core::FilePath::FromUtf8(mPath)};
            Core::FString Error{};

            if (!Open(Path, Error)) {
                mOpen = false;
                mError = std::move(Error);
            }
        }

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

        if (ImGui::Begin("Mesh Viewer", Standalone ? nullptr : &mOpen, Flags)) {
            ImGui::TextUnformatted(mPath.c_str());
            ImGui::Text("Vertices: %u | Triangles: %u | SubMeshes: %zu | Materials: %zu", mMesh->GetVertexCount(), mMesh->GetIndexCount() / 3, mMesh->GetMeshData().GetSubMeshes().size(), mMaterials.size());

            if (ImGui::Button("Frame Mesh")) {
                FrameMesh();
            }

            ImGui::SameLine();

            bool Wireframe{mViewMode == Engine::EViewMode::Wireframe};

            if (ImGui::Checkbox("Wireframe", &Wireframe)) {
                mViewMode = Wireframe ? Engine::EViewMode::Wireframe : Engine::EViewMode::Lit;
            }

            ImGui::SameLine();
            ImGui::TextDisabled("Drag: orbit | Middle: pan | Wheel: zoom | F: frame | W/E: move/rotate bone");
            DrawMaterials();

            if (!mError.empty()) {
                ImGui::TextWrapped("%s", mError.c_str());
            }

            if (mSkeletalMesh != nullptr) {
                DrawPose();
                ImGui::SameLine();
            }

            DrawPreview();
        } else {
            mMouseCaptured = false;
        }

        ImGui::End();
    }

    void Editor::FMeshViewer::DrawPreview() {
        const ImVec2 Size{ImGui::GetContentRegionAvail()};

        if (Size.x < 1.0f || Size.y < 1.0f) {
            mMouseCaptured = false;
            return;
        }

        const Core::Uint32 Width{static_cast<Core::Uint32>(Size.x)};
        const Core::Uint32 Height{static_cast<Core::Uint32>(Size.y)};

        if (mTarget == nullptr) {
            mTarget = mRenderer->CreateRenderTarget(Width, Height);
        } else if ((mTarget->GetWidth() != Width || mTarget->GetHeight() != Height) && !mRenderer->ResizeRenderTarget(mTarget.get(), Width, Height)) {
            return;
        }

        if (mTarget == nullptr) {
            return;
        }

        // 이미지는 입력을 소유하지 않아 드래그가 창 이동으로 처리되므로 버튼이 마우스를 받게 합니다.
        ImGui::InvisibleButton("##Preview", Size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);

        const bool Hovered{ImGui::IsItemHovered()};
        const ImGuiIO& Input{ImGui::GetIO()};
        const Core::FVector2 Extent{Size.x, Size.y};
        const ImVec2 Origin{ImGui::GetItemRectMin()};
        ImGui::GetWindowDrawList()->AddImage(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(mTarget->GetNativeTexture()))}, Origin, ImGui::GetItemRectMax());
        const bool GizmoActive{mSkeletalMesh != nullptr && UpdateBoneGizmo(CalcSceneView(Extent), {Input.MousePos.x - Origin.x, Input.MousePos.y - Origin.y}, Hovered)};

        if (!GizmoActive && Hovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) {
            mMouseCaptured = true;
            ImGui::SetWindowFocus();
        }

        if (!ImGui::IsWindowFocused() || (!Input.MouseDown[0] && !Input.MouseDown[1] && !Input.MouseDown[2])) {
            mMouseCaptured = false;
        }

        if (mMouseCaptured) {
            if (Input.MouseDown[2]) {
                const Core::FQuat Rotation{mRotation.Quaternion()};
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

        if (mSkeletalMesh != nullptr && Hovered && !Input.WantTextInput && !mGizmo.IsDragging()) {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) {
                mGizmoMode = Editor::EGizmoMode::Translate;
            } else if (ImGui::IsKeyPressed(ImGuiKey_E)) {
                mGizmoMode = Editor::EGizmoMode::Rotate;
            }
        }

        const Engine::FSceneView View{CalcSceneView(Extent)};

        Engine::FPrimitiveDrawInterface Bones{};

        if (mSkeletalMesh != nullptr && mShowBones) {
            // 전역 포즈의 원점을 부모와 연결하면 메시와 같은 변환의 본 계층을 볼 수 있습니다.
            for (std::size_t Index{}; Index < mSkeletalMesh->GetBones().size(); ++Index) {
                const Core::Int32 Parent{mSkeletalMesh->GetBones()[Index].GetParentIndex()};

                if (Parent >= 0) {
                    Bones.DrawLine(mPose.GetGlobalTransforms()[static_cast<std::size_t>(Parent)].TransformPosition({}), mPose.GetGlobalTransforms()[Index].TransformPosition({}), Index == mPoseBone ? Core::FVector{1.0f, 0.4f, 0.1f} : Core::FVector{0.2f, 1.0f, 0.4f}, false, 2.0f);
                }
            }
        }

        if (mSkeletalMesh != nullptr) {
            mGizmo.BuildHandles(View, GetBonePlacement(), Bones);
            mGizmo.DrawFeedback(Bones);
        }

        Engine::FSceneViewFamily Family{mScene.get(), mTarget.get(), View, &Bones, mViewMode};

        // 공통 렌더링 경로에서 뷰어의 보조 표시는 본 선분과 기즈모로 제한합니다.
        Family.mShowFlags.mLightIcons = false;
        Family.mShowFlags.mFogIcons = false;
        Family.mShowFlags.mSprites = false;
        Family.mShowFlags.mText = false;

        mRenderer->BeginRenderingViewFamily(Family);
    }

    void Editor::FMeshViewer::DrawPose() {
        const float Scale{ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi};

        if (ImGui::BeginChild("Skeleton", {320.0f * Scale, 0.0f}, ImGuiChildFlags_Borders)) {
            const auto& Bones{mSkeletalMesh->GetBones()};
            ImGui::Text("Bones: %zu", Bones.size());
            ImGui::Checkbox("Show Bones", &mShowBones);

            // 아래쪽 기즈모 설정 영역을 남기고 나머지 높이를 계층 트리에 할당합니다.
            const float ControlsHeight{ImGui::GetTextLineHeightWithSpacing() + ImGui::GetFrameHeightWithSpacing() * 2.0f};

            if (ImGui::BeginChild("Bone Hierarchy", {0.0f, -ControlsHeight}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
                Core::TArray<Core::TArray<Core::Uint32>> Children(Bones.size());

                for (Core::Uint32 Index{}; Index < Bones.size(); ++Index) {
                    const Core::Int32 Parent{Bones[Index].GetParentIndex()};

                    if (Parent >= 0) {
                        Children[static_cast<std::size_t>(Parent)].push_back(Index);
                    }
                }

                // 본 계층은 깊으므로 들여쓰기를 줄여 깊은 본의 이름까지 패널 안에 표시합니다.
                ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, ImGui::GetStyle().IndentSpacing / 3.0f);

                for (Core::Uint32 Index{}; Index < Bones.size(); ++Index) {
                    if (Bones[Index].GetParentIndex() < 0) {
                        DrawBoneNode(Index, Children);
                    }
                }

                ImGui::PopStyleVar();
            }

            ImGui::EndChild();
            ImGui::TextUnformatted(Bones[mPoseBone].GetName().c_str());

            if (ImGui::RadioButton("Rotate", mGizmoMode == Editor::EGizmoMode::Rotate)) {
                mGizmoMode = Editor::EGizmoMode::Rotate;
            }

            ImGui::SameLine();

            if (ImGui::RadioButton("Move", mGizmoMode == Editor::EGizmoMode::Translate)) {
                mGizmoMode = Editor::EGizmoMode::Translate;
            }

            ImGui::SameLine();

            bool Local{mGizmoSpace == Editor::EGizmoSpace::Local};

            if (ImGui::Checkbox("Local", &Local)) {
                mGizmoSpace = Local ? Editor::EGizmoSpace::Local : Editor::EGizmoSpace::World;
            }

            if (ImGui::Button("Reset Bone") && mPose.SetLocalTransform(*mSkeletalMesh, mPoseBone, Bones[mPoseBone].GetBindPose())) {
                UpdateScene();
            }

            ImGui::SameLine();

            if (ImGui::Button("Reset Bind Pose") && mPose.Reset(*mSkeletalMesh)) {
                UpdateScene();
            }
        }

        ImGui::EndChild();
    }

    void Editor::FMeshViewer::DrawBoneNode(Core::Uint32 Bone, const Core::TArray<Core::TArray<Core::Uint32>>& Children) {
        const auto& BoneChildren{Children[Bone]};
        const ImGuiTreeNodeFlags Flags{ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen | (Bone == mPoseBone ? ImGuiTreeNodeFlags_Selected : 0) | (BoneChildren.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : 0)};
        // 본 이름은 중복될 수 있으므로 번호를 트리 노드 식별자로 사용합니다.
        const bool Open{ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<std::uintptr_t>(Bone)), Flags, "%s", mSkeletalMesh->GetBones()[Bone].GetName().c_str())};

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            mPoseBone = Bone;
        }

        if (Open && !BoneChildren.empty()) {
            for (const Core::Uint32 Child : BoneChildren) {
                DrawBoneNode(Child, Children);
            }

            ImGui::TreePop();
        }
    }

    Engine::FSceneView Editor::FMeshViewer::CalcSceneView(const Core::FVector2& Size) const {
        const Core::FQuat Rotation{mRotation.Quaternion()};
        Engine::FSceneView View{};
        View.mSize = Size;
        View.mLocation = mCenter - Rotation.ForwardVector() * mDistance;
        View.mForward = Rotation.ForwardVector();
        View.mFieldOfView = 60.0f;
        View.mView = Core::FMatrix::LookTo(View.mLocation, View.mForward, Rotation.UpVector());
        View.mProjection = Core::FMatrix::Perspective(View.mFieldOfView, Size.x / Size.y, std::max(mRadius * 0.001f, 0.00001f), std::max(mDistance + mRadius * 3.0f, mRadius * 10.0f));
        View.mViewProjection = View.mView * View.mProjection;

        return View;
    }

    Editor::FGizmoPlacement Editor::FMeshViewer::GetBonePlacement() const {
        const Core::FMatrix& Global{mPose.GetGlobalTransforms()[mPoseBone]};
        Editor::FGizmoPlacement Placement{};
        Placement.mPivot = Global.TransformPosition({});
        Placement.mMode = mGizmoMode;

        // 본 행렬에는 스케일이 있을 수 있으므로 축을 정규화하고, 퇴화한 축은 월드 축으로 대체합니다.
        for (Core::Int32 Index{}; Index < 3; ++Index) {
            const Core::FVector Axis{Global.GetAxis(Index).Normalized()};
            Placement.mAxes[Index] = mGizmoSpace == Editor::EGizmoSpace::Local && !Axis.IsNearlyZero() ? Axis : Editor::FGizmoCore::GetUnitAxes()[Index];
        }

        return Placement;
    }

    bool Editor::FMeshViewer::UpdateBoneGizmo(const Engine::FSceneView& View, const Core::FVector2& Mouse, bool Hovered) {
        if (mGizmo.IsDragging()) {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                mGizmo.EndDrag();

                if (mPose.SetLocalTransform(*mSkeletalMesh, mPoseBone, mDragStartLocal)) {
                    UpdateScene();
                }

                return true;
            }

            if (mGizmo.UpdateDrag(Mouse)) {
                ApplyBoneGizmo();
            }

            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                mGizmo.EndDrag();
            }

            return true;
        }

        const bool CanHover{Hovered && !mMouseCaptured && !ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseDown(ImGuiMouseButton_Middle)};
        const Editor::FGizmoPlacement Placement{GetBonePlacement()};
        const Engine::FHitProxy Hit{CanHover ? mGizmo.HitTest(Mouse, View, Placement) : Engine::FHitProxy{}};
        const Core::Int32 Axis{Hit.mType == Engine::EHitProxyType::GizmoAxis ? Hit.mAxis : -1};
        mGizmo.SetHoveredAxis(Axis);

        if (Axis < 0 || !ImGui::IsMouseClicked(ImGuiMouseButton_Left) || !mGizmo.BeginDrag(Axis, Mouse, View, Placement)) {
            return false;
        }

        mDragStartGlobal = mPose.GetGlobalTransforms()[mPoseBone];
        mDragStartLocal = mPose.GetLocalTransforms()[mPoseBone];
        ImGui::SetWindowFocus();

        return true;
    }

    void Editor::FMeshViewer::ApplyBoneGizmo() {
        Core::FMatrix Delta{Core::FMatrix::Translation(mGizmo.GetTranslation())};

        if (mGizmo.GetDragMode() == Editor::EGizmoMode::Rotate) {
            const Core::FVector& Pivot{mGizmo.GetStartPivot()};
            Delta = Core::FMatrix::Translation(-Pivot) * Core::FMatrix::Rotation(mGizmo.GetRotation()) * Core::FMatrix::Translation(Pivot);
        }

        const Core::Int32 Parent{mSkeletalMesh->GetBones()[mPoseBone].GetParentIndex()};
        Core::FMatrix ParentInverse{Core::FMatrix::Identity()};

        if (Parent >= 0 && !mPose.GetGlobalTransforms()[static_cast<std::size_t>(Parent)].TryInverse(ParentInverse)) {
            return;
        }

        // 행 벡터 규약에서 전역 변환은 Local * ParentGlobal이므로 부모의 역행렬을 오른쪽에 곱해 로컬 변환을 되찾습니다.
        Core::FMatrix Local{mDragStartGlobal * Delta * ParentInverse};

        // 역행렬의 부동소수점 오차가 아핀 검사에 걸리지 않도록 마지막 열을 고정합니다.
        for (Core::Int32 Row{}; Row < 3; ++Row) {
            Local(Row, 3) = 0.0f;
        }

        Local(3, 3) = 1.0f;

        if (mPose.SetLocalTransform(*mSkeletalMesh, mPoseBone, Local)) {
            UpdateScene();
        }
    }

}
