#include "Engine/FWorldContext.h"
#include "Engine/Engine.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorViewportClient.h"
#include "Editor/FEditorViewport.h"
#include "Editor/Panel/FPlaceActorsPanel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/AAmbientLightActor.h"
#include "Engine/ADirectionalLightActor.h"
#include "Engine/APointLightActor.h"
#include "Engine/ASpotLightActor.h"
#include "Engine/Component/UPointLightComponent.h"
#include "Engine/Component/USpotLightComponent.h"
#include "Engine/SceneSerializer.h"
#include "Engine/FWorldCloner.h"
#include "Engine/FAssetManager.h"
#include "Engine/UMaterial.h"
#include "Engine/UTexture2D.h"
#include "Engine/PrimitiveMesh.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/FObjectDuplicationContext.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Core/Base/ErrorHandler.h"
#include "Renderer/Renderer.h"
#include "FScene.h"
#include "FTiledLightGrid.h"
#include "MeshImportTests.h"
#include "ShadowTests.h"
#include "StatsTests.h"
#include "Core/Stats.h"

#include <Windows.h>
#include <d3d11sdklayers.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

void Require(bool Condition, const char* Message) {
    if (!Condition) {
        throw std::runtime_error{Message};
    }
}

void TestLightLifecycle() {
    FWorldContext Context{};
    UWorld* World{Context.GetWorld()};
    auto Scene{std::make_unique<FScene>(nullptr, nullptr)};
    FScene* ScenePointer{Scene.get()};
    Require(World->AttachScene(std::move(Scene)), "Attach scene failed");
    Require(World->SpawnActor(AAmbientLightActor::StaticClass()) != nullptr, "Ambient spawn failed");
    Require(World->SpawnActor(ADirectionalLightActor::StaticClass()) != nullptr, "Directional spawn failed");
    auto* Point{Cast<APointLightActor>(World->SpawnActor(APointLightActor::StaticClass()))};
    auto* Spot{Cast<ASpotLightActor>(World->SpawnActor(ASpotLightActor::StaticClass()))};
    Require(Point != nullptr && Spot != nullptr && ScenePointer->GetLights().size() == 4, "Light registration failed");
    Require(Point->GetLightComponent()->SetIntensity(3.0f), "Intensity edit failed");
    Require(Point->GetLightComponent()->SetLightColor({0.2f, 0.4f, 1.0f, 1.0f}), "Color edit failed");
    Require(Point->GetLightComponent()->SetAttenuationRadius(7.0f), "Radius edit failed");
    Require(Point->SetActorTransform({{2.0f, 3.0f, 4.0f}, {}, {1.0f, 1.0f, 1.0f}}), "Transform edit failed");
    Require(!Point->GetLightComponent()->SetIntensity(-1.0f) && !Point->GetLightComponent()->SetAttenuationRadius(0.0f), "Invalid light values accepted");
    const auto Found{std::find_if(ScenePointer->GetLights().begin(), ScenePointer->GetLights().end(), [Point](const FLightSceneProxy& Proxy) {
        return Proxy.mId == Point->GetLightComponent()->GetGuid();
    })};
    Require(Found != ScenePointer->GetLights().end() && Found->mPosition == FVector{2.0f, 3.0f, 4.0f} && Found->mIntensity == 3.0f && Found->mAttenuationRadius == 7.0f, "Edited light did not reach renderer");
    Require(Spot->GetLightComponent()->SetInnerConeAngle(50.0f) && Spot->GetLightComponent()->SetOuterConeAngle(65.0f), "Cone edits failed");
    FString Error{};
    Require(FSceneSerializer::Save("Intermediate/Week5Test.scene.masset", &Context, Error), Error.c_str());
    auto Restored{FSceneSerializer::Load("Intermediate/Week5Test.scene.masset", Error)};
    Require(Restored != nullptr, Error.c_str());
    bool RestoredSpot{};
    for (const auto& Actor : Restored->GetWorld()->GetPersistentLevel()->GetActors()) {
        if (const auto* RestoredLight{Cast<ASpotLightActor>(Actor.get())}) {
            RestoredSpot = RestoredLight->GetLightComponent()->GetInnerConeAngle() == 50.0f && RestoredLight->GetLightComponent()->GetOuterConeAngle() == 65.0f;
        }
    }
    Require(RestoredSpot, "Cone angles did not survive scene save/load");
    FObjectDuplicationContext Duplication{};
    auto Cloned{FWorldCloner::Clone(&Context, EWorldType::PIE, &Duplication, Error)};
    Require(Cloned != nullptr && Cloned->GetWorld()->GetPersistentLevel()->GetActors().size() == 4, "PIE light cloning failed");
    AActor* Repeated{World->SpawnActor(APointLightActor::StaticClass())};
    Require(Repeated != nullptr && Repeated->GetName() != Point->GetName(), "Repeated light spawn failed");
    Require(World->DestroyActor(Repeated->GetHandle()) && ScenePointer->GetLights().size() == 4, "Light destruction left a proxy");
    while (!World->GetPersistentLevel()->GetActors().empty()) {
        World->DestroyActor(World->GetPersistentLevel()->GetActors().front()->GetHandle());
    }
    Require(ScenePointer->GetLights().empty(), "Actor destruction left light proxies");
    World->Shutdown();
}

