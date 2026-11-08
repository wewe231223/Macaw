/**
 * @file FMeshViewer.cpp
 * @brief 독립 메시 미리보기와 궤도 카메라의 구현입니다.
 * @ingroup Editor
 */

#include "Editor/FMeshViewer.h"
#include "Core/FilePath.h"
#include "Core/Math/FBox.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FRay.h"
#include "Core/Math/FVector4.h"
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
#include "Engine/EditorMesh.h"
#include "Engine/MeshPicking.h"
#include "Asset/UMaterial.h"
#include "RenderCore/IRendererModule.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <d3d11.h>
#include <limits>

namespace Editor {

    /**
     * @brief 렌더러와 프로젝트 에셋 관리자를 연결합니다.
     */
    Editor::FMeshViewer::FMeshViewer(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets)
		: mRenderer{Renderer},
		  mAssets{Assets} {
    }

    /**
     * @brief 미리보기 장면과 렌더 타깃을 해제합니다.
     */
    Editor::FMeshViewer::~FMeshViewer() = default;

    /**
     * @brief 에셋을 독립 미리보기 장면에 엽니다.
     */
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
            Ambient.mIntensity = 0.5f;

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
        mPoseBone.reset();
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

    /**
     * @brief 창의 열림 상태를 검사합니다.
     */
    bool Editor::FMeshViewer::IsOpen() const {
        return mOpen;
    }

    /**
     * @brief 전체 메시가 보이도록 카메라를 배치합니다.
     */
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

    /**
     * @brief 현재 메시와 슬롯별 재질을 미리보기 장면에 반영합니다.
     */
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

    /**
     * @brief 슬롯별 임시 재질 선택 UI를 표시합니다.
     */
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

    /**
     * @brief 카메라·재질 조작 UI와 미리보기를 표시합니다.
     */
    void Editor::FMeshViewer::DrawUI(const Core::FInput& Input, bool Standalone) {
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
            ImGui::TextDisabled(mSkeletalMesh != nullptr ? "Click bone | Drag: orbit | Middle: pan | Wheel: zoom | F: frame | W/E: move/rotate" : "Drag: orbit | Middle: pan | Wheel: zoom | F: frame");
            DrawMaterials();

            if (!mError.empty()) {
                ImGui::TextWrapped("%s", mError.c_str());
            }

            if (mSkeletalMesh != nullptr) {
                DrawPose();
                ImGui::SameLine();
            }

            DrawPreview(Input);
        } else {
            mMouseCaptured = false;
        }

