#include "MeshMaterialRegression.h"
#include "Editor/UEditorEngine.h"
#include "Engine/FWorldContext.h"
#include "Engine/Serialization/FWorldSerializer.h"
#include "Engine/Serialization/FSceneSerializer.h"
#include "Engine/Serialization/FWorldCloner.h"
#include "Asset/FAssetRegistry.h"
#include "Asset/FMeshAssetSerializer.h"
#include "Asset/FMaterialAssetSerializer.h"
#include "Asset/UMesh.h"
#include "Asset/USurfaceOpaque.h"
#include "Asset/UTexture.h"
#include "Render/FMaterialBuffer.h"
#include "Render/FMeshDrawCommandCache.h"
#include "Render/FRenderAssetResources.h"
#include "Render/FMeshDrawState.h"
#include "Render/FMeshRenderer.h"
#include "Render/FRenderBuffers.h"
#include "Render/FOffScreenRenderSurface.h"
#include "Render/FRenderScene.h"
#include "Render/FRenderView.h"
#include "RenderCore/FStaticMeshSceneProxy.h"
#include "Serialization/FArchiveJson.h"
#include "Serialization/FJsonFile.h"
#include "World/UWorld.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/Component/USubUVComponent.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace {
    bool Check(bool Passed, const char* Label) {
        std::printf("%s: %s\n", Passed ? "PASS" : "FAIL", Label);
        return Passed;
    }

    bool Convert() {
        UEngine Engine{};

        Engine.Initialize();
        FAssetRegistry& Assets{FAssetRegistry::Get()};

        return Check(Assets.DiscoverAssets("Content") && Assets.LoadAssetsOfType(EAssetType::Texture) && Assets.ConvertLegacyAssets(), "Convert bundled MTL libraries and legacy meshes to runtime assets");
    }

    void MakeSource(const std::filesystem::path& Root) {
        std::filesystem::create_directories(Root);
        std::filesystem::copy_file("Content/Texture/checkerboard.png", Root / "Sheet.png");
        std::ofstream Material{Root / "Mesh.mtl"};

        Material << "newmtl Blue\nKd 0 0 1\nd 0.5\nmap_Kd Sheet.png\nnewmtl Red\nKd 1 0 0\n";
        std::ofstream Mesh{Root / "Mesh.obj"};

        Mesh << "mtllib Mesh.mtl\n";

        for (int Y{}; Y <= 8; ++Y) {
            for (int X{}; X <= 12; ++X) {
                Mesh << "v " << X << ' ' << Y << " 0\nvt " << X / 12.0f << ' ' << Y / 8.0f << "\nvn 0 0 1\n";
            }
        }

        for (int Strip{}; Strip < 3; ++Strip) {
            Mesh << (Strip == 1 ? "usemtl Blue\n" : "usemtl Red\n");

            for (int Y{}; Y < 8; ++Y) {
                for (int X{Strip * 4}; X < Strip * 4 + 4; ++X) {
                    const int A{Y * 13 + X + 1};
                    const int B{A + 1};
                    const int C{A + 14};
                    const int D{A + 13};

                    Mesh << "f ";

                    for (int Index : {A, B, C, D}) {
                        Mesh << Index << '/' << Index << '/' << Index << ' ';
                    }

                    Mesh << '\n';
                }
            }
        }
    }

    bool CheckMesh(const UMesh& Mesh) {
        bool Passed{Mesh.GetMaterialSlots().size() == 2 && Mesh.GetMaterialSlotName(0) == FName{"Red"} && Mesh.GetMaterialSlotName(1) == FName{"Blue"} && Mesh.GetSections().size() == 3};

        for (Uint32 Level{}; Level < Mesh.GetLODCount(); ++Level) {
            Uint32 Count{};

            for (const FMeshSection& Section : Mesh.GetSections(static_cast<int>(Level))) {
                Passed = Passed && Section.mMaterialSlotIndex < Mesh.GetMaterialSlots().size() && Section.mFirstIndex == Count;
                Count += Section.mIndexCount;
            }

            Passed = Passed && Count == Mesh.GetIndexCount(static_cast<int>(Level));
        }

        return Passed;
    }

    void MakeLegacy(const std::filesystem::path& Source) {
        std::ofstream Output{Source / "Legacy.bin", std::ios::binary};
        const Uint32 Magic{0x4d45534d};
        const Uint32 Version{4};
        const Uint32 Count{3};
        const Uint32 Empty{};
        const Uint32 SectionCount{1};
        const std::array<FVector3, 3> Positions{FVector3{}, FVector3{1.0f, 0.0f, 0.0f}, FVector3{0.0f, 1.0f, 0.0f}};
        const std::array<FVector3, 3> Normals{FVector3::UnitZ, FVector3::UnitZ, FVector3::UnitZ};
        const std::array<FVector2, 3> UVs{FVector2{0.0f, 0.25f}, FVector2{1.0f, 0.25f}, FVector2{0.0f, 1.0f}};
        const std::array<Uint32, 3> Indices{0, 1, 2};
        const std::array<FColor4, 3> Colors{FColor4{1.0f, 1.0f, 1.0f, 1.0f}, FColor4{1.0f, 1.0f, 1.0f, 1.0f}, FColor4{1.0f, 1.0f, 1.0f, 1.0f}};

        Output.write(reinterpret_cast<const char*>(&Magic), sizeof(Magic));
        Output.write(reinterpret_cast<const char*>(&Version), sizeof(Version));
        Output.write(reinterpret_cast<const char*>(&Count), sizeof(Count));
        Output.write(reinterpret_cast<const char*>(Positions.data()), sizeof(Positions));
        Output.write(reinterpret_cast<const char*>(&Count), sizeof(Count));
        Output.write(reinterpret_cast<const char*>(Normals.data()), sizeof(Normals));
        Output.write(reinterpret_cast<const char*>(&Count), sizeof(Count));
        Output.write(reinterpret_cast<const char*>(UVs.data()), sizeof(UVs));
        Output.write(reinterpret_cast<const char*>(&Count), sizeof(Count));
        Output.write(reinterpret_cast<const char*>(Indices.data()), sizeof(Indices));
        Output.write(reinterpret_cast<const char*>(&Empty), sizeof(Empty));
        Output.write(reinterpret_cast<const char*>(&Empty), sizeof(Empty));
        Output.write(reinterpret_cast<const char*>(&SectionCount), sizeof(SectionCount));
        Output.write(reinterpret_cast<const char*>(&Count), sizeof(Count));
        Output.write(reinterpret_cast<const char*>(&Count), sizeof(Count));
        Output.write(reinterpret_cast<const char*>(Colors.data()), sizeof(Colors));
        std::ofstream Metadata{Source / "Legacy.bin.meta"};

        Metadata << "{\"Guid\":\"" << FGuid::NewGuid().ToString().c_str() << "\",\"FlipUV\":true}";
    }

    TArray<Uint8> RenderPixels(ID3D11Device* Device, ID3D11DeviceContext* Context, FRenderAssetResources& Resources, FAssetHandle MeshHandle, Uint32 Level, bool Override) {
        const UMesh* Mesh{FAssetRegistry::Get().ResolveAsset<UMesh>(MeshHandle)};
        FMeshBatch Batch{};
        FMeshDrawCommandCache Cache{};
        TArray<Uint32> Indices{};

        Batch.mMeshHandle = MeshHandle;
        Batch.mLODLevel = Level;

        if (Override) {
            Batch.mMaterialOverrides.push_back(FMeshMaterialBinding{FName{"Blue"}, Mesh->GetDefaultMaterial(0)});
        }

        BuildMeshBatchElements(*Mesh, Level, Batch.mElements);
        Cache.BeginUpdate();
        Cache.AddMeshBatch(Batch, Resources.GetMaterialBuffer(), &Resources, Indices);
        Cache.EndUpdate();
        TArray<FMeshDrawRecord> Records{};
        TArray<FMeshDrawCommandBatch> Commands{};

        for (Uint32 Index : Indices) {
            const std::shared_ptr<const FMeshDrawCommand>& Command{Cache.GetCommands()[Index]};

            Commands.push_back(FMeshDrawCommandBatch{Command, static_cast<Uint32>(Records.size()), 1});
            Records.push_back(FMeshDrawRecord{0, Command->mMaterialIndex});
        }

        FOffScreenRenderSurface Surface{};
        FRenderBuffers Buffers{};
        FViewMatrices Camera{};

        Surface.Initialize(Device, 128, 128);
        Camera.mView = (FMatrix::CreateRotationY(3.14159265f) * FMatrix::CreateTranslation(FVector3{6.0f, 4.0f, 20.0f})).Inverse();
        Camera.mProjection = FMatrix::CreateOrthographic(14.0f, 14.0f, 0.1f, 100.0f);
        Camera.mViewProjection = Camera.mView * Camera.mProjection;

        if (!Surface.IsValid() || !Buffers.Initialize(Device, Context) || !Buffers.PrepareView(Device, Context, Camera, Surface.GetViewport(), FVector4{}, TArray<FMatrix>{FMatrix::Identity}, Records)) {
            return {};
        }

        D3D11_SAMPLER_DESC Description{};
        Microsoft::WRL::ComPtr<ID3D11SamplerState> Sampler{};

        Description.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        Description.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        Description.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        Description.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        Description.ComparisonFunc = D3D11_COMPARISON_NEVER;
        Description.MaxLOD = D3D11_FLOAT32_MAX;

        if (FAILED(Device->CreateSamplerState(&Description, Sampler.GetAddressOf()))) {
            return {};
        }

        constexpr float ClearColor[4]{};

        Surface.Clear(Context, ClearColor);
        Surface.Bind(Context);
        Context->PSSetSamplers(0, 1, Sampler.GetAddressOf());
        FMeshRenderer Renderer{};

        Renderer.Draw(FRenderContext{Context, &Resources, Resources.GetMaterialBuffer().GetSRV(), &Buffers}, Commands, ERenderMode::Unlit);
        Context->OMSetRenderTargets(0, nullptr, nullptr);
        Microsoft::WRL::ComPtr<ID3D11Resource> Resource{};
        Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture{};
        Microsoft::WRL::ComPtr<ID3D11Texture2D> Readback{};
        D3D11_TEXTURE2D_DESC TextureDescription{};

        Surface.GetShaderResourceView()->GetResource(Resource.GetAddressOf());
        Resource.As(&Texture);
        Texture->GetDesc(&TextureDescription);
        TextureDescription.Usage = D3D11_USAGE_STAGING;
        TextureDescription.BindFlags = 0;
        TextureDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        TextureDescription.MiscFlags = 0;

        if (FAILED(Device->CreateTexture2D(&TextureDescription, nullptr, Readback.GetAddressOf()))) {
            return {};
        }

        Context->CopyResource(Readback.Get(), Texture.Get());
        D3D11_MAPPED_SUBRESOURCE Mapped{};

        if (FAILED(Context->Map(Readback.Get(), 0, D3D11_MAP_READ, 0, &Mapped))) {
            return {};
        }

        TArray<Uint8> Pixels{};

        Pixels.resize(128 * 128 * 4);

        for (Uint32 Row{}; Row < 128; ++Row) {
            std::memcpy(Pixels.data() + Row * 128 * 4, static_cast<const Uint8*>(Mapped.pData) + Row * Mapped.RowPitch, 128 * 4);
        }

        Context->Unmap(Readback.Get(), 0);
        return Pixels;
    }

    bool Run() {
        UEditorEngine Engine{};

        Engine.Initialize();
        FAssetRegistry& Assets{FAssetRegistry::Get()};
        bool Passed{Check(Assets.Initialize(), "Native sample assets initialize")};

        if (!Passed) {
            return false;
        }

        UWorld Sample{};

        Sample.Initialize();
        const bool LoadedSample{FSceneSerializer::Load(Sample, "scenes/Default.json")};
        const UStaticMeshComponent* Floor{LoadedSample && Sample.GetActors().size() >= 2 ? Sample.GetActors()[0]->GetComponent<UStaticMeshComponent>() : nullptr};
        const UStaticMeshComponent* Cube{LoadedSample && Sample.GetActors().size() >= 2 ? Sample.GetActors()[1]->GetComponent<UStaticMeshComponent>() : nullptr};

        Passed = Check(Floor != nullptr && Cube != nullptr && Floor->GetMaterial(0) == Assets.EnsureDefaultStaticMeshMaterial() && Cube->GetMaterial(0) == Assets.FindAsset(FAssetPath{"/Game/System/Material/Blue.mtl.materials/M_Blue.material"}), "Converted sample scenes retain their original material assignments") && Passed;
        Sample.CleanupWorld();

        const std::filesystem::path Source{std::filesystem::current_path() / "Source"};

        MakeSource(Source);
        const FAssetHandle Viewer{Assets.LoadViewerAsset(Source / "Mesh.obj")};
        const FAssetHandle Imported{Assets.ImportMesh(Source / "Mesh.obj", "/Game/Imported")};
        const UMesh* ViewerMesh{Assets.ResolveAsset<UMesh>(Viewer)};
        const UMesh* Mesh{Assets.ResolveAsset<UMesh>(Imported)};

        Passed = Check(ViewerMesh != nullptr && Mesh != nullptr && CheckMesh(*ViewerMesh) && CheckMesh(*Mesh), "Viewer and persistent import share stable named slots and LOD sections") && Passed;

        if (ViewerMesh == nullptr || Mesh == nullptr) {
            return false;
        }

        const FAssetHandle Red{Mesh->GetDefaultMaterial(0)};
        const FAssetHandle Blue{Mesh->GetDefaultMaterial(1)};
        const UMaterial* RedMaterial{Assets.ResolveAsset<UMaterial>(Red)};
        const UMaterial* BlueMaterial{Assets.ResolveAsset<UMaterial>(Blue)};

        Passed = Check(Red && Blue && Red != Blue && RedMaterial != nullptr && BlueMaterial != nullptr && BlueMaterial->GetBlendMode() == EMaterialBlendMode::Translucent, "Each MTL surface becomes an independent material with its own blend state") && Passed;
        Passed = Check(std::distance(std::filesystem::directory_iterator(Source), std::filesystem::directory_iterator{}) == 3, "Viewer leaves source files and metadata untouched") && Passed;
        Passed = Check(!Assets.ImportMesh(Source / "Mesh.obj", "/Game/Imported"), "Import rejects an existing destination") && Passed;
        MakeLegacy(Source);
        const std::uintmax_t LegacySize{std::filesystem::file_size(Source / "Legacy.bin")};
        const UMesh* LegacyViewer{Assets.ResolveAsset<UMesh>(Assets.LoadViewerAsset(Source / "Legacy.bin"))};

        Passed = Check(LegacyViewer != nullptr && LegacyViewer->GetVertexAttributeData<EVertexAttribute::UV>()[0].mY == 0.75f && std::filesystem::file_size(Source / "Legacy.bin") == LegacySize, "Legacy Viewer imports apply UV metadata without rewriting source binaries") && Passed;
        const USurfaceOpaque* ImportedBlue{Assets.ResolveAsset<USurfaceOpaque>(Blue)};

        Passed = Check(ImportedBlue != nullptr && Assets.GetAssetGuid(ImportedBlue->GetParameters().mDiffuseTexture.GetTexture()) != nullptr, "Persistent import copies a texture even after Viewer has loaded it transiently") && Passed;
        const std::size_t AssetCount{Assets.GetAssetHandles(*UAsset::StaticClass()).size()};

        {
            std::ofstream Invalid{Source / "Invalid.obj"};

            Invalid << "mtllib Mesh.mtl\nv 0 0 0\nv 1 0 0\nv 0 1 0\nusemtl Red\nf 1 2 99\n";
        }

        Passed = Check(!Assets.ImportMesh(Source / "Invalid.obj", "/Game/Imported") && Assets.GetAssetHandles(*UAsset::StaticClass()).size() == AssetCount && !std::filesystem::exists("Content/Imported/Invalid.materials") && !std::filesystem::exists("Content/Imported/Invalid.bin"), "A failed import removes only its newly created runtime assets and files") && Passed;

        Microsoft::WRL::ComPtr<ID3D11Device> Device{};
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context{};
        D3D_FEATURE_LEVEL Feature{};

        if (!Check(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, Device.GetAddressOf(), &Feature, Context.GetAddressOf())), "WARP device initializes")) {
            return false;
        }

        FRenderAssetResources Resources{};

        Passed = Check(Resources.Initialize(Device.Get()) && Resources.GetMaterialBuffer().Synchronize(Context.Get()), "Independent material GPU records synchronize") && Passed;
        FMaterialBuffer& Buffer{Resources.GetMaterialBuffer()};
        UWorld& World{Engine.GetEditorWorldContext().GetWorld()};
        AActor* Actor{World.SpawnStaticMeshActor(Imported, FVector3{})};
        UStaticMeshComponent* Component{Actor != nullptr ? static_cast<UStaticMeshComponent*>(Actor->GetRootComponent()) : nullptr};

        if (!Check(Component != nullptr, "Mesh actor spawns using mesh defaults")) {
            return false;
        }

        USubUVComponent* Animation{Actor->AddComponent<USubUVComponent>()};

        Animation->SetTargetComponent(Component);
        Animation->SetTargetMaterialSlot(FName{"Blue"});
        Animation->SetUVTransform(FUVTransform{FVector2{0.5f, 0.5f}, FVector2{0.25f, 0.0f}});
        Passed = Check(Component->SetMaterialOverride(FName{"Blue"}, Red) && Component->GetMaterial(0) == Red && Component->GetMaterial(1) == Red && !Component->SetMaterialOverride(FName{"Missing"}, Blue), "Only a named valid slot is replaced") && Passed;
        Passed = Check(Animation->GetTargetMaterialSlots().size() == 2 && Animation->GetTargetMaterialSlot() == FName{"Blue"}, "SubUV targets mesh slots independently of overrides") && Passed;

        FMeshDrawCommandCache Cache{};

        Cache.BeginUpdate();

        for (Uint32 Level{}; Level < Mesh->GetLODCount(); ++Level) {
            FMeshBatch Batch{};
            TArray<Uint32> Commands{};

            Batch.mMeshHandle = Imported;
            Batch.mLODLevel = Level;
            Batch.mMaterialOverrides.push_back(FMeshMaterialBinding{FName{"Blue"}, Red});
            BuildMeshBatchElements(*Mesh, Level, Batch.mElements);
            Cache.AddMeshBatch(Batch, Buffer, &Resources, Commands);
            Passed = Passed && Commands.size() == Batch.mElements.size();

            for (Uint32 Index : Commands) {
                const FMeshDrawCommand& Command{*Cache.GetCommands()[Index]};

                Passed = Passed && Command.mMaterialIndex == Buffer.GetMaterialIndex(*RedMaterial) && Command.mState.mPipelineHandle == SelectMaterialPipeline(*RedMaterial) && Command.mMaterialSlotName == Mesh->GetMaterialSlotName(Command.mState.mMaterialSlotIndex);

                for (const FMeshDrawPipelineState& State : Command.mPipelineStates) {
                    Passed = Passed && State.IsValid();
                }
            }
        }

        Cache.EndUpdate();
        Passed = Check(Passed, "Every LOD draw resolves overrides and UV targets through the same slot") && Passed;
        const TArray<Uint8> DefaultPixels{RenderPixels(Device.Get(), Context.Get(), Resources, Imported, 0, false)};
        const TArray<Uint8> OverridePixels{RenderPixels(Device.Get(), Context.Get(), Resources, Imported, 0, true)};
        Uint32 RedPixels{};

        for (std::size_t Index{}; Index + 3 < OverridePixels.size(); Index += 4) {
            RedPixels += OverridePixels[Index] > 200 && OverridePixels[Index + 1] < 10 && OverridePixels[Index + 2] < 10 ? 1 : 0;
        }

        bool PixelsMatch{RedPixels > 1000 && DefaultPixels != OverridePixels};

        std::printf("Red pixels: %u, default equals override: %d\n", RedPixels, DefaultPixels == OverridePixels);

        for (Uint32 Level{}; Level < Mesh->GetLODCount(); ++Level) {
            const TArray<Uint8> ImportedPixels{RenderPixels(Device.Get(), Context.Get(), Resources, Imported, Level, false)};
            const TArray<Uint8> ViewerPixels{RenderPixels(Device.Get(), Context.Get(), Resources, Viewer, Level, false)};
            const TArray<Uint8> ReplacedPixels{RenderPixels(Device.Get(), Context.Get(), Resources, Imported, Level, true)};
            const TArray<Uint8> ViewerReplacedPixels{RenderPixels(Device.Get(), Context.Get(), Resources, Viewer, Level, true)};
            Uint32 Differences{};
            int Maximum{};

            for (std::size_t Index{}; Index < ImportedPixels.size(); ++Index) {
                const int Difference{std::abs(static_cast<int>(ImportedPixels[Index]) - DefaultPixels[Index])};

                Differences += Difference != 0 ? 1 : 0;
                Maximum = std::max(Maximum, Difference);
            }

            std::printf("LOD %u: %u channels differ from base LOD, max %d/255, Viewer/import match %d, override match %d\n", Level, Differences, Maximum, ViewerPixels == ImportedPixels, ReplacedPixels == OverridePixels && ViewerReplacedPixels == OverridePixels);
            PixelsMatch = Maximum <= 2 && ViewerPixels == ImportedPixels && ReplacedPixels == OverridePixels && ViewerReplacedPixels == OverridePixels && PixelsMatch;
        }

        Passed = Check(PixelsMatch, "WARP validates identical Viewer/import pixels per LOD and slot overrides; textured simplification stays within 2/255") && Passed;
        FRenderScene SkyScene{1};
        FSceneUpdateBatch SkyUpdates{};
        FPrimitiveSceneUpdate SkyUpdate{};

        SkyUpdate.mComponentHandle = Component->GetHandle();
        SkyUpdate.mType = ESceneUpdateType::Create;
        SkyUpdate.mProxy = std::make_unique<FStaticMeshSceneProxy>(Component->GetHandle(), Actor->GetHandle(), Component->GetRenderTransform(), FMeshSceneData{Imported, {}});
        SkyUpdates.mPrimitiveUpdates.push_back(std::move(SkyUpdate));
        FOffScreenRenderSurface SkyTarget{};
        FRenderView SkyView{};
        TArray<FVisibleMeshDrawCommand> SkyCommands{};

        SkyTarget.Initialize(Device.Get(), 128, 128);
        SkyView.mTarget = &SkyTarget;
        SkyView.mUseLOD = false;
        SkyView.mSettings.mBRenderSky = false;
        SkyView.mCamera.mView = (FMatrix::CreateRotationY(3.14159265f) * FMatrix::CreateTranslation(FVector3{6.0f, 4.0f, 20.0f})).Inverse();
        SkyView.mCamera.mProjection = FMatrix::CreateOrthographic(14.0f, 14.0f, 0.1f, 100.0f);
        SkyView.mCamera.mViewProjection = SkyView.mCamera.mView * SkyView.mCamera.mProjection;
        Assets.ResolveAsset<USurfaceOpaque>(Blue)->SetShader(EMaterialShader::Sky);
        SkyScene.Synchronize(SkyUpdates, Buffer);
        SkyView.CollectMeshDrawCommands(SkyScene, SkyCommands);
        bool SkySlotsValid{SkyCommands.size() == 2};

        Assets.ResolveAsset<USurfaceOpaque>(Red)->SetShader(EMaterialShader::Sky);
        SkyScene.Synchronize(SkyUpdates, Buffer);
        SkyView.CollectMeshDrawCommands(SkyScene, SkyCommands);
        SkySlotsValid = SkyCommands.empty() && SkySlotsValid;
        Assets.ResolveAsset<USurfaceOpaque>(Blue)->SetShader(EMaterialShader::Surface);
        SkyScene.Synchronize(SkyUpdates, Buffer);
        SkyView.CollectMeshDrawCommands(SkyScene, SkyCommands);
        SkySlotsValid = SkyCommands.size() == 1 && SkySlotsValid;
        Assets.ResolveAsset<USurfaceOpaque>(Red)->SetShader(EMaterialShader::Surface);
        Passed = Check(SkySlotsValid, "Sky visibility follows each material slot, including mixed meshes with sky in the first slot") && Passed;
        Passed = Check(SelectMaterialPipeline(*BlueMaterial) == Assets.FindAsset(FAssetPath{"/Game/Pipeline/TexturedBase"}) && SelectMaterialPipeline(*RedMaterial) == Assets.FindAsset(FAssetPath{"/Game/Pipeline/Base"}), "Renderer selects textured and untextured shader families") && Passed;

        bool ShaderPipelinesValid{true};

        for (EMaterialShader Shader : {EMaterialShader::Surface, EMaterialShader::Base, EMaterialShader::Textured, EMaterialShader::BaseSolid, EMaterialShader::Alternate, EMaterialShader::AlternateSolid, EMaterialShader::Sky, EMaterialShader::Gizmo}) {
            USurfaceOpaque* Material{Assets.ResolveAsset<USurfaceOpaque>(Blue)};

            Material->SetShader(Shader);
            const UPipeline* Pipeline{Assets.ResolveAsset<UPipeline>(SelectMaterialPipeline(*Material))};
            const FPipelineRenderResource* Resource{Pipeline != nullptr ? Resources.GetPipeline(*Pipeline) : nullptr};

            ShaderPipelinesValid = ShaderPipelinesValid && Resource != nullptr;

            for (std::size_t Mode{}; Resource != nullptr && Mode < static_cast<std::size_t>(ERenderMode::Max); ++Mode) {
                const ERenderMode Resolved{Pipeline->ResolveRenderMode(static_cast<ERenderMode>(Mode))};
                FMeshDrawPipelineState State{};

                ShaderPipelinesValid = Resource->BuildMaterialState(Resolved, Material->GetBlendMode(), State) && State.IsValid() && ShaderPipelinesValid;
            }
        }

        Assets.ResolveAsset<USurfaceOpaque>(Blue)->SetShader(EMaterialShader::Surface);
        Passed = Check(ShaderPipelinesValid, "All material shader choices build their supported view modes on WARP") && Passed;
        USurfaceOpaque* EditedMaterial{Assets.ResolveAsset<USurfaceOpaque>(Blue)};
        FSurfaceParameters EditedParameters{EditedMaterial->GetParameters()};

        EditedParameters.mDiffuse.mW = 0.35f;
        EditedParameters.mShininess = 23.0f;
        EditedParameters.mEmissive = FVector3{0.2f, 0.3f, 0.4f};
        EditedMaterial->SetParameters(EditedParameters);
        EditedMaterial->SetShader(EMaterialShader::Alternate);
        FMaterialGPUSlot ExpectedMaterial{};

        EditedMaterial->BuildGPUData(ExpectedMaterial);
        Passed = Check(Assets.SaveMaterial(Blue) && Buffer.Synchronize(Context.Get()), "Material edits save independently of mesh actors and upload to the GPU") && Passed;
        FMeshBatch EditedBatch{};
        TArray<Uint32> EditedCommands{};

        EditedBatch.mMeshHandle = Imported;
        BuildMeshBatchElements(*Mesh, 0, EditedBatch.mElements);
        Cache.BeginUpdate();
        Cache.AddMeshBatch(EditedBatch, Buffer, &Resources, EditedCommands);
        Cache.EndUpdate();
        bool SawEditedMaterial{};

        for (Uint32 Index : EditedCommands) {
            const FMeshDrawCommand& Command{*Cache.GetCommands()[Index]};

            if (Command.mMaterialSlotName == FName{"Blue"}) {
                SawEditedMaterial = Command.mState.mPipelineHandle == Assets.FindAsset(FAssetPath{"/Game/Pipeline/Alternate"}) && Command.mPass == ERenderPass::Translucent;
            }
        }

        Passed = Check(SawEditedMaterial, "A material shader edit refreshes draw commands without changing the component") && Passed;
        Passed = Check(Engine.RequestStartPIE(), "PIE start is requested") && Passed;
        Engine.Tick(0.0f);
        const FWorldContext* PIE{Engine.GetPIEWorldContext()};
        AActor* PIEActor{PIE != nullptr && !PIE->GetWorld().GetActors().empty() ? PIE->GetWorld().GetActors().back().get() : nullptr};
        const UStaticMeshComponent* PIEComponent{PIEActor != nullptr ? PIEActor->GetComponent<UStaticMeshComponent>() : nullptr};

        Passed = Check(PIEComponent != nullptr && PIEComponent->GetMeshHandle() == Imported && PIEComponent->GetMaterialOverride(FName{"Blue"}) == Red, "PIE clones mesh identity and named overrides") && Passed;
        Engine.RequestStopPIE();
        Engine.Tick(0.0f);
        rapidjson::Document Saved{rapidjson::kObjectType};
        FArchiveJson Save{Saved, Saved.GetAllocator()};

        Passed = Check(FWorldSerializer::Save(World, Save), "World with named overrides saves") && Passed;
        World.CleanupWorld();
        World.Initialize(EWorldType::Editor);
        FArchiveJson Load{Saved};

        Passed = Check(FWorldSerializer::Load(World, Load), "World with named overrides reloads") && Passed;
        const UStaticMeshComponent* Restored{World.GetActors().empty() ? nullptr : World.GetActors().back()->GetComponent<UStaticMeshComponent>()};

        Passed = Check(Restored != nullptr && Restored->GetMaterial(1) == Red && Restored->GetMaterialOverride(FName{"Red"}) == FAssetHandle{}, "Reload retains only explicit slot overrides") && Passed;
        World.CleanupWorld();
        std::filesystem::create_directories("Content/Legacy.mtl.materials");
        std::filesystem::copy_file("Content/Imported/Mesh.materials/M_Red.material", "Content/Legacy.mtl.materials/M_Red.material");
        std::filesystem::copy_file("Content/Imported/Mesh.materials/M_Blue.material", "Content/Legacy.mtl.materials/M_Blue.material");
        std::filesystem::rename(Source, Source.parent_path() / "SourceRemoved");
        Assets.Reset();
        Passed = Check(Assets.Initialize(), "Runtime assets reload with source OBJ, MTL and textures unavailable") && Passed;
        const UMesh* Reloaded{Assets.ResolveAsset<UMesh>(Assets.FindAsset(FAssetPath{"/Game/Imported/Mesh.bin"}))};

        Passed = Check(Reloaded != nullptr && CheckMesh(*Reloaded) && Assets.ResolveAsset<UMaterial>(Reloaded->GetDefaultMaterial(0)) != nullptr && Assets.ResolveAsset<UMaterial>(Reloaded->GetDefaultMaterial(1)) != nullptr, "Saved mesh slots resolve independent material references after registry recreation") && Passed;
        const USurfaceOpaque* ReloadedBlue{Reloaded != nullptr ? Assets.ResolveAsset<USurfaceOpaque>(Reloaded->GetDefaultMaterial(1)) : nullptr};

        Passed = Check(ReloadedBlue != nullptr && Assets.ResolveAsset<UTexture>(ReloadedBlue->GetParameters().mDiffuseTexture.GetTexture()) != nullptr, "Runtime material textures resolve after the transient Viewer assets and original source files are gone") && Passed;
        FMaterialGPUSlot ReloadedMaterial{};

        if (ReloadedBlue != nullptr) {
            ReloadedBlue->BuildGPUData(ReloadedMaterial);
        }

        Passed = Check(ReloadedBlue != nullptr && ReloadedBlue->GetShader() == EMaterialShader::Alternate && std::memcmp(ExpectedMaterial.mData.data(), ReloadedMaterial.mData.data(), MaterialGpuStride) == 0, "Saved shader choices and material parameters produce identical GPU data after reload") && Passed;
        Resources.Prune();
        Resources.GetMaterialBuffer().Synchronize(Context.Get());
        Passed = Check(RenderPixels(Device.Get(), Context.Get(), Resources, Assets.FindAsset(FAssetPath{"/Game/Imported/Mesh.bin"}), 0, true) == OverridePixels, "Runtime assets render identical override pixels after registry recreation") && Passed;
        UWorld LegacyWorld{};

        LegacyWorld.Initialize();
        AActor* LegacyActor{LegacyWorld.SpawnStaticMeshActor(Assets.FindAsset(FAssetPath{"/Game/Imported/Mesh.bin"}), FVector3{})};
        UStaticMeshComponent* LegacyComponent{LegacyActor->GetComponent<UStaticMeshComponent>()};
        const FAssetHandle LegacyBlue{Assets.FindAsset(FAssetPath{"/Game/Legacy.mtl.materials/M_Blue.material"})};
        rapidjson::Document Legacy{rapidjson::kObjectType};
        FArchiveJson LegacySave{Legacy, Legacy.GetAllocator()};
        FGuid LegacyGuid{*Assets.GetAssetGuid(LegacyBlue)};
        FString LibraryPath{"/Game/Legacy.mtl"};
        FString PipelinePath{"/Game/Pipeline/AlternateSolid"};

        LegacySave.Serialize("MaterialAssetGuid", LegacyGuid);
        LegacySave.Serialize("MaterialAssetPath", LibraryPath);
        LegacySave.Serialize("PipelineAssetPath", PipelinePath);
        FArchiveJson LegacyLoad{Legacy};

        LegacyLoad.SetAssetResolver(&Assets);
        LegacyComponent->Load(LegacyLoad);
        const USurfaceOpaque* LegacyRedMaterial{Assets.ResolveAsset<USurfaceOpaque>(LegacyComponent->GetMaterial(0))};
        const USurfaceOpaque* LegacyBlueMaterial{Assets.ResolveAsset<USurfaceOpaque>(LegacyComponent->GetMaterial(1))};

        Passed = Check(!LegacyLoad.HasError() && LegacyRedMaterial != nullptr && LegacyBlueMaterial != nullptr && LegacyRedMaterial->GetShader() == EMaterialShader::AlternateSolid && LegacyBlueMaterial->GetShader() == EMaterialShader::AlternateSolid && LegacyRedMaterial->GetParameters().mDiffuse.mX == 1.0f && LegacyBlueMaterial->GetParameters().mDiffuse.mZ == 1.0f, "Legacy group references and pipeline selections convert to named independent materials") && Passed;
        LegacyWorld.CleanupWorld();
        return Passed;
    }
}

int main(int ArgumentCount, char** Arguments) {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (ArgumentCount > 1 && std::string_view{Arguments[1]} == "--convert") {
        const bool Passed{Convert()};

        CoUninitialize();
        return Passed ? 0 : 1;
    }

    const std::filesystem::path Repository{std::filesystem::current_path()};
    const std::filesystem::path Root{Repository / "Build/Diagnostics/MeshMaterialFixtures" / FGuid::NewGuid().ToString().c_str()};

    std::filesystem::create_directories(Root / "Content");

    for (const char* Directory : {"System", "Shader", "Pipeline", "Font", "Texture"}) {
        std::filesystem::copy(Repository / "Content" / Directory, Root / "Content" / Directory, std::filesystem::copy_options::recursive);
    }

    std::filesystem::copy(Repository / "scenes", Root / "scenes", std::filesystem::copy_options::recursive);

    std::filesystem::current_path(Root);
    const bool Passed{Run()};

    std::filesystem::current_path(Repository);
    CoUninitialize();
    return Passed ? 0 : 1;
}
