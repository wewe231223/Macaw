/** @file Week3Regression.cpp @brief 메시·재질 임포트와 에디터 상태의 통합 회귀 검사입니다. */
#include "Core/FilePath.h"
#include "Core/Memory.h"
#include "CoreUObject/TObjectIterator.h"
#include "CoreUObject/FObjectDuplicationContext.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Engine/FProject.h"
#include "Engine/FAssetManager.h"
#include "Engine/FAssetFile.h"
#include "Engine/FWorldContext.h"
#include "Engine/FWorldCloner.h"
#include "Engine/SceneSerializer.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/UStaticMesh.h"
#include "Engine/UMaterial.h"
#include "Engine/UTexture2D.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FSceneViewFamily.h"
#include "Editor/EditorEngine.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FPlayInEditorSession.h"
#include "Editor/FMeshViewer.h"
#include "Editor/FEditorAssetCatalog.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FAssetPicker.h"
#include "Editor/FAssetDragDrop.h"
#include "Editor/FEditorContext.h"
#include "FEditorViewportLayout.h"
#include "FAssetSerialization.h"
#include "Serialization/JsonArchive.h"
#include "Renderer/Renderer.h"
#include "Platform/Window.h"

#include <DirectXTex.h>
#include <imgui.h>
#include <wincodec.h>
#include <cstdio>
#include <fstream>
#include <limits>
#include <stdexcept>

/** @brief 성공한 검사의 수입니다. */
Uint32 Checks{};

/** @brief 실패 시 설명을 포함한 예외를 발생시킵니다. @param Condition 검사 결과입니다. @param Message 실패 설명입니다. */
void Check(bool Condition, const char* Message) {
    if (!Condition) {
        throw std::runtime_error{Message};
    }
    ++Checks;
}

/** @brief 테스트용 텍스트 파일을 씁니다. @param Path 파일 경로입니다. @param Text 기록할 내용입니다. */
void WriteText(const std::filesystem::path& Path, const FString& Text) {
    std::ofstream Output{Path, std::ios::binary};
    Output << Text;
    Check(static_cast<bool>(Output), "Write fixture text");
}

/** @brief 클래스·Outer 필터와 삭제·슬롯 재사용에 대한 순회를 검사합니다. */
void TestIterator() {
    auto Parent{NewObject<AActor>()};
    auto Child{NewObject<AStaticMeshActor>()};
    auto Grandchild{NewObject<UStaticMeshComponent>()};
    Check(Child->SetOuter(Parent.get()) && Grandchild->SetOuter(Child.get()), "Iterator outer hierarchy");
    Uint32 Count{};
    for (AActor* Actor : TObjectRange<AActor>{Parent.get()}) {
        Check(Actor == Child.get(), "Derived actor filter");
        ++Count;
    }
    Check(Count == 1, "Outer range count");
    Check(!TObjectIterator<AActor>{Parent.get(), false}, "Exact class filter");
    bool FoundGrandchild{};
    for (const UActorComponent* Component : TObjectRange<UActorComponent>{Parent.get()}) {
        FoundGrandchild = FoundGrandchild || Component == Grandchild.get();
    }
    Check(FoundGrandchild, "Recursive outer filter");
    TObjectIterator<AStaticMeshActor> Iterator{Parent.get()};
    const auto OldHandle{Child->GetHandle()};
    Child.reset();
    auto Replacement{NewObject<AStaticMeshActor>()};
    Check(Replacement->SetOuter(Parent.get()), "Replacement outer");
    Check(Replacement->GetHandle() != OldHandle && !Iterator, "Deleted snapshot excludes reused slots");
    Check(TObjectIterator<const AStaticMeshActor>{Parent.get()}.Get() == Replacement.get(), "Const iterator");
    auto First{NewObject<UStaticMesh>()};
    auto Second{NewObject<UStaticMesh>()};
    TObjectIterator<UStaticMesh> Meshes{};
    const auto Previous{Meshes++};
    Check(Previous.Get() == First.get() && Meshes.Get() == Second.get(), "Post increment snapshot");
    ++Meshes;
    ++Meshes;
    Check(Meshes == std::default_sentinel_t{}, "Stable end iterator");
    Check(TObjectIterator<UStaticMesh>::StaticStruct() != nullptr, "Iterator reflection instantiation");
}