void TestTileCoverage() {
    FSceneView View{};
    View.mSize = {257.0f, 193.0f};
    View.mView = FMatrix::LookTo({}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
    FTiledLightGrid Grid{};
    for (const bool Orthographic : {false, true}) {
        View.mProjection = Orthographic ? FMatrix::Orthographic(12.0f, 9.0f, 0.1f, 100.0f) : FMatrix::Perspective(60.0f, View.mSize.x / View.mSize.y, 0.1f, 100.0f);
        View.mViewProjection = View.mView * View.mProjection;
        TArray<FLightSceneProxy> Lights{};
        for (Uint32 Index{}; Index < 40; ++Index) {
            FLightSceneProxy Light{};
            Light.mType = Index % 2 == 0 ? ELightType::Point : ELightType::Spot;
            Light.mPosition = {0.2f + Index * 0.25f, static_cast<float>(static_cast<Int32>(Index % 9) - 4), static_cast<float>(static_cast<Int32>(Index % 7) - 3)};
            Light.mAttenuationRadius = 0.5f + (Index % 5) * 0.3f;
            Lights.push_back(Light);
        }
        Grid.Build(Lights, View);
        Require(Grid.GetColumns() == 17 && Grid.GetRows() == 13, "Partial edge tiles were lost");
        for (Uint32 Index{}; Index < Lights.size(); ++Index) {
            for (Int32 X{-2}; X <= 2; ++X) {
                for (Int32 Y{-2}; Y <= 2; ++Y) {
                    for (Int32 Z{-2}; Z <= 2; ++Z) {
                        const FVector Offset{X * 0.25f, Y * 0.25f, Z * 0.25f};
                        const FVector4 Clip{FVector4{Lights[Index].mPosition + Offset * Lights[Index].mAttenuationRadius, 1.0f} * View.mViewProjection};
                        if (Clip.w <= 0.0f || Clip.z < 0.0f || Clip.z > Clip.w || std::abs(Clip.x) >= Clip.w || std::abs(Clip.y) >= Clip.w) {
                            continue;
                        }
                        const Uint32 TileX{static_cast<Uint32>((Clip.x / Clip.w * 0.5f + 0.5f) * View.mSize.x) / 16};
                        const Uint32 TileY{static_cast<Uint32>((0.5f - Clip.y / Clip.w * 0.5f) * View.mSize.y) / 16};
                        const auto Range{Grid.GetRanges()[TileY * Grid.GetColumns() + TileX]};
                        const auto Begin{Grid.GetIndices().begin() + Range[0]};
                        Require(std::find(Begin, Begin + Range[1], Index) != Begin + Range[1], "Culling lost a light affecting a visible sample");
                    }
                }
            }
        }
    }
    TArray<FLightSceneProxy> Many{};
    Many.resize(300);
    Grid.Build(Many, View);
    Require(Grid.GetLights().size() == 300 && Grid.GetRanges().front()[1] == 300, "Tile light list truncated");
    for (auto& Light : Many) {
        Light.mVisible = false;
    }
    Grid.Build(Many, View);
    Require(Grid.GetLights().empty() && Grid.GetIndices().empty(), "Hidden lights entered shading lists");
}

std::array<Uint8, 4> ReadCenter(FRenderer& Renderer, FRenderTarget& Target) {
    auto* ResourceView{static_cast<ID3D11ShaderResourceView*>(Target.GetNativeTexture())};
    Microsoft::WRL::ComPtr<ID3D11Resource> Resource{};
    ResourceView->GetResource(Resource.GetAddressOf());
    Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture{};
    Require(SUCCEEDED(Resource.As(&Texture)), "Cannot read render target texture");
    D3D11_TEXTURE2D_DESC Description{};
    Texture->GetDesc(&Description);
    Description.Usage = D3D11_USAGE_STAGING;
    Description.BindFlags = 0;
    Description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    Description.MiscFlags = 0;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> Staging{};
    auto* Device{static_cast<ID3D11Device*>(Renderer.GetNativeDevice())};
    auto* Context{static_cast<ID3D11DeviceContext*>(Renderer.GetNativeContext())};
    Require(SUCCEEDED(Device->CreateTexture2D(&Description, nullptr, Staging.GetAddressOf())), "Cannot create readback texture");
    Context->CopyResource(Staging.Get(), Texture.Get());
    D3D11_MAPPED_SUBRESOURCE Data{};
    Require(SUCCEEDED(Context->Map(Staging.Get(), 0, D3D11_MAP_READ, 0, &Data)), "Cannot map readback texture");
    const auto* Pixel{static_cast<const Uint8*>(Data.pData) + Data.RowPitch * (Description.Height / 2) + 4 * (Description.Width / 2)};
    const std::array<Uint8, 4> Color{Pixel[0], Pixel[1], Pixel[2], Pixel[3]};
    Context->Unmap(Staging.Get(), 0);
    return Color;
}

void TestEditorPlacement(FRenderer& Renderer) {
    UEngine Engine{};
    Require(Engine.Initialize(&Renderer), "Placement engine initialization failed");
    FEditorContext Context{&Engine};
    Require(Context.SetWorld(Engine.GetWorldHandle()), "Placement context initialization failed");
    FEditorViewportClient Client{&Context};
    for (const EEditorViewportType Type : {EEditorViewportType::Perspective, EEditorViewportType::Top}) {
        Client.SetViewType(Type);
        for (const char* Preset : {"Cube", "Sphere", "Plane", "Text", "Billboard", "ACameraActor", "AAmbientLightActor", "ADirectionalLightActor", "APointLightActor", "ASpotLightActor"}) {
            const std::size_t Before{Engine.GetWorld()->GetPersistentLevel()->GetActors().size()};
            Require(Client.DropActor(Preset, {320.0f, 240.0f}, {640.0f, 480.0f}), "Actor viewport drop failed");
            Require(Engine.GetWorld()->GetPersistentLevel()->GetActors().size() == Before + 1 && Context.IsSceneDirty() && Context.GetSelectedActor() != nullptr && Context.GetSelectedActor()->GetRootComponent() != nullptr, "Drop did not update the world, selection and scene state");
        }
    }
    Require(!Client.DropActor("MissingActor", {320.0f, 240.0f}, {640.0f, 480.0f}), "Unknown actor preset accepted");
    ImGui::CreateContext();
    ImGuiIO& Input{ImGui::GetIO()};
    Input.IniFilename = nullptr;
    Input.DisplaySize = {1024.0f, 768.0f};
    Input.DeltaTime = 1.0f / 60.0f;
    Input.Fonts->AddFontDefault();
    Input.Fonts->Build();
    FPlaceActorsPanel Panel{&Context};
    FEditorViewport Viewport{&Context};
    Require(Viewport.Initialize(&Renderer), "Drag target viewport initialization failed");
    const auto Frame{[&]() {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0.0f, 0.0f}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({240.0f, 750.0f}, ImGuiCond_Always);
        Panel.Draw(1.0f / 60.0f);
        ImGui::SetNextWindowPos({250.0f, 0.0f}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({770.0f, 750.0f}, ImGuiCond_Always);
        Viewport.DrawUI(1.0f / 60.0f);
        ImGui::Render();
    }};
    Frame();
    const ImGuiID CubeId{ImHashStr("Cube", 0, ImHashStr("Cube", 0, ImHashStr("Place Actors")))};
    ImVec2 Source{};
    bool FoundSource{};
    for (Int32 Y{50}; Y < 500 && !FoundSource; Y += 4) {
        Input.AddMousePosEvent(100.0f, static_cast<float>(Y));
        Frame();
        if (ImGui::GetCurrentContext()->HoveredId == CubeId) {
            Source = {100.0f, static_cast<float>(Y)};
            FoundSource = true;
        }
    }
    Require(FoundSource, "Cannot find the actual Place Actors drag source");
    const std::size_t BeforeDrag{Engine.GetWorld()->GetPersistentLevel()->GetActors().size()};
    Input.AddMouseButtonEvent(0, true);
    Frame();
    Input.AddMousePosEvent(Source.x + 12.0f, Source.y);
    Frame();
    Require(ImGui::GetDragDropPayload() != nullptr, "Place Actors did not start a drag payload");
    Input.AddMousePosEvent(650.0f, 400.0f);
    Frame();
    Input.AddMouseButtonEvent(0, false);
    Frame();
    Frame();
    Require(Engine.GetWorld()->GetPersistentLevel()->GetActors().size() == BeforeDrag + 1, "Place Actors payload was not delivered to the viewport");
    Viewport.Shutdown();
    ImGui::DestroyContext();
    Engine.Shutdown();
}

void TestTearingCommands(FRenderer& Renderer) {
    FEditorContext Detached{nullptr};
    Detached.ExecuteConsoleCommand("Allow Tearing");
    Require(Detached.GetLogs().back() == "Allow Tearing: renderer is unavailable.", "Tearing command did not handle a missing engine");

    UEngine Engine{};
    Require(Engine.Initialize(&Renderer), "Tearing command engine initialization failed");
    FEditorContext Context{&Engine};
    Require(!Renderer.IsTearingAllowed(), "Tearing must be opt-in");
    Context.ExecuteConsoleCommand("Allow Tearing");
    Require(Renderer.IsTearingAllowed() == Renderer.IsTearingSupported(), "Tearing enable did not respect device support");
    Require(Context.GetLogs().back() == (Renderer.IsTearingSupported() ? "Allow Tearing: on (VSync off in windowed mode)." : "Allow Tearing: unsupported or renderer is not initialized."), "Tearing enable result was not reported");
    Context.ExecuteConsoleCommand("Allow Tearing status");
    Require(Renderer.IsTearingAllowed() == Renderer.IsTearingSupported(), "Tearing status changed the setting");
    for (const char* Command : {"Allow", "Allow Other", "Allow Tearing maybe", "Allow Tearing off extra"}) {
        Context.ExecuteConsoleCommand(Command);
        Require(Context.GetLogs().back() == "Usage: Allow Tearing [on/off/status]" && Renderer.IsTearingAllowed() == Renderer.IsTearingSupported(), "Invalid tearing command changed the setting");
    }
    for (const bool Allow : {false, true, false, true}) {
        Context.ExecuteConsoleCommand(Allow ? "  aLLoW   TeArInG  ON  " : "Allow Tearing off");
        Require(Renderer.IsTearingAllowed() == (Allow && Renderer.IsTearingSupported()), "Tearing command did not apply the requested state");
        Renderer.BeginFrame(Allow ? 320u : 256u, Allow ? 192u : 256u);
        Renderer.BeginUiRender();
        Renderer.Present();
    }
    Context.ExecuteConsoleCommand("Allow Tearing off");
    Require(!Renderer.IsTearingAllowed() && Context.GetLogs().back() == "Allow Tearing: off (VSync on).", "Tearing disable did not restore VSync");
    Renderer.BeginFrame(256, 256);
    Engine.Shutdown();
}

void TestGpuRendering() {
    const HWND Window{CreateWindowExW(0, L"STATIC", L"Week5 integration tests", WS_OVERLAPPEDWINDOW, 0, 0, 256, 256, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr)};
    Require(Window != nullptr, "Cannot create hidden test window");
    FRenderer Renderer{};
    Require(!Renderer.IsTearingSupported() && !Renderer.IsTearingAllowed() && !Renderer.SetAllowTearing(true), "Uninitialized renderer accepted tearing");
    Renderer.Present();
    Renderer.SetProjectPaths(std::filesystem::absolute("Templates/Project/Content"), std::filesystem::absolute("Intermediate/Week5ShaderCache"));
    Require(Renderer.Initialize(Window, 256, 256), "Renderer initialization failed");
    TestTearingCommands(Renderer);
    auto Scene{Renderer.AllocateScene()};
    auto Target{Renderer.CreateRenderTarget(64, 64)};
    auto Mesh{NewObject<UStaticMesh>()};
    Require(Mesh->SetMeshData(GetPrimitiveMesh(EPrimitiveType::Plane)), "Plane creation failed");
    auto Material{NewObject<UMaterial>()};
    Material->SetTwoSided(true);
    Material->SetSpecularStrength(0.0f);
    FPrimitiveSceneProxy Primitive{};
    Primitive.mMesh = Mesh.get();
    Primitive.mMaterial = Material.get();
    Primitive.mBounds = Mesh->GetBounds();
    Scene->AddPrimitive(Primitive);
    FSceneView View{};
    View.mSize = {64.0f, 64.0f};
    View.mLocation = {0.0f, 0.0f, 2.0f};
    View.mForward = {0.0f, 0.0f, -1.0f};
    View.mView = FMatrix::LookTo(View.mLocation, View.mForward, {0.0f, 1.0f, 0.0f});
    View.mProjection = FMatrix::Orthographic(1.0f, 1.0f, 0.1f, 10.0f);
    View.mViewProjection = View.mView * View.mProjection;
    const auto Render{[&](EViewMode Mode) {
        const FScopedStatFrame StatFrame{};
        Renderer.BeginRenderingViewFamily({Scene.get(), Target.get(), View, nullptr, Mode});
        return ReadCenter(Renderer, *Target);
    }};
    const auto Dark{Render(EViewMode::Lit_Lambert)};
    const auto FirstRenderStats{FStats::Get().GetSnapshots("render")};
    Require(std::any_of(FirstRenderStats.begin(), FirstRenderStats.end(), [](const FStatSnapshot& Stat) {
        return Stat.GetName() == "Rendered views" && Stat.GetValue() == 1.0;
    }), "Rendered view stat was not published");
    Require(std::any_of(FirstRenderStats.begin(), FirstRenderStats.end(), [](const FStatSnapshot& Stat) {
        return Stat.GetName() == "Scene/editor triangles" && Stat.GetValue() == 2.0;
    }), "Submitted triangle stat was not published");
    Require(Dark[0] < 3 && Dark[1] < 3 && Dark[2] < 3, "Scene without lights was not dark");
    FLightSceneProxy Ambient{};
    Ambient.mColor = {1.0f, 0.0f, 0.0f, 1.0f};
    Ambient.mIntensity = 0.2f;
    Scene->AddLight(Ambient);
    FLightSceneProxy Directional{};
    Directional.mType = ELightType::Directional;
    Directional.mDirection = {0.0f, 0.0f, -1.0f};
    Directional.mColor = {0.0f, 1.0f, 0.0f, 1.0f};
    Directional.mIntensity = 0.5f;
    Scene->AddLight(Directional);
    for (const EViewMode Mode : {EViewMode::Lit_Gouraud, EViewMode::Lit_Lambert, EViewMode::Lit_Phong}) {
        Require(Renderer.GetMaterialPipeline(*Material, Mode) != nullptr, "Lighting permutation compilation failed");
        const auto Color{Render(Mode)};
        Require(std::abs(static_cast<Int32>(Color[0]) - 51) <= 2 && std::abs(static_cast<Int32>(Color[1]) - 128) <= 2, "Ambient/directional GPU lighting mismatch");
    }
    Scene->RemoveLight(Ambient.mId);
    Scene->RemoveLight(Directional.mId);
    FLightSceneProxy Local{};
    Local.mType = ELightType::Point;
    Local.mPosition = {0.0f, 0.0f, 1.0f};
    Local.mAttenuationRadius = 2.0f;
    Local.mColor = {0.0f, 0.0f, 1.0f, 1.0f};
    Scene->AddLight(Local);
    const auto Point{Render(EViewMode::Lit_Lambert)};
    Require(std::abs(static_cast<Int32>(Point[2]) - 64) <= 3, "Point falloff GPU mismatch");
    Local.mAttenuationRadius = 0.5f;
    Scene->AddLight(Local);
    Require(Render(EViewMode::Lit_Lambert)[2] < 3, "Point radius was ignored");
    Local.mType = ELightType::Spot;
    Local.mAttenuationRadius = 10.0f;
    Local.mDirection = {0.0f, 0.0f, -1.0f};
    Scene->AddLight(Local);
    Require(Render(EViewMode::Lit_Phong)[2] > 190, "Spot cone did not light its center");
    Local.mDirection = {0.0f, 0.0f, 1.0f};
    Scene->AddLight(Local);
    Require(Render(EViewMode::Lit_Phong)[2] < 3, "Spot illuminated outside its cone");
    Local.mVisible = false;
    Scene->AddLight(Local);
    Require(Render(EViewMode::Lit_Lambert)[2] < 3, "Visibility toggle did not reach GPU");
    Require(Render(EViewMode::Unlit)[0] > 250, "Unlit view was affected by lights");
    const auto Normal{Render(EViewMode::WorldNormal)};
    Require(std::abs(static_cast<Int32>(Normal[0]) - 128) <= 2 && Normal[2] > 250, "World normal output mismatch");
    std::filesystem::create_directories("Intermediate/Week5Assets");
    FAssetManager Assets{std::filesystem::absolute("Intermediate/Week5Assets")};
    auto NormalTexture{NewObject<UTexture2D>()};
    Require(NormalTexture->SetPixels(1, 1, ETextureFormat::RGBA8, {255, 128, 128, 255}), "Normal texture creation failed");
    FString Error{};
    Require(Assets.SaveTexture("Normal.masset", *NormalTexture, Error), Error.c_str());
    Material->SetNormalMapTexturePath("Normal.masset");
    Require(Material->ResolveTexture(Assets, Error), Error.c_str());
    Require(!Material->GetNormalMapTexture()->IsSRGB() && Assets.LoadTexture("Normal.masset", Error)->IsSRGB(), "Normal map changed shared color texture gamma");
    for (const EViewMode Mode : {EViewMode::Lit_Gouraud, EViewMode::Lit_Lambert, EViewMode::Lit_Phong, EViewMode::Unlit, EViewMode::WorldNormal}) {
        Require(Renderer.GetMaterialPipeline(*Material, Mode) != nullptr, "Normal map permutation failed");
    }
    const auto Mapped{Render(EViewMode::WorldNormal)};
    Require(Mapped[0] > 250 && std::abs(static_cast<Int32>(Mapped[2]) - 128) <= 3, "Normal texture did not affect world normal output");
    Material->SetName("Normal material");
    Require(Assets.SaveMaterial("Normal.mtl", *Material, Error), Error.c_str());
    UMaterial* Restored{Assets.LoadMaterial("Normal.mtl", Error)};
    Require(Restored != nullptr && Restored->GetNormalMapTexturePath() == "Normal.masset", "Normal map material save/load failed");
    Microsoft::WRL::ComPtr<ID3D11InfoQueue> Queue{};
    {
        const FScopedStatFrame StatFrame{};
        Renderer.BeginFrame(256, 256);
        Renderer.BeginRenderingViewFamily({Scene.get(), Target.get(), View, nullptr, EViewMode::Unlit});
        Renderer.BeginRenderingViewFamily({Scene.get(), Target.get(), View, nullptr, EViewMode::Unlit});
    }
    const auto MultiViewStats{FStats::Get().GetSnapshots("render")};
    Require(std::any_of(MultiViewStats.begin(), MultiViewStats.end(), [](const FStatSnapshot& Stat) {
        return Stat.GetName() == "Rendered views" && Stat.GetValue() == 2.0;
    }), "Multiple views did not accumulate in one stat frame");
    Require(std::any_of(MultiViewStats.begin(), MultiViewStats.end(), [](const FStatSnapshot& Stat) {
        return Stat.GetName() == "Scene/editor triangles" && Stat.GetValue() == 4.0;
    }), "Multiple views did not accumulate triangle counts");
    std::cout << "Render view, triangle and multiple-view stat accumulation passed\n";
    TestShadowRendering(Renderer);
    if (SUCCEEDED(static_cast<ID3D11Device*>(Renderer.GetNativeDevice())->QueryInterface(IID_PPV_ARGS(Queue.GetAddressOf())))) {
        for (UINT64 Index{}; Index < Queue->GetNumStoredMessages(); ++Index) {
            SIZE_T Size{};
            Queue->GetMessage(Index, nullptr, &Size);
            TArray<Uint8> Bytes{};
            Bytes.resize(Size);
            auto* Message{reinterpret_cast<D3D11_MESSAGE*>(Bytes.data())};
            Queue->GetMessage(Index, Message, &Size);
            if (Message->Severity <= D3D11_MESSAGE_SEVERITY_WARNING) {
                std::cerr << Message->pDescription << '\n';
                Require(false, "D3D11 validation warning during lighting rendering");
            }
        }
    }
    TestEditorPlacement(Renderer);
    Scene.reset();
    Target.reset();
    Renderer.Shutdown();
    Require(!Renderer.IsTearingSupported() && !Renderer.IsTearingAllowed() && !Renderer.SetAllowTearing(true), "Renderer shutdown retained tearing state");
    Require(Renderer.InitializeUi(Window, 256, 256), "Renderer reinitialization failed");
    Require(!Renderer.IsTearingAllowed() && Renderer.SetAllowTearing(true) == Renderer.IsTearingSupported(), "Reinitialized renderer lost tearing support");
    Renderer.BeginFrame(320, 192);
    Renderer.BeginUiRender();
    Renderer.Present();
    Renderer.Shutdown();
    DestroyWindow(Window);
}

int main() {
    ErrorHandler::Initialize();
    const HRESULT Com{CoInitializeEx(nullptr, COINIT_MULTITHREADED)};
    try {
        TestStats();
        std::cout << "Stat aggregation, history, concurrent writers, timers, engine counters, commands and UI passed\n";
        TestMeshImport();
        std::cout << "Mesh import, material textures, Unicode paths and source diagnostics passed\n";
        TestLightLifecycle();
        std::cout << "Light lifecycle, serialization and PIE clone passed\n";
        TestTileCoverage();
        TestShadowAllocation();
        std::cout << "Shadow quadtree allocation, CSM stability, Unreal icons and shadow property serialization passed\n";
        std::cout << "Perspective/orthographic tile coverage and 300-light lists passed\n";
        TestGpuRendering();
        std::cout << "Tearing commands, presentation, resize, reinitialization, GPU lighting, shader permutations, normal mapping, material roundtrip and actor drag/drop passed\n";
    } catch (const std::exception& Error) {
        std::cerr << Error.what() << '\n';
        return 1;
    }
    if (SUCCEEDED(Com)) {
        CoUninitialize();
    }
    return 0;
}
