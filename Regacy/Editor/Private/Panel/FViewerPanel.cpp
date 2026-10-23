#include "pch.h"
#include "Editor/Panel/FViewerPanel.h"
#include "Editor/FileDialog.h"

#include "ImGui/imgui.h"
#include "Render/Renderer.h"
#include "Asset/FAssetRegistry.h"
#include "Asset/UMaterial.h"
#include "Asset/UMesh.h"
#include "Asset/USurfaceOpaque.h"
#include "Core/Console/Console.h"
#include "Asset/Pipeline/UPipeline.h"
#include "Editor/World/FWorldEditorContext.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float MinimumDistance{0.10f};
    constexpr float MaximumDistance{1000000.0f};
    constexpr char DefaultMeshPath[]{"/Game/System/Mesh/Cube.bin"};
}

FViewerPanel::FViewerPanel(HWND InputWindowHandle, FMessageChannel::FSender InEditorToWorldSender, FWorldEditorContext& InEditorContext, FAssetThumbnailRenderer* InThumbnailRenderer)
	: FEditorWindow{"Viewer", ImGuiWindowFlags_MenuBar},
	  mEditorToWorldSender{std::move(InEditorToWorldSender)},
	  mWindowHandle{InputWindowHandle},
	  mEditorContext{InEditorContext} {
    mPropertyEditor.BindThumbnailRenderer(InThumbnailRenderer);
    SetMesh({});
    SetMaterial({});
}

void FViewerPanel::SetMesh(FAssetHandle InMeshHandle) {
    mMeshHandle = FAssetRegistry::Get().ResolveAsset<UMesh>(InMeshHandle) != nullptr ? InMeshHandle : FAssetRegistry::Get().FindAsset(FAssetPath{DefaultMeshPath});
    mMaterialOverrides.clear();
    const UMesh* Mesh{FAssetRegistry::Get().ResolveAsset<UMesh>(mMeshHandle)};

    mSelectedMaterialSlot = Mesh != nullptr ? Mesh->GetMaterialSlotName(0) : FName{};
}

FAssetHandle FViewerPanel::GetMaterial() const {
    const auto Position{std::ranges::find(mMaterialOverrides, mSelectedMaterialSlot, &FMeshMaterialBinding::mSlot)};

    return Position != mMaterialOverrides.end() ? Position->mMaterial : FAssetHandle{};
}

void FViewerPanel::SetMaterial(FAssetHandle InMaterialHandle) {
    std::erase_if(mMaterialOverrides, [this](const FMeshMaterialBinding& Binding) {
        return Binding.mSlot == mSelectedMaterialSlot;
    });

    if (!mSelectedMaterialSlot.IsNone() && FAssetRegistry::Get().ResolveAsset<UMaterial>(InMaterialHandle) != nullptr) {
        mMaterialOverrides.push_back(FMeshMaterialBinding{mSelectedMaterialSlot, InMaterialHandle});
    }
}

bool FViewerPanel::OpenViewerFile(const std::filesystem::path& FilePath) {
    if (FAssetRegistry::TryGet() == nullptr) {
        return false;
    }

    std::string Extension{FilePath.extension().string()};
    std::ranges::transform(Extension, Extension.begin(), [](unsigned char Character) {
        return static_cast<char>(std::tolower(Character));
    });

    if (Extension != ".obj" && Extension != ".bin" && Extension != ".mtl" && Extension != ".material") {
        return false;
    }

    const FAssetHandle Handle{FAssetRegistry::Get().LoadViewerAsset(FilePath)};

    if ((Extension == ".mtl" || Extension == ".material") && FAssetRegistry::Get().ResolveAsset<UMesh>(mMeshHandle) != nullptr && FAssetRegistry::Get().ResolveAsset<UMaterial>(Handle) != nullptr) {
        SetMaterial(Handle);
        return true;
    }

    if (Extension != ".mtl" && Extension != ".material" && FAssetRegistry::Get().ResolveAsset<UMesh>(Handle) != nullptr) {
        SetMesh(Handle);

        const UMesh* Mesh{FAssetRegistry::Get().ResolveAsset<UMesh>(mMeshHandle)};
        const Uint32 VertexCount{Mesh->GetVertexAttributeCount(EVertexAttribute::Position)};
        const Uint32 VertexStride{Mesh->GetVertexStride(EVertexAttribute::Position)};
        const std::byte* VertexData{static_cast<const std::byte*>(Mesh->GetVertexData(EVertexAttribute::Position))};

        if (VertexData != nullptr && VertexCount > 0 && VertexStride >= sizeof(FVector3)) {
            FVector3 Minimum{*reinterpret_cast<const FVector3*>(VertexData)};
            FVector3 Maximum{Minimum};

            for (Uint32 Index{1}; Index < VertexCount; ++Index) {
                const FVector3& Position{*reinterpret_cast<const FVector3*>(VertexData + static_cast<std::size_t>(Index) * VertexStride)};

                Minimum.mX = std::min(Minimum.mX, Position.mX);
                Minimum.mY = std::min(Minimum.mY, Position.mY);
                Minimum.mZ = std::min(Minimum.mZ, Position.mZ);
                Maximum.mX = std::max(Maximum.mX, Position.mX);
                Maximum.mY = std::max(Maximum.mY, Position.mY);
                Maximum.mZ = std::max(Maximum.mZ, Position.mZ);
            }

            mTarget = (Minimum + Maximum) * 0.5f;
            mDistance = std::clamp((Maximum - Minimum).Length() / std::tan(mFieldOfView * 0.5f), MinimumDistance, MaximumDistance);
        }

        return true;
    }

    return false;
}