/** @brief OBJ와 연결 재질·공유 텍스처를 임포트합니다. @param Project 테스트 프로젝트입니다. @param Root 원본 파일의 디렉터리입니다. @return 캐시가 소유한 임포트 메시입니다. */
UStaticMesh* TestImport(FProject& Project, const std::filesystem::path& Root) {
    std::filesystem::create_directories(Root);
    std::array<Uint8, 30> Tga{0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 24, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
    std::ofstream Texture{Root / "shared texture.tga", std::ios::binary};
    Texture.write(reinterpret_cast<const char*>(Tga.data()), Tga.size());
    Texture.close();
    WriteText(Root / "fixture.mtl", "newmtl Red\nKd 1 0 0\nmap_Kd shared texture.tga\nnewmtl Green\nKd 0 1 0\nmap_Kd shared texture.tga\n");
    WriteText(Root / "fixture.obj", "mtllib fixture.mtl\nv 0 -2 -1\nv 0 0 -1\nv 0 0 1\nv 0 -2 1\nv 0 0 -1\nv 0 2 -1\nv 0 2 1\nv 0 0 1\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nusemtl Red\nf 1/1 2/2 3/3 4/4\nusemtl Green\nf -4/1 -3/2 -2/3 -1/4\n");
    FString Error{};
    auto* Assets{Project.GetAssets()};
    Check(Assets->ImportMesh(Root / "fixture.obj", "Meshes/Fixture.masset", Error), Error.c_str());
    UStaticMesh* Mesh{Assets->LoadMesh("Meshes/Fixture.masset", Error)};
    Check(Mesh != nullptr && Mesh->GetIndexCount() == 12, "Triangulate OBJ quads and negative indices");
    Check(Mesh->GetMeshData().GetSubMeshes().size() == 2, "Preserve material sections");
    Uint32 Next{};
    UTexture2D* Shared{};
    for (const FSubMesh& Section : Mesh->GetMeshData().GetSubMeshes()) {
        Check(Section.GetFirstIndex() == Next && Section.GetIndexCount() == 6, "Contiguous section ranges");
        Next += Section.GetIndexCount();
        const auto& Slot{Mesh->GetMeshData().GetMaterialSlots()[Section.GetMaterialIndex()]};
        UMaterial* Material{Assets->LoadMaterial(FilePath::FromUtf8(Slot.GetMaterialPath()), Error)};
        Check(Material != nullptr && Material->GetBaseColorTexture() != nullptr, "Auto-linked MTL texture");
        if (Shared == nullptr) {
            Shared = Material->GetBaseColorTexture();
        }
        Check(Shared == Material->GetBaseColorTexture(), "Deduplicate shared source textures");
    }
    FAssetManager Fresh{Assets->GetContentRoot()};
    auto* Reloaded{Fresh.LoadMesh("Meshes/Fixture.masset", Error)};
    Check(Reloaded != nullptr && Reloaded->GetMeshData().GetSubMeshes().size() == 2 && Reloaded->GetMeshData().GetMaterialSlots()[1].GetMaterialPath() == Mesh->GetMeshData().GetMaterialSlots()[1].GetMaterialPath(), "Persist mesh sections and slot paths");
    FMeshData Invalid{Mesh->GetMeshData()};
    Invalid.GetSubMeshes()[0] = FSubMesh{1, 6, 0};
    Check(!Mesh->SetMeshData(Invalid) && Mesh->GetIndexCount() == 12, "Reject corrupt section without mutating mesh");
    const auto Before{Assets->GetAssetPaths(Error)};
    WriteText(Root / "fixture.mtl", "newmtl Red\nKd 1 0 0\nmap_Kd shared texture.tga\nnewmtl Green\nKd 0 1 0\nmap_Kd missing.tga\n");
    Check(!Assets->ImportMesh(Root / "fixture.obj", "Meshes/Fixture.masset", Error), "Report missing referenced texture");
    Check(Before == Assets->GetAssetPaths(Error), "Rollback incomplete dependency assets");
    FAssetManager AfterFailure{Assets->GetContentRoot()};
    Check(AfterFailure.LoadMesh("Meshes/Fixture.masset", Error) != nullptr, "Preserve previously imported mesh on failure");
    std::filesystem::create_directories(Root / "Materials");
    WriteText(Root / "Materials/nested.mtl", "newmtl Red\nKd 1 0 0\nmap_Kd ../shared texture.tga\nnewmtl Green\nKd 0 1 0\nmap_Kd ../shared texture.tga\n");
    std::ifstream Original{Root / "fixture.obj"};
    FString NestedObj{std::istreambuf_iterator<char>{Original}, std::istreambuf_iterator<char>{}};
    NestedObj.replace(0, FString{"mtllib fixture.mtl"}.size(), "mtllib Materials/nested.mtl");
    WriteText(Root / "nested.obj", NestedObj);
    Check(Assets->ImportMesh(Root / "nested.obj", "Meshes/Nested.masset", Error), Error.c_str());
    Check(Assets->LoadMesh("Meshes/Nested.masset", Error) != nullptr, "Resolve textures relative to nested MTL");
    return Mesh;
}

/** @brief 구형 단일 재질 메시의 바이너리 호환성을 검사합니다. @param Assets 에셋 관리자입니다. @param Mesh 원본 메시입니다. */
void TestLegacyMesh(FAssetManager& Assets, const UStaticMesh& Mesh) {
    TArray<Uint8> Bytes{};
    FMemoryArchive Writer{Bytes};
    FMeshData Data{Mesh.GetMeshData()};
    FAssetSerialization::SerializeValues(Writer, Data.GetPositions());
    FAssetSerialization::SerializeValues(Writer, Data.GetNormals());
    FAssetSerialization::SerializeValues(Writer, Data.GetTangents());
    FAssetSerialization::SerializeValues(Writer, Data.GetColors());
    Uint32 Channels{static_cast<Uint32>(Data.GetUVChannels().size())};
    Writer.Serialize("UVChannels", Channels);
    for (auto& Channel : Data.GetUVChannels()) {
        FAssetSerialization::SerializeValues(Writer, Channel);
    }
    FAssetSerialization::SerializeValues(Writer, Data.GetIndices());
    FString Error{};
    Check(FAssetFile::Save(Assets.GetContentRoot() / "Meshes/Legacy.masset", EAssetType::Mesh, Bytes, Error), "Write legacy mesh asset");
    auto* Legacy{Assets.LoadMesh("Meshes/Legacy.masset", Error)};
    Check(Legacy != nullptr && Legacy->GetMeshData().GetSubMeshes().size() == 1 && Legacy->GetMeshData().GetSubMeshes()[0].GetIndexCount() == Mesh.GetIndexCount(), "Read legacy mesh as one section");
}

/** @brief 카메라와 슬롯 재질의 장면 저장과 복제를 검사합니다. @param Project 테스트 프로젝트입니다. @param Mesh 임포트 메시입니다. */
void TestScene(FProject& Project, const UStaticMesh& Mesh) {
    FString Error{};
    FWorldContext World{};
    World.GetWorld()->SetAssets(Project.GetAssets());
    AStaticMeshActor* Actor{World.GetWorld()->SpawnPrimitive(EPrimitiveType::Cube)};
    auto* Component{Actor->GetStaticMeshComponent()};
    Check(Component->SetMeshPath("Meshes/Fixture.masset"), "Assign imported mesh");
    const auto& Slots{Mesh.GetMeshData().GetMaterialSlots()};
    const Uint32 Red{Mesh.GetMeshData().GetSubMeshes()[0].GetMaterialIndex()};
    const Uint32 Green{Mesh.GetMeshData().GetSubMeshes()[1].GetMaterialIndex()};
    Check(Component->SetSlotMaterialPath(Red, Slots[Green].GetMaterialPath()), "Set slot override");
    Check(World.GetEditorCamera().Set({3.0f, 4.0f, 5.0f}, {-13.0f, 42.0f, 2.0f}, 77.0f, 0.25f, 4321.0f), "Set saved editor camera");
    Check(!World.GetEditorCamera().Set({}, {}, 60.0f, 1.0f, 0.5f), "Reject reversed camera clip planes");
    const auto ScenePath{Project.GetAssets()->GetContentRoot() / "Maps/Week3.masset"};
    Check(FSceneSerializer::Save(ScenePath, &World, Error), Error.c_str());
    auto Loaded{FSceneSerializer::Load(ScenePath, Error, Project.GetAssets())};
    Check(Loaded != nullptr && Loaded->GetEditorCamera().GetLocation() == FVector{3.0f, 4.0f, 5.0f} && Loaded->GetEditorCamera().GetRotation() == FRotator{-13.0f, 42.0f, 2.0f} && Loaded->GetEditorCamera().GetFieldOfView() == 77.0f && Loaded->GetEditorCamera().GetNearClip() == 0.25f && Loaded->GetEditorCamera().GetFarClip() == 4321.0f, "Camera scene roundtrip");
    auto* LoadedActor{static_cast<AStaticMeshActor*>(Loaded->GetWorld()->GetPersistentLevel()->GetActors()[0].get())};
    Check(LoadedActor->GetStaticMeshComponent()->GetSlotMaterialPath(Red) == Slots[Green].GetMaterialPath(), "Persist per-slot material override");
    FObjectDuplicationContext Duplication{};
    auto Duplicate{FWorldCloner::Clone(&World, EWorldType::PIE, &Duplication, Error)};
    Check(Duplicate != nullptr, Error.c_str());
    auto* DuplicateActor{static_cast<AStaticMeshActor*>(Duplicate->GetWorld()->GetPersistentLevel()->GetActors()[0].get())};
    Check(DuplicateActor->GetStaticMeshComponent()->GetSlotMaterialPath(Red) == Slots[Green].GetMaterialPath(), "PIE clone slot override");
    FEditorViewportClient Client{nullptr};
    Client.SetViewType(EEditorViewportType::Top);
    Check(Client.SetPerspectiveCamera(World.GetEditorCamera()), "Restore camera while orthographic");
    Check(Client.GetViewType() == EEditorViewportType::Top && Client.GetPerspectiveCamera().GetFieldOfView() == 77.0f, "Preserve orthographic viewport mode");
    Client.SetViewType(EEditorViewportType::Perspective);
    Check(Client.GetCameraLocation() == World.GetEditorCamera().GetLocation(), "Return to restored perspective pose");
    Check(Client.CalcSceneView({800.0f, 600.0f}).mFieldOfView == 77.0f, "Use restored FOV in projection");
    auto Old{FSceneSerializer::Load(Project.GetAssets()->GetContentRoot() / "Maps/Main.masset", Error, Project.GetAssets())};
    Check(Old != nullptr && Old->GetEditorCamera().GetFieldOfView() == 60.0f, "Legacy scene uses default editor camera");
    UEditorEngine Editor{};
    Editor.SetProject(&Project);
    Check(Editor.Initialize(), "Initialize editor camera persistence callbacks");
    Check(Editor.GetViewportClient()->SetPerspectiveCamera(World.GetEditorCamera()), "Set editor viewport camera");
    Check(Editor.GetEditorContext()->IsSceneDirty(), "Camera navigation marks scene dirty");
    Check(Editor.GetEditorContext()->SaveScene("Maps/EditorCamera.masset"), "Save scene captures viewport camera");
    Check(!Editor.GetEditorContext()->IsSceneDirty(), "Saved camera clears dirty state");
    Check(Editor.GetEditorContext()->NewScene(), "New scene resets camera");
    Check(Editor.GetViewportClient()->GetPerspectiveCamera() == FEditorCameraState{}, "Default camera after new scene");
    Check(Editor.GetEditorContext()->LoadScene("Maps/EditorCamera.masset"), "Load editor scene");
    Check(Editor.GetViewportClient()->GetPerspectiveCamera() == World.GetEditorCamera(), "Load scene restores viewport camera callback");
    Editor.Shutdown();
}

/** @brief GPU에서 재질별 인덱스 구간을 실제로 그리는지 검사합니다. @param Renderer 렌더러입니다. @param Assets 에셋 관리자입니다. @param Mesh 임포트 메시입니다. @param Output 캡처 PNG 경로입니다. */
void TestRendering(FRenderer& Renderer, FAssetManager& Assets, UStaticMesh& Mesh, const std::filesystem::path& Output) {
    FPrimitiveSceneProxy Proxy{};
    Proxy.mMesh = &Mesh;
    Proxy.mBounds = Mesh.GetBounds();
    FString Error{};
    for (const auto& Slot : Mesh.GetMeshData().GetMaterialSlots()) {
        UMaterial* Material{Assets.LoadMaterial(FilePath::FromUtf8(Slot.GetMaterialPath()), Error)};
        Check(Material != nullptr && Material->SetTwoSided(true), "Resolve rendered material");
        Proxy.mMaterials.push_back(Material);
    }
    auto Scene{Renderer.AllocateScene()};
    Scene->AddPrimitive(Proxy);
    auto Target{Renderer.CreateRenderTarget(256, 256)};
    FSceneView View{};
    View.mSize = {256.0f, 256.0f};
    View.mLocation = {-5.0f, 0.0f, 0.0f};
    View.mForward = {1.0f, 0.0f, 0.0f};
    View.mView = FMatrix::LookTo(View.mLocation, View.mForward, {0.0f, 0.0f, 1.0f});
    View.mProjection = FMatrix::Perspective(60.0f, 1.0f, 0.05f, 100.0f);
    View.mViewProjection = View.mView * View.mProjection;
    Renderer.BeginFrame(256, 256);
    Renderer.BeginRenderingViewFamily({Scene.get(), Target.get(), View});
    auto* ResourceView{static_cast<ID3D11ShaderResourceView*>(Target->GetNativeTexture())};
    Microsoft::WRL::ComPtr<ID3D11Resource> Resource{};
    ResourceView->GetResource(Resource.GetAddressOf());
    DirectX::ScratchImage Capture{};
    Check(SUCCEEDED(DirectX::CaptureTexture(static_cast<ID3D11Device*>(Renderer.GetNativeDevice()), static_cast<ID3D11DeviceContext*>(Renderer.GetNativeContext()), Resource.Get(), Capture)), "GPU section readback");
    Check(SUCCEEDED(DirectX::SaveToWICFile(*Capture.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatPng, Output.c_str())), "Save rendering evidence");
    Uint32 RedPixels{};
    Uint32 GreenPixels{};
    const auto* Image{Capture.GetImage(0, 0, 0)};
    for (std::size_t Y{}; Y < Image->height; ++Y) {
        for (std::size_t X{}; X < Image->width; ++X) {
            const Uint8* Pixel{Image->pixels + Y * Image->rowPitch + X * 4};
            RedPixels += Pixel[0] > 60 && Pixel[1] < 10;
            GreenPixels += Pixel[1] > 60 && Pixel[0] < 10;
        }
    }
    Check(RedPixels > 1000 && GreenPixels > 1000, "Render both material sections with their own index ranges");
    Check(Renderer.GetStats().mDrawCalls == 2 && Renderer.GetStats().mVisiblePrimitives == 1, "Count sections separately from primitives");
    FMeshViewer Viewer{&Renderer, &Assets};
    Check(Viewer.Open("Meshes/Fixture.masset", Error) && Viewer.IsOpen(), "Independent asset viewer scene");
}

/** @brief 임포트 저장 경로와 논리적 브라우저 경로를 분리하는지 검사합니다. @param Assets 프로젝트 에셋 관리자입니다. @param Mesh 임포트한 메시입니다. */
void TestAssetCatalog(FAssetManager& Assets, const UStaticMesh& Mesh) {
    FString Error{};
    const auto& Slot{Mesh.GetMeshData().GetMaterialSlots()[Mesh.GetMeshData().GetSubMeshes()[0].GetMaterialIndex()]};
    const auto MaterialPath{FilePath::FromUtf8(Slot.GetMaterialPath())};
    Check(*MaterialPath.begin() == L".Imported", "Store new import dependencies outside user asset folders");
    UMaterial* Material{Assets.LoadMaterial(MaterialPath, Error)};
    const auto LegacyDirectory{std::filesystem::path{"Meshes/Legacy_Assets"} / FilePath::FromUtf8(FGuid{}.ToString())};
    const auto LegacyMaterial{LegacyDirectory / "Material_0.mtl"};
    Check(Assets.SaveMaterial(LegacyMaterial, *Material, Error), Error.c_str());
    FMeshData LegacyData{Mesh.GetMeshData()};
    LegacyData.GetMaterialSlots()[Mesh.GetMeshData().GetSubMeshes()[0].GetMaterialIndex()] = FMeshMaterialSlot{"Red", FilePath::ToGenericUtf8(LegacyMaterial)};
    auto Legacy{NewObject<UStaticMesh>()};
    Check(Legacy->SetMeshData(LegacyData) && Assets.SaveMesh("Meshes/Legacy.masset", *Legacy, Error), Error.c_str());
    const auto Orphan{std::filesystem::path{"Meshes/Legacy_Assets"} / FilePath::FromUtf8(FGuid{}.ToString()) / "Unused.mtl"};
    Check(Assets.SaveMaterial(Orphan, *Material, Error), Error.c_str());
    Check(Assets.SaveMaterial("Materials/User_Assets/Visible.mtl", *Material, Error), Error.c_str());
    FEditorAssetCatalog Catalog{&Assets};
    Check(Catalog.Refresh(Error) && Error.empty(), Error.c_str());
    const auto* Entry{Catalog.Find("Meshes/Fixture.masset")};
    Check(Entry != nullptr && Entry->GetType() == EAssetType::Mesh && Entry->GetDisplayPath() == "Meshes/Fixture.masset", "Show actual mesh assets in Meshes");
    Entry = Catalog.Find(MaterialPath);
    Check(Entry != nullptr && Entry->GetType() == EAssetType::Material && Entry->GetDisplayPath().parent_path().parent_path() == "Materials", "Show linked import materials by logical names");
    Entry = Catalog.Find(LegacyMaterial);
    Check(Entry != nullptr && Entry->GetDisplayPath() == "Materials/Legacy/Red.mtl", "Expose active legacy material without GUID storage paths");
    Check(Catalog.Find(Orphan) == nullptr && Catalog.Find("Materials/User_Assets/Visible.mtl") != nullptr, "Hide obsolete imports while preserving ordinary folders");
    Check(Catalog.Find("MESHES/FIXTURE.MASSET") != nullptr, "Resolve payload paths without case sensitivity");
    Check(Catalog.Find(FilePath::FromUtf8(Material->GetBaseColorTexturePath())) != nullptr, "Expose linked texture assets");
    for (const auto& Directory : Catalog.GetDirectories()) {
        Check(FilePath::ToGenericUtf8(Directory).find(".Imported") == FString::npos && FilePath::ToGenericUtf8(Directory).find("Legacy_Assets") == FString::npos, "Hide import implementation directories");
    }
    Check(std::filesystem::exists(Assets.GetContentRoot() / Orphan), "Preserve hidden asset files and existing references");
}

/** @brief 썸네일을 GPU에서 읽어 표시색을 검사합니다. @param Renderer 렌더러입니다. @param View 썸네일의 SRV입니다. @param Output PNG 증거 경로입니다. @param Red 빨강을 세면 true, 초록을 세면 false입니다. @return 선택한 색의 픽셀 수입니다. */
Uint32 ReadThumbnail(FRenderer& Renderer, void* View, const std::filesystem::path& Output, bool Red) {
    Check(View != nullptr, "Create asset thumbnail SRV");
    Microsoft::WRL::ComPtr<ID3D11Resource> Resource{};
    static_cast<ID3D11ShaderResourceView*>(View)->GetResource(Resource.GetAddressOf());
    DirectX::ScratchImage Capture{};
    Check(SUCCEEDED(DirectX::CaptureTexture(static_cast<ID3D11Device*>(Renderer.GetNativeDevice()), static_cast<ID3D11DeviceContext*>(Renderer.GetNativeContext()), Resource.Get(), Capture)), "Read asset thumbnail GPU pixels");
    const auto* Image{Capture.GetImage(0, 0, 0)};
    Check(Image != nullptr && Image->width == 256 && Image->height == 256 && SUCCEEDED(DirectX::SaveToWICFile(*Image, DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatPng, Output.c_str())), "Save asset thumbnail evidence");
    Uint32 Count{};
    for (std::size_t Y{}; Y < Image->height; ++Y) {
        for (std::size_t X{}; X < Image->width; ++X) {
            const Uint8* Pixel{Image->pixels + Y * Image->rowPitch + X * 4};
            Count += Red ? Pixel[0] > 60 && Pixel[1] < 10 : Pixel[1] > 60 && Pixel[0] < 10;
        }
    }
    return Count;
}

/** @brief 메시·재질 미리보기와 캐시 수명 및 변경 반영을 검사합니다. @param Renderer 테스트 렌더러입니다. @param Assets 프로젝트 에셋 관리자입니다. @param Mesh 임포트한 메시입니다. @param Output PNG 증거 디렉터리입니다. */
void TestThumbnails(FRenderer& Renderer, FAssetManager& Assets, const UStaticMesh& Mesh, const std::filesystem::path& Output) {
    FAssetThumbnailCache Cache{&Assets, &Renderer};
    Cache.BeginFrame(16);
    void* MeshView{Cache.GetThumbnail("Meshes/Fixture.masset", EAssetType::Mesh)};
    Check(ReadThumbnail(Renderer, MeshView, Output / "MeshThumbnail.png", true) > 300 && ReadThumbnail(Renderer, MeshView, Output / "MeshThumbnail.png", false) > 300, "Render real mesh sections in thumbnail");
    const auto& Slot{Mesh.GetMeshData().GetMaterialSlots()[Mesh.GetMeshData().GetSubMeshes()[0].GetMaterialIndex()]};
    FString Error{};
    UMaterial* Material{Assets.LoadMaterial(FilePath::FromUtf8(Slot.GetMaterialPath()), Error)};
    void* MaterialView{Cache.GetThumbnail(FilePath::FromUtf8(Slot.GetMaterialPath()), EAssetType::Material)};
    const auto RedPixels{ReadThumbnail(Renderer, MaterialView, Output / "MaterialThumbnailRed.png", true)};
    Check(RedPixels > 1000 && RedPixels < 40000, "Render material on sphere silhouette");
    Check(Material->SetBaseColor({0.0f, 1.0f, 0.0f, 1.0f}), "Edit thumbnail material");
    Check(Cache.GetThumbnail(FilePath::FromUtf8(Slot.GetMaterialPath()), EAssetType::Material) == MaterialView && ReadThumbnail(Renderer, MaterialView, Output / "MaterialThumbnailGreen.png", false) > 1000, "Update material thumbnail without invalidating submitted SRV");
    Check(Material->SetBaseColor({1.0f, 0.0f, 0.0f, 1.0f}), "Restore test material");
    Cache.Invalidate();
    Check(Cache.GetThumbnail("Meshes/Fixture.masset", EAssetType::Mesh) == MeshView, "Refresh retains active thumbnail resources");
    Check(Cache.GetThumbnail(FilePath::FromUtf8(Material->GetBaseColorTexturePath()), EAssetType::Texture) != nullptr, "Share imported texture preview");
    Cache.BeginFrame(0);
    Check(Cache.GetThumbnail("Meshes/Nested.masset", EAssetType::Mesh) == nullptr, "Defer new thumbnail beyond frame rendering budget");
    Cache.BeginFrame();
    Check(Cache.GetThumbnail("Meshes/Nested.masset", EAssetType::Mesh) != nullptr, "Finish pending thumbnail on next frame");
}

/** @brief 실제 ImGui 프레임 사이의 메시·재질 드롭과 검색 팝업을 검사합니다. @param Renderer 테스트 렌더러입니다. @param Assets 프로젝트 에셋 관리자입니다. @param Mesh 임포트한 메시입니다. */
void TestAssetPicker(FRenderer& Renderer, FAssetManager& Assets, const UStaticMesh& Mesh) {
    FEditorAssetCatalog Catalog{&Assets};
    FString Error{};
    Check(Catalog.Refresh(Error), Error.c_str());
    FAssetThumbnailCache Thumbnails{&Assets, &Renderer};
    ImGui::CreateContext();
    ImGuiIO& Input{ImGui::GetIO()};
    Input.IniFilename = nullptr;
    Input.DisplaySize = {800.0f, 500.0f};
    unsigned char* Pixels{};
    int Width{};
    int Height{};
    Input.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);
    FAssetPicker Picker{};
    FString Current{};
    FString SourcePath{};
    EAssetType TargetType{EAssetType::Mesh};
    const char* PayloadType{EditorAssetDragDrop::MeshPayload};
    ImVec2 Source{};
    ImVec2 Thumbnail{};
    ImVec2 Combo{};
    bool Changed{};
    const auto DrawFrame{[&](bool OpenPopup = false) {
        Thumbnails.BeginFrame(16);
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0.0f, 0.0f});
        ImGui::SetNextWindowSize({800.0f, 500.0f});
        ImGui::Begin("Asset picker regression", nullptr, ImGuiWindowFlags_NoSavedSettings);
        ImGui::SetCursorPos({200.0f, 30.0f});
        ImGui::BeginGroup();
        const ImVec2 Origin{ImGui::GetCursorScreenPos()};
        Thumbnail = {Origin.x + 24.0f, Origin.y + ImGui::GetTextLineHeightWithSpacing() + 24.0f};
        Combo = {Origin.x + 130.0f, Origin.y + ImGui::GetTextLineHeightWithSpacing() + 10.0f};
        if (OpenPopup) {
            ImGui::PushID("Target");
            ImGui::OpenPopup("##ThumbnailPicker");
            ImGui::PopID();
        }
        Changed = Picker.Draw(Catalog, Thumbnails, "Target", Current, TargetType);
        ImGui::EndGroup();
        ImGui::SetCursorPos({20.0f, 200.0f});
        ImGui::Selectable("Source", false, ImGuiSelectableFlags_None, {100.0f, 80.0f});
        const ImVec2 Minimum{ImGui::GetItemRectMin()};
        Source = {Minimum.x + 30.0f, Minimum.y + 30.0f};
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(PayloadType, SourcePath.c_str(), SourcePath.size() + 1);
            ImGui::TextUnformatted("Dragging test asset");
            ImGui::EndDragDropSource();
        }
        ImGui::End();
        ImGui::Render();
    }};
    const auto DragTo{[&](const ImVec2& Target) {
        Input.AddMousePosEvent(Source.x, Source.y);
        DrawFrame();
        Input.AddMouseButtonEvent(0, true);
        DrawFrame();
        Input.AddMousePosEvent(Source.x + 25.0f, Source.y);
        DrawFrame();
        Check(ImGui::GetDragDropPayload() != nullptr, "Start thumbnail drag across held mouse frames");
        Input.AddMousePosEvent(Target.x, Target.y);
        DrawFrame();
        DrawFrame();
        Input.AddMouseButtonEvent(0, false);
        DrawFrame();
    }};
    for (EAssetType Type : {EAssetType::Mesh, EAssetType::Material}) {
        TargetType = Type;
        PayloadType = Type == EAssetType::Mesh ? EditorAssetDragDrop::MeshPayload : EditorAssetDragDrop::MaterialPayload;
        SourcePath = Type == EAssetType::Mesh ? FString{"Meshes/Fixture.masset"} : Mesh.GetMeshData().GetMaterialSlots()[Mesh.GetMeshData().GetSubMeshes()[0].GetMaterialIndex()].GetMaterialPath();
        for (bool OntoThumbnail : {true, false}) {
            Current.clear();
            DrawFrame();
            DrawFrame();
            const ImVec2 Target{OntoThumbnail ? Thumbnail : Combo};
            DragTo(Target);
            Check(Changed && Current == SourcePath, OntoThumbnail ? "Assign mesh/material by dropping onto thumbnail" : "Assign mesh/material by dropping onto combo");
        }
    }
    Current.clear();
    TargetType = EAssetType::Mesh;
    DrawFrame();
    DragTo(Thumbnail);
    Check(!Changed && Current.empty(), "Reject incompatible material payload on mesh picker");
    TargetType = EAssetType::Material;
    SourcePath = "Meshes/Fixture.masset";
    DrawFrame();
    DragTo(Thumbnail);
    Check(!Changed && Current.empty(), "Reject forged payload whose catalog type does not match");
    DrawFrame(true);
    DrawFrame();
    Check(ImGui::GetDrawData()->TotalVtxCount > 0, "Draw searchable thumbnail popup without layout assertion");
    ImGui::DestroyContext();
}