        ImGui::End();
    }

    /**
     * @brief 카메라 입력을 처리하고 공통 렌더 경로로 메시와 본 도형을 그리며 아이콘·스프라이트·텍스트 표시는 제외합니다.
     */
    void Editor::FMeshViewer::DrawPreview(const Core::FInput& Input) {
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

        using Core::EKey;

        const Core::FVector2 Mouse{Input.GetMousePosition()};
        const Core::FVector2 Delta{Input.GetMouseDelta()};
        const Core::FVector2 Extent{Size.x, Size.y};
        const ImVec2 Origin{ImGui::GetItemRectMin()};

        ImGui::GetWindowDrawList()->AddImage(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(mTarget->GetNativeTexture()))}, Origin, ImGui::GetItemRectMax());

        const Engine::FSceneView InputView{CalcSceneView(Extent)};
        const Core::FVector2 PreviewMouse{Mouse.x - Origin.x, Mouse.y - Origin.y};
        const bool GizmoActive{mPoseBone && UpdateBoneGizmo(*mPoseBone, Input, InputView, PreviewMouse, Hovered)};
        const Core::TArray<Editor::FMeshViewer::FBoneShape> BoneShapes{mSkeletalMesh != nullptr && mShowBones ? BuildBoneShapes() : Core::TArray<Editor::FMeshViewer::FBoneShape>{}};
        const bool CanPickBone{Hovered && !mMouseCaptured && !GizmoActive && !BoneShapes.empty()};
        const bool LeftClick{Input.WasPressed(EKey::MouseLeft) && !Input.IsDown(EKey::MouseRight) && !Input.IsDown(EKey::MouseMiddle)};
        const bool PickedBone{CanPickBone && LeftClick && PickBone(InputView, PreviewMouse, BoneShapes)};

        if (PickedBone) {
            ImGui::SetWindowFocus();
        } else if (!GizmoActive && Hovered && (Input.WasPressed(EKey::MouseLeft) || Input.WasPressed(EKey::MouseRight) || Input.WasPressed(EKey::MouseMiddle))) {
            mMouseCaptured = true;

            ImGui::SetWindowFocus();
        }

        if (!ImGui::IsWindowFocused() || (!Input.IsDown(EKey::MouseLeft) && !Input.IsDown(EKey::MouseRight) && !Input.IsDown(EKey::MouseMiddle))) {
            mMouseCaptured = false;
        }

        if (mMouseCaptured) {
            if (Input.IsDown(EKey::MouseMiddle)) {
                const Core::FQuat Rotation{mRotation.Quaternion()};

                mCenter += (Rotation.RightVector() * -Delta.x + Rotation.UpVector() * Delta.y) * (mDistance / std::max(Size.y, 1.0f));
            } else {
                mRotation.yaw += Delta.x * 0.3f;
                mRotation.pitch = std::clamp(mRotation.pitch - Delta.y * 0.3f, -89.0f, 89.0f);
            }
        }

        if (Hovered && Input.GetMouseWheel() != 0.0f) {
            mDistance = std::clamp(mDistance * std::exp(-Input.GetMouseWheel() * 0.15f), mRadius * 0.02f, mRadius * 1000.0f);
        }

        if (Hovered && !ImGui::GetIO().WantTextInput && Input.WasPressed(EKey::F)) {
            FrameMesh();
        }

        if (mSkeletalMesh != nullptr && Hovered && !ImGui::GetIO().WantTextInput && !mGizmo.IsDragging()) {
            if (Input.WasPressed(EKey::W)) {
                mGizmoMode = Editor::EGizmoMode::Translate;
            } else if (Input.WasPressed(EKey::E)) {
                mGizmoMode = Editor::EGizmoMode::Rotate;
            }
        }

        const Engine::FSceneView View{CalcSceneView(Extent)};

        Engine::FPrimitiveDrawInterface Bones{};

        for (const Editor::FMeshViewer::FBoneShape& Shape : BoneShapes) {
            const float Gray{Shape.mMesh == Engine::EEditorMesh::Sphere ? 0.76f : 0.66f};
            const Core::FVector4 Color{Shape.mBone == mPoseBone ? Core::FVector4{0.26f, 0.66f, 1.0f, 1.0f} : Core::FVector4{Gray, Gray, Gray, 1.0f}};

            Bones.DrawMesh({Shape.mMesh, Shape.mWorld, Color});
        }

        Engine::FPrimitiveDrawInterface Gizmo{};

        if (mPoseBone) {
            mGizmo.BuildHandles(View, GetBonePlacement(*mPoseBone), Gizmo);
            mGizmo.DrawFeedback(Gizmo);
        }

        Engine::FSceneViewFamily Family{mScene.get(), mTarget.get(), View, mPoseBone ? &Gizmo : nullptr, mViewMode};
        Family.mBackgroundOverlay = BoneShapes.empty() ? nullptr : &Bones;

        // 공통 렌더링 경로에서 뷰어의 보조 표시는 본 도형과 기즈모로 제한합니다.
        Family.mShowFlags.mSky = false;
        Family.mShowFlags.mLightIcons = false;
        Family.mShowFlags.mFogIcons = false;
        Family.mShowFlags.mSprites = false;
        Family.mShowFlags.mText = false;

        mRenderer->BeginRenderingViewFamily(Family);
    }

    /**
     * @brief 미리보기 옆에 본 계층 선택·기즈모 설정·포즈 복원 패널을 표시합니다.
     */
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
            ImGui::TextUnformatted(mPoseBone ? Bones[*mPoseBone].GetName().c_str() : "Select a bone");

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

            ImGui::BeginDisabled(!mPoseBone);

            if (ImGui::Button("Reset Bone") && mPoseBone && mPose.SetLocalTransform(*mSkeletalMesh, *mPoseBone, Bones[*mPoseBone].GetBindPose())) {
                UpdateScene();
            }

            ImGui::EndDisabled();
            ImGui::SameLine();

            if (ImGui::Button("Reset Bind Pose") && mPose.Reset(*mSkeletalMesh)) {
                UpdateScene();
            }
        }

        ImGui::EndChild();
    }

    /**
     * @brief 본 하나와 그 자손을 계층 트리 노드로 표시하고 클릭하면 조작 대상으로 선택합니다.
     */
    void Editor::FMeshViewer::DrawBoneNode(Core::Uint32 Bone, const Core::TArray<Core::TArray<Core::Uint32>>& Children) {
        const auto& BoneChildren{Children[Bone]};
        const ImGuiTreeNodeFlags Flags{ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen | (Bone == mPoseBone ? ImGuiTreeNodeFlags_Selected : 0) | (BoneChildren.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : 0)};

        // 본 이름은 중복될 수 있으므로 번호를 트리 노드 식별자로 사용합니다.
        const bool Open{ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<std::uintptr_t>(Bone)), Flags, "%s", mSkeletalMesh->GetBones()[Bone].GetName().c_str())};

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            mGizmo.EndDrag();
            mPoseBone = Bone;
        }

        if (Open && !BoneChildren.empty()) {
            for (const Core::Uint32 Child : BoneChildren) {
                DrawBoneNode(Child, Children);
            }

            ImGui::TreePop();
        }
    }

    Core::TArray<Editor::FMeshViewer::FBoneShape> Editor::FMeshViewer::BuildBoneShapes() const {
        const auto& Bones{mSkeletalMesh->GetBones()};
        const auto& Transforms{mPose.GetGlobalTransforms()};
        const float MaxJointRadius{mRadius * 0.019f};
        const float MinJointRadius{MaxJointRadius * 0.12f};
        const float MaxBoneHalfWidth{mRadius * 0.05f};
        Core::TArray<float> JointRadii{};
        JointRadii.resize(Bones.size(), MaxJointRadius);

        Core::TArray<Editor::FMeshViewer::FBoneShape> Shapes{};
        Shapes.reserve(Bones.size() * 2);

        for (Core::Uint32 Bone{}; Bone < Bones.size(); ++Bone) {
            const Core::Int32 ParentIndex{Bones[Bone].GetParentIndex()};

            if (ParentIndex < 0) {
                continue;
            }

            const Core::Uint32 Parent{static_cast<Core::Uint32>(ParentIndex)};
            const Core::FVector Start{Transforms[Parent].TransformPosition({})};
            const Core::FVector Offset{Transforms[Bone].TransformPosition({}) - Start};
            const float Length{Offset.Length()};

            if (!std::isfinite(Length) || Length <= MaxJointRadius * 0.01f) {
                continue;
            }

            const float JointRadius{std::clamp(Length * 0.052f, MinJointRadius, MaxJointRadius)};
            JointRadii[Bone] = std::min(JointRadii[Bone], JointRadius);
            JointRadii[Parent] = std::min(JointRadii[Parent], JointRadius);

            const float HalfWidth{std::min(Length * 0.105f, MaxBoneHalfWidth)};
            const Core::FVector Axis{Offset / Length};
            const Core::FVector Up{std::abs(Axis.z) < 0.8f ? Core::FVector{0.0f, 0.0f, 1.0f} : Core::FVector{0.0f, 1.0f, 0.0f}};
            const Core::FMatrix Rotation{Core::FMatrix::LookTo({}, Axis, Up).Transposed()};
            const Core::FMatrix World{Core::FMatrix::Scale({HalfWidth, HalfWidth, Length}) * Rotation * Core::FMatrix::Translation(Start)};

            Shapes.push_back({Engine::EEditorMesh::BoneOctahedron, World, Parent});
        }

        for (Core::Uint32 Bone{}; Bone < Bones.size(); ++Bone) {
            const Core::FVector Joint{Transforms[Bone].TransformPosition({})};
            const float Diameter{JointRadii[Bone] * 2.0f};
            const Core::FMatrix World{Core::FMatrix::Scale({Diameter, Diameter, Diameter}) * Core::FMatrix::Translation(Joint)};

            Shapes.push_back({Engine::EEditorMesh::Sphere, World, Bone});
        }

        return Shapes;
    }

    bool Editor::FMeshViewer::PickBone(const Engine::FSceneView& View, const Core::FVector2& Mouse, const Core::TArray<Editor::FMeshViewer::FBoneShape>& Shapes) {
        Core::FRay Ray{};

        if (!View.Deproject(Mouse, Ray)) {
            return false;
        }

        const Editor::FMeshViewer::FBoneShape* Hit{};
        float Closest{std::numeric_limits<float>::max()};

        for (auto Shape{Shapes.rbegin()}; Shape != Shapes.rend(); ++Shape) {
            if (Engine::RaycastMesh(&Engine::GetEditorMesh(Shape->mMesh), Shape->mWorld, Ray, Closest)) {
                Hit = &*Shape;
            }
        }

        if (Hit == nullptr) {
            return false;
        }

        const Core::FVector4 Clip{Core::FVector4{Ray.mOrigin + Ray.mDirection * Closest, 1.0f} * View.mViewProjection};

        if (Clip.z > Clip.w) {
            return false;
        }

        mPoseBone = Hit->mBone;
        mGizmo.EndDrag();

        return true;
    }

    /**
     * @brief 현재 궤도 카메라의 렌더링과 입력 해석에 쓸 뷰를 계산합니다.
     */
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

    /**
     * @brief 선택한 본의 현재 전역 포즈로 기즈모 배치를 구성합니다.
     */
    Editor::FGizmoPlacement Editor::FMeshViewer::GetBonePlacement(Core::Uint32 Bone) const {
        const Core::FMatrix& Global{mPose.GetGlobalTransforms()[Bone]};
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

    /**
     * @brief 기즈모의 호버와 드래그 입력을 처리합니다.
     */
    bool Editor::FMeshViewer::UpdateBoneGizmo(Core::Uint32 Bone, const Core::FInput& Input, const Engine::FSceneView& View, const Core::FVector2& Mouse, bool Hovered) {
        using Core::EKey;

        if (mGizmo.IsDragging()) {
            if (Input.WasPressed(EKey::Escape)) {
                mGizmo.EndDrag();

                if (mPose.SetLocalTransform(*mSkeletalMesh, Bone, mDragStartLocal)) {
                    UpdateScene();
                }

                return true;
            }

            if (mGizmo.UpdateDrag(Mouse)) {
                ApplyBoneGizmo(Bone);
            }

            if (!Input.IsDown(EKey::MouseLeft)) {
                mGizmo.EndDrag();
            }

            return true;
        }

        const bool CanHover{Hovered && !mMouseCaptured && !Input.IsDown(EKey::MouseRight) && !Input.IsDown(EKey::MouseMiddle)};
        const Editor::FGizmoPlacement Placement{GetBonePlacement(Bone)};
        const Engine::FHitProxy Hit{CanHover ? mGizmo.HitTest(Mouse, View, Placement) : Engine::FHitProxy{}};
        const Core::Int32 Axis{Hit.mType == Engine::EHitProxyType::GizmoAxis ? Hit.mAxis : -1};

        mGizmo.SetHoveredAxis(Axis);

        if (Axis < 0 || !Input.WasPressed(EKey::MouseLeft) || !mGizmo.BeginDrag(Axis, Mouse, View, Placement)) {
            return false;
        }

        mDragStartGlobal = mPose.GetGlobalTransforms()[Bone];
        mDragStartLocal = mPose.GetLocalTransforms()[Bone];

        ImGui::SetWindowFocus();

        return true;
    }

    /**
     * @brief 기즈모의 드래그 결과를 선택한 본의 부모 기준 로컬 변환으로 바꾸어 적용합니다.
     */
    void Editor::FMeshViewer::ApplyBoneGizmo(Core::Uint32 Bone) {
        Core::FMatrix Delta{Core::FMatrix::Translation(mGizmo.GetTranslation())};

        if (mGizmo.GetDragMode() == Editor::EGizmoMode::Rotate) {
            const Core::FVector& Pivot{mGizmo.GetStartPivot()};

            Delta = Core::FMatrix::Translation(-Pivot) * Core::FMatrix::Rotation(mGizmo.GetRotation()) * Core::FMatrix::Translation(Pivot);
        }

        const Core::Int32 Parent{mSkeletalMesh->GetBones()[Bone].GetParentIndex()};
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

        if (mPose.SetLocalTransform(*mSkeletalMesh, Bone, Local)) {
            UpdateScene();
        }
    }

}