bool FViewerPanel::HandleExternalFileDrop(const std::filesystem::path& FilePath, const ImVec2& ScreenPosition) {
    if (!IsVisible() || ScreenPosition.x < mDropTargetMin.x || ScreenPosition.x >= mDropTargetMax.x || ScreenPosition.y < mDropTargetMin.y || ScreenPosition.y >= mDropTargetMax.y) {
        return false;
    }

    return OpenViewerFile(FilePath);
}

void FViewerPanel::DrawContents() {
    mDropTargetMin = ImGui::GetWindowPos();

    const ImVec2 WindowSize{ImGui::GetWindowSize()};

    mDropTargetMax = ImVec2{mDropTargetMin.x + WindowSize.x, mDropTargetMin.y + WindowSize.y};

    if (const FAssetHandle PreviewMesh{mEditorContext.ConsumePreviewMesh()}; PreviewMesh) {
        SetMesh(PreviewMesh);
    }

    DrawMenuBar();
    DrawProperties();
    ImGui::Separator();
    DrawPreview();
}

void FViewerPanel::DrawMenuBar() {
    if (!ImGui::BeginMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
#ifdef OBJ_VIEWER
        if (ImGui::MenuItem("Open Model or Material...")) {
#else
        if (ImGui::MenuItem("Import OBJ...")) {
#endif
#ifdef OBJ_VIEWER
            FString FilePath{OpenFileDialog(mWindowHandle, "./Content/ModelingFiles", "Model and Material Files\0*.bin;*.obj;*.mtl;*.material\0All Files\0*.*\0")};
#else
            FString FilePath{OpenFileDialog(mWindowHandle, "./Content/ModelingFiles", "OBJ Files(*.obj)\0*.obj\0All Files(*.*)\0*.*\0", "obj")};
#endif
            if (!FilePath.empty()) {
#ifdef OBJ_VIEWER
                OpenViewerFile(std::filesystem::path{FilePath.c_str()});
#else
                mEditorToWorldSender.TryEmplace<FMessageImportMesh>(FString{"ObjImport"}, std::move(FilePath), FString{});
#endif
            }
        }

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

void FViewerPanel::DrawProperties() {
    if (FAssetRegistry::TryGet() == nullptr) {
        mPropertyEditor.DrawDisabledText("Asset registry unavailable");
        return;
    }

    ImGui::TextUnformatted("Preview Assets");
    mPropertyEditor.DrawAssetPicker("StaticMesh", *UMesh::StaticClass(), mMeshHandle, [this](FAssetHandle Handle) {
        SetMesh(Handle);
    });

    const UMesh* Mesh{FAssetRegistry::Get().ResolveAsset<UMesh>(mMeshHandle)};
    const FString SlotName{mSelectedMaterialSlot.ToString()};

    if (Mesh != nullptr && ImGui::BeginCombo("Material Slot", SlotName.c_str())) {
        for (const FMeshMaterialSlot& Slot : Mesh->GetMaterialSlots()) {
            const FString Name{Slot.mName.ToString()};

            if (ImGui::Selectable(Name.c_str(), Slot.mName == mSelectedMaterialSlot)) {
                mSelectedMaterialSlot = Slot.mName;
            }
        }

        ImGui::EndCombo();
    }

    mPropertyEditor.DrawAssetPicker("Material Override", *UMaterial::StaticClass(), GetMaterial(), [this](FAssetHandle Handle) {
        SetMaterial(Handle);
    });
}

void FViewerPanel::ResizeSurfaceIfNeeded(ID3D11Device* Device, Uint32 Width, Uint32 Height) {
    if (Device == nullptr || Width == 0 || Height == 0) {
        return;
    }

    if (!mSurface.IsValid()) {
        mSurface.Initialize(Device, Width, Height);
    } else if (Width != mSurfaceWidth || Height != mSurfaceHeight) {
        mSurface.Resize(Device, Width, Height);
    }

    mSurfaceWidth = Width;
    mSurfaceHeight = Height;
}

FMatrix FViewerPanel::MakeCameraWorldMatrix(const FVector3& Eye) const {
    FVector3 Forward{mTarget - Eye};

    Forward.Normalize();

    FVector3 Right{FVector3{0.0f, 0.0f, 1.0f}.Cross(Forward)};

    Right.Normalize();

    FVector3 Up{Forward.Cross(Right)};

    Up.Normalize();

    FMatrix Result{FMatrix::Identity};

    Result.m_[0][0] = Right.mX;
    Result.m_[0][1] = Right.mY;
    Result.m_[0][2] = Right.mZ;
    Result.m_[1][0] = Up.mX;
    Result.m_[1][1] = Up.mY;
    Result.m_[1][2] = Up.mZ;
    Result.m_[2][0] = Forward.mX;
    Result.m_[2][1] = Forward.mY;
    Result.m_[2][2] = Forward.mZ;
    Result.m_[3][0] = Eye.mX;
    Result.m_[3][1] = Eye.mY;
    Result.m_[3][2] = Eye.mZ;
    return Result;
}

void FViewerPanel::UpdatePreviewScene() {
    if (FAssetRegistry::TryGet() == nullptr || mSurfaceWidth == 0 || mSurfaceHeight == 0) {
        return;
    }

    if (FAssetRegistry::Get().ResolveAsset<UMesh>(mMeshHandle) == nullptr) {
        SetMesh({});
    }

    const FMeshSceneData MeshData{mMeshHandle, mMaterialOverrides};

    mPreviewScene.SetMesh(MeshData, FMatrix::Identity);

    FVector3 LightDirection{-FMatrix::CreateFromQuaternion(mOrbitRotation).TransformDirection(-FVector::UnitX) - FVector::UnitZ * 0.75f};

    LightDirection.Normalize();
    mPreviewScene.SetLight(LightDirection, FVector3{1.0f, 1.0f, 1.0f}, 1.0f);
}

FViewMatrices FViewerPanel::BuildPreviewCamera() const {
    const FMatrix OrbitMatrix{FMatrix::CreateFromQuaternion(mOrbitRotation)};
    const FVector3 Offset{OrbitMatrix.TransformDirection(-FVector::UnitX)};
    const FVector3 Eye{mTarget + Offset * mDistance};
    const float Aspect{static_cast<float>(mSurfaceWidth) / static_cast<float>(mSurfaceHeight)};
    FViewMatrices Camera{};

    Camera.mView = MakeCameraWorldMatrix(Eye).Invert();
    Camera.mProjection = FMatrix::CreatePerspectiveFieldOfView(mFieldOfView, Aspect, 0.1f, std::max(1000.0f, mDistance * 4.0f));
    Camera.mViewProjection = Camera.mView * Camera.mProjection;

    FFrustum LocalFrustum{};

    FFrustum::CreateFromMatrix(LocalFrustum, Camera.mProjection.ToSimpleMath());
    LocalFrustum.Transform(Camera.mViewFrustum, Camera.mView.Inverse().ToSimpleMath());

    return Camera;
}

void FViewerPanel::ProcessInput() {
    const ImGuiIO& Input{ImGui::GetIO()};

    if (ImGui::IsItemActive()) {
        const float DeltaYaw{Input.MouseDelta.x * 0.01f};
        const float DeltaPitch{Input.MouseDelta.y * 0.01f};
        const FQuat YawRotation{FQuat::CreateFromAxisAngle(FVector::UnitZ, DeltaYaw)};
        const FMatrix CurrentRotation{FMatrix::CreateFromQuaternion(mOrbitRotation)};
        const FVector Right{CurrentRotation.TransformDirection(FVector::UnitY)};
        const FQuat PitchRotation{FQuat::CreateFromAxisAngle(Right, DeltaPitch)};

        mOrbitRotation = PitchRotation * YawRotation * mOrbitRotation;
        mOrbitRotation.Normalize();
    }

    if (ImGui::IsItemHovered() && Input.MouseWheel != 0.0f) {
        mDistance *= std::pow(0.9f, Input.MouseWheel);
        mDistance = std::clamp(mDistance, MinimumDistance, MaximumDistance);
    }
}

void FViewerPanel::RenderOffscreen(FRenderer& InRenderer) {
    if (mDesiredWidth == 0 || mDesiredHeight == 0) {
        return;
    }

    ResizeSurfaceIfNeeded(InRenderer.GetDevice(), mDesiredWidth, mDesiredHeight);

    if (!mSurface.IsValid()) {
        return;
    }

    if (!mPreviewScene.Initialize(InRenderer)) {
        return;
    }

    UpdatePreviewScene();

    const FRenderScene* PreviewScene{mPreviewScene.Synchronize()};

    if (PreviewScene == nullptr) {
        return;
    }

    FRenderSettings PreviewSettings{};

    PreviewSettings.mClearColor = FVector4{0.12f, 0.13f, 0.15f, 1.0f};

    FRenderView View{};

    View.mTarget = &mSurface;
    View.mCamera = BuildPreviewCamera();
    View.mUseLOD = false;
    View.mSettings = PreviewSettings;
    FOverlayRenderData Overlay{};

    Overlay.mOrientationAxisSize = 100.0f;
    Overlay.mPasses.reset();
    Overlay.SetPassEnabled(EOverlayPass::OrientationAxis, true);
    View.mPasses.reset();
    View.SetPassEnabled(ERenderPass::Opaque, true);
    View.SetPassEnabled(ERenderPass::Translucent, true);
    View.SetPassEnabled(ERenderPass::PostProcessing, true);
    InRenderer.RenderView(View, *PreviewScene, Overlay);
}

void FViewerPanel::ReleaseRenderResources() {
    mPreviewScene.Reset();
    mSurface.Reset();
}

void FViewerPanel::DrawPreview() {
    const ImVec2 Available{ImGui::GetContentRegionAvail()};

    mDesiredWidth = static_cast<Uint32>(std::max(1.0f, Available.x));
    mDesiredHeight = static_cast<Uint32>(std::max(1.0f, Available.y));

    const ImVec2 Size{static_cast<float>(mDesiredWidth), static_cast<float>(mDesiredHeight)};
    const ImVec2 TopLeft{ImGui::GetCursorScreenPos()};
    const ImGuiViewport* Viewport{ImGui::GetWindowViewport()};

    ImGui::InvisibleButton("##PreviewViewport", Size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    if (ID3D11ShaderResourceView* PreviewSRV{mSurface.GetShaderResourceView()}; PreviewSRV != nullptr) {
        ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<ImTextureID>(PreviewSRV), TopLeft, ImVec2{TopLeft.x + Size.x, TopLeft.y + Size.y});
    }

    ProcessInput();

    Uint32 VertexCount{};
    std::size_t TriangleCount{};

    if (const UMesh* Mesh{FAssetRegistry::TryGet() != nullptr ? FAssetRegistry::Get().ResolveAsset<UMesh>(mMeshHandle) : nullptr}; Mesh != nullptr) {
        VertexCount = Mesh->GetVertexAttributeCount(EVertexAttribute::Position);
        TriangleCount = Mesh->GetIndices().size() / 3;
    }

    constexpr float OverlayMargin{12.0f};

    ImGui::SetNextWindowViewport(Viewport->ID);
    ImGui::SetNextWindowPos(ImVec2{TopLeft.x + Size.x - OverlayMargin, TopLeft.y + OverlayMargin}, ImGuiCond_Always, ImVec2{1.0f, 0.0f});
    ImGui::SetNextWindowBgAlpha(0.4f);

    const ImGuiWindowFlags OverlayFlags{ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize};

    if (ImGui::Begin("Mesh Statistics##Viewer", nullptr, OverlayFlags)) {
        ImGui::Text("Vertices: %u", VertexCount);
        ImGui::Text("Triangles: %zu", TriangleCount);
    }

    ImGui::End();
}