/** @brief 전체 에디터에서 브라우저와 선택된 메시 Details를 함께 렌더링합니다. @param Renderer 테스트 렌더러입니다. @param Platform 숨긴 테스트 창입니다. @param Project 테스트 프로젝트입니다. */
void TestEditorAssetUI(FRenderer& Renderer, FWindowsApplication& Platform, FProject& Project) {
    UEditorEngine Engine{};
    Engine.SetProject(&Project);
    Check(Engine.InitializeEditor(&Renderer, &Platform), "Initialize complete editor asset UI");
    AStaticMeshActor* Actor{Engine.GetWorld()->SpawnPrimitive(EPrimitiveType::Cube)};
    Check(Actor != nullptr && Actor->GetStaticMeshComponent()->SetMeshPath("Meshes/Fixture.masset") && Engine.GetEditorContext()->SelectActor(Actor->GetHandle()), "Select imported mesh for actual Details widgets");
    for (Uint32 Frame{}; Frame < 8; ++Frame) {
        Engine.TickFrame(1.0f / 60.0f);
    }
    Check(ImGui::GetDrawData() != nullptr && ImGui::GetDrawData()->TotalVtxCount > 0, "Render browser grid and Details thumbnails without layout assertions");
    Engine.Shutdown();
}

/** @brief INI 분할 정보의 왕복과 잘못된 값의 거부를 검사합니다. @param Renderer 테스트 렌더러입니다. */
void TestLayout(FRenderer& Renderer) {
    UEditorEngine Engine{};
    Check(Engine.Initialize(&Renderer), "Layout editor engine");
    FEditorViewport Viewport{Engine.GetEditorContext()};
    FPlayInEditorSession Play{&Engine, Engine.GetEditorContext(), &Viewport};
    ImGui::CreateContext();
    {
        FEditorViewportLayout Layout{Engine.GetEditorContext(), &Viewport, &Play};
        Check(Layout.Initialize(&Renderer), "Register viewport INI settings");
        Check(Layout.SetSplitRatios(0.32f, 0.68f), "Custom split ratios");
        Layout.SetQuadView(true);
        const FString Ini{ImGui::SaveIniSettingsToMemory()};
        Check(Ini.find("[MacawViewport][Layout]") != FString::npos, "Write custom INI section");
        Check(Layout.SetSplitRatios(0.5f, 0.5f), "Change split ratios before reload");
        Layout.SetQuadView(false);
        ImGui::LoadIniSettingsFromMemory(Ini.c_str());
        Check(std::abs(Layout.GetHorizontalRatio() - 0.32f) < 0.00001f && std::abs(Layout.GetVerticalRatio() - 0.68f) < 0.00001f && Layout.IsQuadView(), "Restore split ratios and quad view");
        ImGui::LoadIniSettingsFromMemory("[MacawViewport][Layout]\nHorizontalRatio=nan\nVerticalRatio=-1\nQuadView=8\nSingleViewport=999\n");
        Check(Layout.GetHorizontalRatio() > 0.31f && Layout.GetVerticalRatio() > 0.67f && Layout.IsQuadView(), "Ignore invalid INI layout values");
    }
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::DestroyContext();
    Engine.Shutdown();
}

