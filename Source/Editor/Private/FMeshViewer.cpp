/**
 * @file FMeshViewer.cpp
 * @brief 독립 메시 미리보기와 궤도 카메라의 구현입니다.
 * @ingroup Editor
 */

#include "Core/Types.h"
#include "Editor/FMeshViewer.h"
#include "Viewer/FMeshViewerContent.h"
#include "Viewer/FBonePreview.h"
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
#include "Asset/USkeleton.h"
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
		: mAssets{Assets},
		  mViewer{Renderer, Assets} {
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
        Asset::EAssetType Type{};

        if (mAssets == nullptr || !Asset::FAssetFile::GetType(mAssets->ResolvePath(Path, Error), Type, Error) || (Type != Asset::EAssetType::Mesh && Type != Asset::EAssetType::SkeletalMesh)) {
            return false;
        }

        const Core::FString MeshPath{Core::FilePath::ToGenericUtf8(Path)};
        const Core::FString SkeletonPath{Type == Asset::EAssetType::SkeletalMesh && MeshPath == mPath ? mSkeletonPath : Core::FString{}};
        CoreUObject::UObject* Object{Type == Asset::EAssetType::SkeletalMesh ? static_cast<CoreUObject::UObject*>(mAssets->LoadSkeletalMesh(Path, Error)) : mAssets->LoadMesh(Path, Error)};

        if (Object == nullptr) {
            return false;
        }

        if (!mViewer.SetAsset(Object, Type)) {
            Error = mViewer.GetError();

            return false;
        }

        mContent = static_cast<Viewer::FMeshViewerContent*>(mViewer.GetContent());

        if (Type == Asset::EAssetType::SkeletalMesh) {
            Asset::USkeleton* Skeleton{SkeletonPath.empty() ? mAssets->FindLegacySkeleton(Path) : mAssets->LoadSkeleton(Core::FilePath::FromUtf8(SkeletonPath), Error)};

            if ((!SkeletonPath.empty() && Skeleton == nullptr) || !mContent->SetSkeleton(Skeleton)) {
                Error = Error.empty() ? "Cannot prepare the skeleton preview" : Error;
                mViewer.Close();
                mContent = nullptr;
                mOpen = false;

                return false;
            }
        }

        mMesh = mContent->GetMesh();
        mSkeletalMesh = mContent->GetSkeletalMesh();
        mSkeleton = mContent->GetSkeleton();
        mSkeletonPath = SkeletonPath;
        mPoseBone.reset();
        mGizmo.EndDrag();
        mMaterialPaths.assign(mContent->GetMaterials().size(), Core::FString{});
        mPath = MeshPath;
        mError.clear();
        mOpen = true;
        mMouseCaptured = false;
        FrameMesh();

        return true;
    }

    /**
     * @brief 창의 열림 상태를 검사합니다.
     */
    bool Editor::FMeshViewer::IsOpen() const {
        return mOpen;
    }

    void Editor::FMeshViewer::CloseRetired(const std::filesystem::path& Path) {
        if (!mOpen || mAssets == nullptr) {
            return;
        }

        Core::FString Error{};
        const auto Normalize{[this, &Error](const std::filesystem::path& Target) {
            return Core::FilePath::ToGenericUtf8(Core::FilePath::ToLower(Core::FilePath::FromUtf8(mAssets->GetRelativePath(Target, Error))));
        }};
        const Core::FString Root{Normalize(Path)};
        const auto Contains{[&](const Core::FString& Open) {
            if (Open.empty() || Root.empty()) {
                return false;
            }

            const Core::FString Relative{Normalize(Core::FilePath::FromUtf8(Open))};

            return Root == "." || Relative == Root || Relative.starts_with(Root + "/");
        }};

        if (Contains(mPath) || Contains(mSkeletonPath)) {
            mOpen = false;
        }
    }

    /**
     * @brief 전체 메시가 보이도록 카메라를 배치합니다.
     */
    void Editor::FMeshViewer::FrameMesh() {
        if (mContent != nullptr) {
            mViewer.GetCamera().SetRotation({-20.0f, 45.0f, 0.0f});
            mViewer.Fit();
            mNeedsFit = true;
        }
    }

    /**
     * @brief 현재 메시와 슬롯별 재질을 미리보기 장면에 반영합니다.
     */
    void Editor::FMeshViewer::UpdateScene() {
        mContent->Refresh();
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

        for (Core::Index_t Slot{}; Slot < Slots.size(); ++Slot) {
            ImGui::PushID(Slot);

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
                    mContent->GetMaterials()[Slot] = Material;
                    mMaterialPaths[Slot] = Path;

                    UpdateScene();
                }
            }

            ImGui::PopID();
        }
    }

    void Editor::FMeshViewer::DrawSkeletonPicker() {
        const char* Label{mSkeletonPath.empty() ? mSkeleton == nullptr ? "None" : "Embedded Skeleton" : mSkeletonPath.c_str()};

        if (!ImGui::BeginCombo("Skeleton", Label)) {
            return;
        }

        Core::FString SelectedPath{mSkeletonPath};
        Core::FString Error{};

        if (ImGui::Selectable("Default", SelectedPath.empty())) {
            SelectedPath.clear();
        }

        for (const auto& Candidate : mAssets->GetAssetPaths(Error)) {
            Asset::EAssetType Type{};

            if (Asset::FAssetFile::GetType(mAssets->ResolvePath(Candidate, Error), Type, Error) && Type == Asset::EAssetType::Skeleton) {
                const Core::FString Relative{Core::FilePath::ToGenericUtf8(Candidate)};

                if (ImGui::Selectable(Relative.c_str(), Relative == SelectedPath)) {
                    SelectedPath = Relative;
                }
            }
        }

        ImGui::EndCombo();

        if (SelectedPath == mSkeletonPath) {
            return;
        }

        Error.clear();

        Asset::USkeleton* Skeleton{SelectedPath.empty() ? mAssets->FindLegacySkeleton(Core::FilePath::FromUtf8(mPath)) : mAssets->LoadSkeleton(Core::FilePath::FromUtf8(SelectedPath), Error)};
        if ((!SelectedPath.empty() && Skeleton == nullptr) || !mContent->SetSkeleton(Skeleton)) {
            mError = Error.empty() ? "Cannot prepare the skeleton preview pose" : std::move(Error);

            return;
        }

        mSkeleton = mContent->GetSkeleton();
        mSkeletonPath = std::move(SelectedPath);
        mPoseBone.reset();
        mGizmo.EndDrag();
        mError.clear();

        UpdateScene();
    }

    /**
     * @brief 카메라·재질 조작 UI와 미리보기를 표시합니다.
     */
    void Editor::FMeshViewer::DrawUI(const Core::FInput& Input, bool Standalone) {
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
            ImGui::Text("Vertices: %u | Triangles: %u | SubMeshes: %zu | Materials: %zu", mMesh->GetVertexCount(), mMesh->GetIndexCount() / 3, mMesh->GetMeshData().GetSubMeshes().size(), mContent->GetMaterials().size());

            if (ImGui::Button("Frame Mesh")) {
                FrameMesh();
            }

            ImGui::SameLine();

            bool Wireframe{mViewMode == Engine::EViewMode::Wireframe};

            if (ImGui::Checkbox("Wireframe", &Wireframe)) {
                mViewMode = Wireframe ? Engine::EViewMode::Wireframe : Engine::EViewMode::Lit;
            }

            ImGui::SameLine();
            ImGui::TextDisabled(mSkeleton != nullptr ? "Click bone | Drag: orbit | Middle: pan | Wheel: zoom | F: frame | W/E: move/rotate" : "Drag: orbit | Middle: pan | Wheel: zoom | F: frame");
            DrawMaterials();

            if (mSkeletalMesh != nullptr) {
                DrawSkeletonPicker();
            }

            if (!mError.empty()) {
                ImGui::TextWrapped("%s", mError.c_str());
            }

            if (mSkeleton != nullptr) {
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

        if (!mViewer.Resize(Width, Height)) {
            return;
        }

        if (mNeedsFit) {
            mNeedsFit = !mViewer.Fit();
        }

        // 이미지는 입력을 소유하지 않아 드래그가 창 이동으로 처리되므로 버튼이 마우스를 받게 합니다.
        ImGui::InvisibleButton("##Preview", Size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);

        const bool Hovered{ImGui::IsItemHovered()};

        using Core::EKey;

        const Core::FVector2 Mouse{Input.GetMousePosition()};
        const Core::FVector2 Delta{Input.GetMouseDelta()};
        const Core::FVector2 Extent{Size.x, Size.y};
        const ImVec2 Origin{ImGui::GetItemRectMin()};

        ImGui::GetWindowDrawList()->AddImage(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(mViewer.GetNativeTexture()))}, Origin, ImGui::GetItemRectMax());

        const Engine::FSceneView InputView{CalcSceneView(Extent)};
        const Core::FVector2 PreviewMouse{Mouse.x - Origin.x, Mouse.y - Origin.y};
        const bool GizmoActive{mPoseBone && UpdateBoneGizmo(*mPoseBone, Input, InputView, PreviewMouse, Hovered)};
        const Core::TArray<Viewer::FBoneShape> BoneShapes{mSkeleton != nullptr && mShowBones ? Viewer::BuildBoneShapes(mSkeleton->GetBones(), mContent->GetPose().GetGlobalTransforms(), mViewer.GetCamera().GetRadius()) : Core::TArray<Viewer::FBoneShape>{}};
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
                mViewer.GetCamera().Pan(Delta, Size.y);
            } else {
                mViewer.GetCamera().Orbit(Delta);
            }
        }

        if (Hovered && Input.GetMouseWheel() != 0.0f) {
            mViewer.GetCamera().Zoom(Input.GetMouseWheel());
        }

        if (Hovered && !ImGui::GetIO().WantTextInput && Input.WasPressed(EKey::F)) {
            FrameMesh();
        }

        if (mSkeleton != nullptr && Hovered && !ImGui::GetIO().WantTextInput && !mGizmo.IsDragging()) {
            if (Input.WasPressed(EKey::W)) {
                mGizmoMode = Editor::EGizmoMode::Translate;
            } else if (Input.WasPressed(EKey::E)) {
                mGizmoMode = Editor::EGizmoMode::Rotate;
            }
        }

        const Engine::FSceneView View{CalcSceneView(Extent)};

        Engine::FPrimitiveDrawInterface Bones{};

        for (const Viewer::FBoneShape& Shape : BoneShapes) {
            const float Gray{Shape.GetMesh() == Engine::EEditorMesh::Sphere ? 0.76f : 0.66f};
            const Core::FVector4 Color{Shape.GetBone() == mPoseBone ? Core::FVector4{0.26f, 0.66f, 1.0f, 1.0f} : Core::FVector4{Gray, Gray, Gray, 1.0f}};

            Bones.DrawMesh({Shape.GetMesh(), Shape.GetWorld(), Color});
        }

        Engine::FPrimitiveDrawInterface Gizmo{};

        if (mPoseBone) {
            mGizmo.BuildHandles(View, GetBonePlacement(*mPoseBone), Gizmo);
            mGizmo.DrawFeedback(Gizmo);
        }

        mViewer.SetViewMode(mViewMode);
        mViewer.Render(mPoseBone ? &Gizmo : nullptr, BoneShapes.empty() ? nullptr : &Bones);
    }

    /**
     * @brief 미리보기 옆에 본 계층 선택·기즈모 설정·포즈 복원 패널을 표시합니다.
     */
    void Editor::FMeshViewer::DrawPose() {
        const float Scale{ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi};

        if (ImGui::BeginChild("Skeleton", {320.0f * Scale, 0.0f}, ImGuiChildFlags_Borders)) {
            const auto& Bones{mSkeleton->GetBones()};

            ImGui::Text("Bones: %zu", Bones.size());
            ImGui::Checkbox("Show Bones", &mShowBones);

            // 아래쪽 기즈모 설정 영역을 남기고 나머지 높이를 계층 트리에 할당합니다.
            const float ControlsHeight{ImGui::GetTextLineHeightWithSpacing() + ImGui::GetFrameHeightWithSpacing() * 2.0f};

            if (ImGui::BeginChild("Bone Hierarchy", {0.0f, -ControlsHeight}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
                Core::TArray<Core::TArray<Core::Index_t>> Children(Bones.size());

                for (Core::Index_t Index{}; Index < Bones.size(); ++Index) {
                    const Core::Int32 Parent{static_cast<Core::Int32>(Bones[Index].GetParentIndex())};

                    if (Parent >= 0) {
                        Children[Parent].push_back(Index);
                    }
                }

                // 본 계층은 깊으므로 들여쓰기를 줄여 깊은 본의 이름까지 패널 안에 표시합니다.
                ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, ImGui::GetStyle().IndentSpacing / 3.0f);

                for (Core::Index_t Index{}; Index < Bones.size(); ++Index) {
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

            if (ImGui::Button("Reset Bone") && mPoseBone && mContent->GetPose().SetLocalTransform(*mSkeletalMesh, *mSkeleton, *mPoseBone, Bones[*mPoseBone].GetBindPose())) {
                UpdateScene();
            }

            ImGui::EndDisabled();
            ImGui::SameLine();

            if (ImGui::Button("Reset Bind Pose") && mContent->GetPose().Reset(*mSkeletalMesh, *mSkeleton)) {
                UpdateScene();
            }
        }

        ImGui::EndChild();
    }

    /**
     * @brief 본 하나와 그 자손을 계층 트리 노드로 표시하고 클릭하면 조작 대상으로 선택합니다.
     */
    void Editor::FMeshViewer::DrawBoneNode(Core::Index_t Bone, const Core::TArray<Core::TArray<Core::Index_t>>& Children) {
        const auto& BoneChildren{Children[Bone]};
        const ImGuiTreeNodeFlags Flags{ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen | (Bone == mPoseBone ? ImGuiTreeNodeFlags_Selected : 0) | (BoneChildren.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : 0)};

        // 본 이름은 중복될 수 있으므로 번호를 트리 노드 식별자로 사용합니다.
        const bool Open{ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<std::uintptr_t>(Bone)), Flags, "%s", mSkeleton->GetBones()[Bone].GetName().c_str())};

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            mGizmo.EndDrag();
            mPoseBone = Bone;
        }

        if (Open && !BoneChildren.empty()) {
            for (const Core::Index_t Child : BoneChildren) {
                DrawBoneNode(Child, Children);
            }

            ImGui::TreePop();
        }
    }

    bool Editor::FMeshViewer::PickBone(const Engine::FSceneView& View, const Core::FVector2& Mouse, const Core::TArray<Viewer::FBoneShape>& Shapes) {
        Core::FRay Ray{};

        if (!View.Deproject(Mouse, Ray)) {
            return false;
        }

        const Viewer::FBoneShape* Hit{};
        float Closest{std::numeric_limits<float>::max()};

        for (auto Shape{Shapes.rbegin()}; Shape != Shapes.rend(); ++Shape) {
            if (Engine::RaycastMesh(&Engine::GetEditorMesh(Shape->GetMesh()), Shape->GetWorld(), Ray, Closest)) {
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

        mPoseBone = Hit->GetBone();
        mGizmo.EndDrag();

        return true;
    }

    /**
     * @brief 현재 궤도 카메라의 렌더링과 입력 해석에 쓸 뷰를 계산합니다.
     */
    Engine::FSceneView Editor::FMeshViewer::CalcSceneView(const Core::FVector2& Size) const {
        return mViewer.GetCamera().CalcSceneView(Size);
    }

    /**
     * @brief 선택한 본의 현재 전역 포즈로 기즈모 배치를 구성합니다.
     */
    Editor::FGizmoPlacement Editor::FMeshViewer::GetBonePlacement(Core::Index_t Bone) const {
        const Core::FMatrix& Global{mContent->GetPose().GetGlobalTransforms()[Bone]};
        Editor::FGizmoPlacement Placement{};
        Placement.mPivot = Global.TransformPosition({});
        Placement.mMode = mGizmoMode;

        // 본 행렬에는 스케일이 있을 수 있으므로 축을 정규화하고, 퇴화한 축은 월드 축으로 대체합니다.
        for (Core::Index_t Index{}; Index < 3; ++Index) {
            const Core::FVector Axis{Global.GetAxis(Index).Normalized()};

            Placement.mAxes[Index] = mGizmoSpace == Editor::EGizmoSpace::Local && !Axis.IsNearlyZero() ? Axis : Editor::FGizmoCore::GetUnitAxes()[Index];
        }

        return Placement;
    }

    /**
     * @brief 기즈모의 호버와 드래그 입력을 처리합니다.
     */
    bool Editor::FMeshViewer::UpdateBoneGizmo(Core::Index_t Bone, const Core::FInput& Input, const Engine::FSceneView& View, const Core::FVector2& Mouse, bool Hovered) {
        using Core::EKey;

        if (mGizmo.IsDragging()) {
            if (Input.WasPressed(EKey::Escape)) {
                mGizmo.EndDrag();

                if (mContent->GetPose().SetLocalTransform(*mSkeletalMesh, *mSkeleton, Bone, mDragStartLocal)) {
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
        const Core::Int32 Axis{static_cast<Core::Int32>(Hit.mType == Engine::EHitProxyType::GizmoAxis ? Hit.mAxis : -1)};

        mGizmo.SetHoveredAxis(Axis);

        if (Axis < 0 || !Input.WasPressed(EKey::MouseLeft) || !mGizmo.BeginDrag(Axis, Mouse, View, Placement)) {
            return false;
        }

        mDragStartGlobal = mContent->GetPose().GetGlobalTransforms()[Bone];
        mDragStartLocal = mContent->GetPose().GetLocalTransforms()[Bone];

        ImGui::SetWindowFocus();

        return true;
    }

    /**
     * @brief 기즈모의 드래그 결과를 선택한 본의 부모 기준 로컬 변환으로 바꾸어 적용합니다.
     */
    void Editor::FMeshViewer::ApplyBoneGizmo(Core::Index_t Bone) {
        Core::FMatrix Delta{Core::FMatrix::Translation(mGizmo.GetTranslation())};

        if (mGizmo.GetDragMode() == Editor::EGizmoMode::Rotate) {
            const Core::FVector& Pivot{mGizmo.GetStartPivot()};

            Delta = Core::FMatrix::Translation(-Pivot) * Core::FMatrix::Rotation(mGizmo.GetRotation()) * Core::FMatrix::Translation(Pivot);
        }

        const Core::Int32 Parent{static_cast<Core::Int32>(mSkeleton->GetBones()[Bone].GetParentIndex())};
        Core::FMatrix ParentInverse{Core::FMatrix::Identity()};

        if (Parent >= 0 && !mContent->GetPose().GetGlobalTransforms()[Parent].TryInverse(ParentInverse)) {
            return;
        }

        // 행 벡터 규약에서 전역 변환은 Local * ParentGlobal이므로 부모의 역행렬을 오른쪽에 곱해 로컬 변환을 되찾습니다.
        Core::FMatrix Local{mDragStartGlobal * Delta * ParentInverse};

        // 역행렬의 부동소수점 오차가 아핀 검사에 걸리지 않도록 마지막 열을 고정합니다.
        for (Core::Index_t Row{}; Row < 3; ++Row) {
            Local(Row, 3) = 0.0f;
        }

        Local(3, 3) = 1.0f;

        if (mContent->GetPose().SetLocalTransform(*mSkeletalMesh, *mSkeleton, Bone, Local)) {
            UpdateScene();
        }
    }

}