/** @brief 기능 회귀 검사를 실행합니다. @return 모든 검사에 성공하면 0입니다. */
int main() {
    try {
        const HRESULT Com{CoInitializeEx(nullptr, COINIT_MULTITHREADED)};
        Check(SUCCEEDED(Com), "Initialize COM for texture import");
        const auto Root{std::filesystem::absolute(".tools/week3-regression") / FilePath::FromUtf8(FGuid{}.ToString())};
        FProject Project{};
        FString Error{};
        Check(Project.Create(Root / "Project/Test.mproject", "Templates/Project", Error), Error.c_str());
        TestIterator();
        UStaticMesh* Mesh{TestImport(Project, Root / "Source models")};
        TestLegacyMesh(*Project.GetAssets(), *Mesh);
        TestScene(Project, *Mesh);
        TestAssetCatalog(*Project.GetAssets(), *Mesh);
        FWindowsApplication Platform{};
        Check(Platform.Create(L"Week3 Regression", 1280, 900, false), "Hidden GPU test window");
        FRenderer Renderer{};
        Renderer.SetProjectPaths(Project.GetAssets()->GetContentRoot(), Root / "ShaderCache");
        Check(Renderer.Initialize(Platform.GetWindowHandle(), 256, 256), "Initialize renderer");
        TestRendering(Renderer, *Project.GetAssets(), *Mesh, Root / "Sections.png");
        TestThumbnails(Renderer, *Project.GetAssets(), *Mesh, Root);
        TestAssetPicker(Renderer, *Project.GetAssets(), *Mesh);
        TestLayout(Renderer);
        TestEditorAssetUI(Renderer, Platform, Project);
        Renderer.Shutdown();
        Platform.Shutdown();
        CoUninitialize();
        std::printf("PASS: %u checks. Evidence: %s\n", Checks, FilePath::ToUtf8(Root).c_str());
        return 0;
    } catch (const std::exception& Error) {
        std::fprintf(stderr, "FAIL after %u checks: %s\n", Checks, Error.what());
        return 1;
    }
}
