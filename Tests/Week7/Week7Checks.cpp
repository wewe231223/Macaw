/**
 * @file Week7Checks.cpp
 * @brief PIE 전용 이동과 후처리 수식·셰이더·렌더 타깃·장면 등록을 통합 검증합니다.
 */

#include "Engine/AExponentionFogActor.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/Component/UHeightFogComponent.h"
#include "Engine/Component/URotationMovementComponent.h"
#include "Engine/Component/UProjectileMovementComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/FWorldContext.h"
#include "Engine/FWorldCloner.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/PrimitiveMesh.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/UTexture2D.h"
#include "Engine/EPrimitiveType.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/FObjectDuplicationContext.h"
#include "Core/Math/FRotator.h"
#include "Core/TDelegate.h"
#include "CoreUObject/Reflection/UScriptStruct.h"
#include "Engine/Reflection/RegisterEngine.h"
#include "RenderTarget.h"
#include "FPostProcessResources.h"
#include "FScene.h"
#include "Renderer/Renderer.h"

#include <Windows.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace Tests {

    /**
     * @brief 통합 검증 조건이 거짓이면 설명 메시지로 예외를 발생시킵니다.
     * @param[in] Condition 참이어야 하는 검증 조건입니다.
     * @param[in] Message 검증이 실패했을 때 예외에 포함할 설명입니다.
     */
    void Check(bool Condition, const char* Message) {
        if (!Condition) {
            throw std::runtime_error{Message};
        }
    }

    /**
     * @brief 실제 값과 기대값의 차이가 허용 오차 이내인지 검증합니다.
     * @param[in] Actual 실제 계산 또는 읽기 결과입니다.
     * @param[in] Expected 비교할 기대값입니다.
     * @param[in] Tolerance 실제 값과 기대값에 허용하는 최대 절대 오차입니다.
     * @param[in] Message 검증이 실패했을 때 예외에 포함할 설명입니다.
     */
    void CheckNear(float Actual, float Expected, float Tolerance, const char* Message) {
        Check(std::abs(Actual - Expected) <= Tolerance, Message);
    }

    /**
     * @brief 모듈 네임스페이스 적용 후 리플렉션 이름·모듈 간 상속·열거형 메타데이터와 델리게이트 호출을 검증합니다.
     */
    void CheckNamespaces() {
        Engine::ReflectionModules::RegisterEngine();
        Check(Core::FVector::StaticStruct()->GetName() == "FVector", "Preserve serialized struct names");
        Check(CoreUObject::FReflectionRegistry::FindClass("AActor") == Engine::AActor::StaticClass(), "Preserve serialized class names");
        Check(Engine::AActor::StaticClass()->GetSuperClass() == CoreUObject::UObject::StaticClass(), "Cross-module class inheritance");
        const CoreUObject::FProperty* WorldType{Engine::UWorld::StaticClass()->FindProperty("WorldType")};
        Check(WorldType != nullptr && WorldType->GetEnumValues().size() == 4, "Namespaced enum property reflection");
        Core::TDelegate<void()> Delegate{};
        Core::Uint32 Calls{};
        const Core::Uint32 Handle{Delegate.Add([&Calls]() {
            ++Calls;
        })};
        Delegate.Broadcast();
        Delegate.RemoveHandler(Handle);
        Delegate.Broadcast();
        Check(Calls == 1, "Namespaced delegate templates");
        std::cout << "PASS: module namespaces, stable reflection names, enum metadata, delegate templates\n";
    }

    /**
     * @brief Editor·Game 격리, PIE 회전·발사체 이동, 복제·일시정지·단계 실행을 검증합니다.
     */
    void CheckMovement() {
        Engine::FWorldContext Editor{Engine::EWorldType::Editor};
        Engine::UWorld* World{Editor.GetWorld()};
        Engine::AStaticMeshActor* Actor{World->SpawnPrimitive(Engine::EPrimitiveType::Cube)};
        auto Rotation{CoreUObject::NewObject<Engine::URotationMovementComponent>()};
        auto Projectile{CoreUObject::NewObject<Engine::UProjectileMovementComponent>()};
        Engine::URotationMovementComponent* RotationPointer{Rotation.get()};
        Engine::UProjectileMovementComponent* ProjectilePointer{Projectile.get()};
        Rotation->SetName("Rotation");
        Projectile->SetName("Projectile");
        Rotation->SetEditorTickable(true);
        Projectile->SetEditorTickable(true);
        Check(Rotation->SetRotationRate({0.0f, 90.0f, 0.0f}), "Rotation rate");
        Check(Projectile->SetInitialVelocity({2.0f, 0.0f, 0.0f}) && Projectile->SetGravityScale(0.0f) && Projectile->SetRotationFollowsVelocity(false), "Projectile settings");
        Check(Actor->AddComponent(std::move(Rotation)) && Actor->AddComponent(std::move(Projectile)), "Attach movement");
        const Core::FTransform Original{Actor->GetRootComponent()->GetTransform()};
        Check(World->Tick(1.0f), "Editor tick");
        RotationPointer->Tick(1.0f);
        ProjectilePointer->Tick(1.0f);
        Check(Actor->GetRootComponent()->GetTransform().Equals(Original), "Movement must not run in editor");
        Check(!RotationPointer->SetRotationRate({std::numeric_limits<float>::infinity(), 0.0f, 0.0f}), "Reject invalid rotation");
        Check(!ProjectilePointer->SetGravityScale(-1.0f) && !ProjectilePointer->SetInitialVelocity({std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f}), "Reject invalid projectile settings");
        Core::FString Error{};
        CoreUObject::FObjectDuplicationContext Duplication{};
        auto PIE{Engine::FWorldCloner::Clone(&Editor, Engine::EWorldType::PIE, &Duplication, Error)};
        Check(PIE != nullptr, Error.c_str());
        Engine::UWorld* PlayWorld{PIE->GetWorld()};
        Engine::AActor* Copy{PlayWorld->GetPersistentLevel()->GetActors().front().get()};
        Check(PlayWorld->BeginPlay() && PlayWorld->Tick(0.5f), "PIE tick");
        CheckNear(Copy->GetRootComponent()->GetTransform().mLocation.x, Original.mLocation.x + 1.0f, 0.001f, "Projectile displacement");
        CheckNear(Copy->GetRootComponent()->GetTransform().GetRotator().yaw, 45.0f, 0.01f, "Rotation degrees per second");
        const Core::FTransform BeforePause{Copy->GetRootComponent()->GetTransform()};
        Check(PlayWorld->SetPaused(true) && !PlayWorld->Tick(1.0f), "PIE pause");
        Check(Copy->GetRootComponent()->GetTransform().Equals(BeforePause), "Paused transform");
        Check(PlayWorld->Step(0.5f), "PIE step");
        CheckNear(Copy->GetRootComponent()->GetTransform().mLocation.x, Original.mLocation.x + 2.0f, 0.001f, "Single step displacement");
        CheckNear(Copy->GetRootComponent()->GetTransform().GetRotator().yaw, 90.0f, 0.01f, "Single step rotation");
        PlayWorld->EndPlay();
        Check(Actor->GetRootComponent()->GetTransform().Equals(Original), "PIE must preserve source transforms");
        auto Game{Engine::FWorldCloner::Clone(&Editor, Engine::EWorldType::Game, &Duplication, Error)};
        Check(Game != nullptr && Game->GetWorld()->BeginPlay() && Game->GetWorld()->Tick(1.0f), "Game world tick");
        Check(Game->GetWorld()->GetPersistentLevel()->GetActors().front()->GetRootComponent()->GetTransform().Equals(Original), "Movement must run only in PIE");
        std::cout << "PASS: editor isolation, PIE movement, pause, step, clone, Game isolation\n";
    }

    /**
     * @brief 컬러 텍스처를 staging 자원에 복사하여 행 패딩을 제거한 RGBA8 픽셀로 읽습니다.
     * @param[in] Device GPU 자원을 생성하거나 조회할 Direct3D 11 장치입니다.
     * @param[in] Context GPU 명령을 기록하고 읽기·쓰기 상태를 설정할 Direct3D 11 컨텍스트입니다.
     * @param[in] Texture RGBA8로 읽을 컬러 텍스처의 비소유 주소입니다.
     * @return 행 우선으로 연속 저장한 폭×높이×4 바이트의 픽셀 배열입니다.
     */
    Core::TArray<Core::Uint8> ReadTexture(ID3D11Device* Device, ID3D11DeviceContext* Context, Engine::UTexture2D* Texture) {
        D3D11_TEXTURE2D_DESC Description{};
        Texture->GetTexture()->GetDesc(&Description);
        Description.Usage = D3D11_USAGE_STAGING;
        Description.BindFlags = 0;
        Description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        Description.MiscFlags = 0;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> Staging{};
        Check(SUCCEEDED(Device->CreateTexture2D(&Description, nullptr, Staging.GetAddressOf())), "Readback texture");
        Context->OMSetRenderTargets(0, nullptr, nullptr);
        Context->CopyResource(Staging.Get(), Texture->GetTexture());
        D3D11_MAPPED_SUBRESOURCE Mapping{};
        Check(SUCCEEDED(Context->Map(Staging.Get(), 0, D3D11_MAP_READ, 0, &Mapping)), "Readback map");
        Core::TArray<Core::Uint8> Pixels{};
        Pixels.resize(static_cast<std::size_t>(Description.Width) * Description.Height * 4);
        for (Core::Uint32 Row{}; Row < Description.Height; ++Row) {
            std::copy_n(static_cast<const Core::Uint8*>(Mapping.pData) + Row * Mapping.RowPitch, Description.Width * 4, Pixels.data() + Row * Description.Width * 4);
        }
        Context->Unmap(Staging.Get(), 0);
        return Pixels;
    }

    /**
     * @brief D3D11 디버그 큐를 읽고 경고 이상 메시지가 있으면 검증을 실패시킵니다.
     * @param[in] Device GPU 자원을 생성하거나 조회할 Direct3D 11 장치입니다.
     */
    void CheckDebugMessages(ID3D11Device* Device) {
        Microsoft::WRL::ComPtr<ID3D11InfoQueue> Queue{};
        if (FAILED(Device->QueryInterface(IID_PPV_ARGS(Queue.GetAddressOf())))) {
            return;
        }
        for (UINT64 Index{}; Index < Queue->GetNumStoredMessagesAllowedByRetrievalFilter(); ++Index) {
            SIZE_T Size{};
            Queue->GetMessage(Index, nullptr, &Size);
            Core::TArray<Core::Uint8> Storage{};
            Storage.resize(Size);
            auto* Message{reinterpret_cast<D3D11_MESSAGE*>(Storage.data())};
            Queue->GetMessage(Index, Message, &Size);
            if (Message->Severity <= D3D11_MESSAGE_SEVERITY_WARNING) {
                std::cerr << Message->pDescription << '\n';
                Check(false, "D3D11 validation message");
            }
        }
    }

    /**
     * @brief WARP 장치로 셰이더를 실행해 안개 수식·거리·다중 합성·정사영·Depth·FXAA·리사이즈를 검증합니다.
     */
    void CheckPostProcess() {
        Engine::FWorldContext World{};
        Microsoft::WRL::ComPtr<ID3D11Device> Device{};
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> Context{};
        HRESULT Result{D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, Device.GetAddressOf(), nullptr, Context.GetAddressOf())};
        if (FAILED(Result)) {
            Result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, Device.GetAddressOf(), nullptr, Context.GetAddressOf());
        }
        Check(SUCCEEDED(Result), "D3D11 device");
        Microsoft::WRL::ComPtr<ID3DBlob> Vertex{};
        Microsoft::WRL::ComPtr<ID3DBlob> Pixel{};
        for (const auto& [Entry, Profile, Code] : {std::tuple{"VertexMain", "vs_5_0", Vertex.GetAddressOf()}, std::tuple{"PixelMain", "ps_5_0", Pixel.GetAddressOf()}}) {
            Microsoft::WRL::ComPtr<ID3DBlob> Diagnostics{};
            Result = D3DCompileFromFile(L"Templates/Project/Content/Shaders/PostProcess.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, Entry, Profile, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0, Code, Diagnostics.GetAddressOf());
            if (FAILED(Result) && Diagnostics != nullptr) {
                std::cerr << static_cast<const char*>(Diagnostics->GetBufferPointer());
            }
            Check(SUCCEEDED(Result), "Compile postprocess shader");
        }
        Renderer::FPostProcessResources Pass{};
        Check(Pass.Initialize(Device.Get()) && Pass.CreateShaders(Device.Get(), {static_cast<const Core::Uint8*>(Vertex->GetBufferPointer()), Vertex->GetBufferSize()}, {static_cast<const Core::Uint8*>(Pixel->GetBufferPointer()), Pixel->GetBufferSize()}), "Postprocess resources");
        Renderer::FDXRenderTarget Target{};
        Check(Target.Resize(Device.Get(), 64, 64), "Render target allocation");
        const D3D11_VIEWPORT Viewport{0.0f, 0.0f, 64.0f, 64.0f, 0.0f, 1.0f};
        Context->RSSetViewports(1, &Viewport);
        Engine::FSceneViewFamily Family{};
        Family.mView.mSize = {64.0f, 64.0f};
        Family.mView.mView = Core::FMatrix::LookTo({}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
        Family.mView.mProjection = Core::FMatrix::Perspective(90.0f, 1.0f, 0.1f, 100.0f);
        Family.mView.mViewProjection = Family.mView.mView * Family.mView.mProjection;
        Family.mShowFlags.mFXAA = false;
        constexpr float Color[]{0.2f, 0.3f, 0.4f, 1.0f};
        const Core::FVector4 Clip{Core::FVector4{10.0f, 0.0f, 0.0f, 1.0f} * Family.mView.mViewProjection};
        Context->ClearDepthStencilView(Target.GetDepthView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, Clip.z / Clip.w, 0);
        Context->ClearRenderTargetView(Target.GetSceneColor()->GetRenderTargetView(), Color);
        Check(Pass.Render(Context.Get(), Target, Family, {}) == 1, "Color pass");
        const auto Original{ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor())};
        constexpr std::size_t Center{(32 * 64 + 32) * 4};
        CheckNear(Original[Center], 51.0f, 1.0f, "Scene color identity");
        Engine::FHeightFogSceneProxy Fog{};
        Fog.mDensity = 0.1f;
        Fog.mHeightFalloff = 0.0f;
        Fog.mInscatteringColor = {1.0f, 0.0f, 0.0f, 1.0f};
        const std::span<const Engine::FHeightFogSceneProxy> Fogs{&Fog, 1};
        Check(Pass.Render(Context.Get(), Target, Family, Fogs) == 2, "Fog pass");
        auto Pixels{ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor())};
        CheckNear(Pixels[Center], (0.2f + 0.8f * (1.0f - std::exp(-1.0f))) * 255.0f, 2.0f, "Analytic constant density fog");

        const std::array<Engine::FHeightFogSceneProxy, 2> Layers{Fog, Fog};
        Pass.Render(Context.Get(), Target, Family, Layers);
        Pixels = ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor());
        CheckNear(Pixels[Center], (0.2f + 0.8f * (1.0f - std::exp(-2.0f))) * 255.0f, 2.0f, "Multiple fog layers");
        Context->ClearRenderTargetView(Target.GetSceneColor()->GetRenderTargetView(), Color);

        Family.mView.mOrthographic = true;
        Family.mView.mProjection = Core::FMatrix::Orthographic(10.0f, 10.0f, 0.1f, 100.0f);
        Family.mView.mViewProjection = Family.mView.mView * Family.mView.mProjection;
        const Core::FVector4 OrthoClip{Core::FVector4{10.0f, 0.0f, 0.0f, 1.0f} * Family.mView.mViewProjection};
        Context->ClearDepthStencilView(Target.GetDepthView(), D3D11_CLEAR_DEPTH, OrthoClip.z / OrthoClip.w, 0);
        Pass.Render(Context.Get(), Target, Family, Fogs);
        Pixels = ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor());
        CheckNear(Pixels[Center], (0.2f + 0.8f * (1.0f - std::exp(-1.0f))) * 255.0f, 2.0f, "Orthographic fog reconstruction");
        Check(Pixels[Center] == Pixels[0], "Orthographic fog parallel rays");
        Family.mView.mOrthographic = false;
        Family.mView.mProjection = Core::FMatrix::Perspective(90.0f, 1.0f, 0.1f, 100.0f);
        Family.mView.mViewProjection = Family.mView.mView * Family.mView.mProjection;
        Context->ClearDepthStencilView(Target.GetDepthView(), D3D11_CLEAR_DEPTH, Clip.z / Clip.w, 0);

        Family.mShowFlags.mFog = false;
        Pass.Render(Context.Get(), Target, Family, Fogs);
        Check(ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor()) == Original, "Fog show flag bypass");
        Family.mShowFlags.mFog = true;
        Fog.mStartDistance = 11.0f;
        Pass.Render(Context.Get(), Target, Family, Fogs);
        Pixels = ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor());
        Check(Pixels[Center] == Original[Center], "Fog start distance");
        Fog.mStartDistance = 0.0f;
        Fog.mEndDistance = 2.0f;
        Pass.Render(Context.Get(), Target, Family, Fogs);
        Pixels = ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor());
        CheckNear(Pixels[Center], (0.2f + 0.8f * (1.0f - std::exp(-0.2f))) * 255.0f, 2.0f, "Fog end distance");
        Fog.mEndDistance = 1000.0f;
        Fog.mCutoffDistance = 5.0f;
        Pass.Render(Context.Get(), Target, Family, Fogs);
        Check(ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor()) == Original, "Fog cutoff distance");
        Fog.mCutoffDistance = 0.0f;
        Fog.mMaxOpacity = 0.25f;
        Pass.Render(Context.Get(), Target, Family, Fogs);
        Pixels = ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor());
        CheckNear(Pixels[Center], 102.0f, 1.0f, "Fog maximum opacity");
        Fog.mMaxOpacity = 1.0f;
        Fog.mHeightFalloff = 0.2f;
        Fog.mPosition.z = -10.0f;
        Pass.Render(Context.Get(), Target, Family, Fogs);
        Pixels = ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor());
        Check(Pixels[Center] < 100, "Fog height attenuation");
        Family.mViewMode = Engine::EViewMode::SceneDepth;
        Pass.Render(Context.Get(), Target, Family, Fogs);
        Pixels = ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor());
        CheckNear(Pixels[Center], (1.0f - std::exp(-0.1f)) * 255.0f, 1.0f, "Linear scene depth visualization");
        Check(Pixels[Center] == Pixels[Center + 1] && Pixels[Center] == Pixels[Center + 2], "Depth grayscale");
        Family.mViewMode = Engine::EViewMode::WorldNormal;
        Pass.Render(Context.Get(), Target, Family, Fogs);
        Check(ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor()) == Original, "Normal visualization excludes fog");
        Family.mViewMode = Engine::EViewMode::Lit;
        Family.mShowFlags.mFog = false;
        Core::TArray<Core::Uint8> Edges{};
        Edges.resize(64 * 64 * 4, 255);
        for (Core::Uint32 Y{}; Y < 64; ++Y) {
            for (Core::Uint32 X{}; X < 64; ++X) {
                for (Core::Uint32 Channel{}; Channel < 3; ++Channel) {
                    Edges[(Y * 64 + X) * 4 + Channel] = X > Y ? 255 : 0;
                }
            }
        }
        Context->UpdateSubresource(Target.GetSceneColor()->GetTexture(), 0, nullptr, Edges.data(), 64 * 4, 0);
        Family.mShowFlags.mFXAA = true;
        Pass.Render(Context.Get(), Target, Family, {});
        const auto Antialiased{ReadTexture(Device.Get(), Context.Get(), Target.GetOutputColor())};
        Check(Antialiased != Edges && Antialiased[(60 * 64 + 2) * 4] == 0 && Antialiased[(2 * 64 + 60) * 4] == 255, "FXAA smooths edges and preserves flat areas");
        Check(Target.Resize(Device.Get(), 31, 47) && Target.GetWidth() == 31 && Target.GetHeight() == 47 && Target.GetSceneDepth()->GetShaderResourceView() != nullptr, "Resize all postprocess resources");
        CheckDebugMessages(Device.Get());
        std::cout << "PASS: shader compilation, color, analytic fog, show flag, distances, opacity, height, depth, FXAA, resize, D3D11 validation\n";
    }

    /**
     * @brief 실제 렌더러에서 버퍼 모드·언리얼 아이콘·안개 복제 등록·삭제·리사이즈를 통합 검증합니다.
     */
    void CheckRenderer() {
        const HWND Window{CreateWindowExW(0, L"STATIC", L"Week7Checks", WS_OVERLAPPEDWINDOW, 0, 0, 128, 128, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr)};
        Check(Window != nullptr, "Hidden renderer window");
        Renderer::FRenderer Renderer{};
        Renderer.SetProjectPaths(std::filesystem::absolute("Templates/Project/Content"), std::filesystem::absolute("Intermediate/Week7/ShaderCache"));
        Check(Renderer.Initialize(Window, 128, 128), "Full renderer initialize");
        Engine::FWorldContext World{};
        Check(World.GetWorld()->AttachScene(Renderer.AllocateScene()), "Attach renderer scene");
        for (Core::Uint32 Index{}; Index < static_cast<Core::Uint32>(Engine::EPrimitiveType::Count); ++Index) {
            Check(Renderer.UploadMesh(static_cast<Engine::EPrimitiveType>(Index), Engine::GetPrimitiveMesh(static_cast<Engine::EPrimitiveType>(Index))), "Upload primitive");
        }
        Check(World.GetWorld()->SpawnPrimitive(Engine::EPrimitiveType::Cube) != nullptr, "Spawn geometry");
        auto* Actor{CoreUObject::Cast<Engine::AExponentionFogActor>(World.GetWorld()->SpawnActor(Engine::AExponentionFogActor::StaticClass()))};
        Check(Actor != nullptr && Actor->GetFogComponent() != nullptr, "Spawn fog actor");
        Check(Actor->GetFogComponent()->CreateFogProxy().mIcon != nullptr, "Unreal fog icon loaded");
        Check(!Actor->GetFogComponent()->SetFogDensity(-1.0f) && !Actor->GetFogComponent()->SetFogMaxOpacity(2.0f), "Fog property validation");
        Core::FString Error{};
        CoreUObject::FObjectDuplicationContext Duplication{};
        auto Copy{Engine::FWorldCloner::Clone(&World, Engine::EWorldType::PIE, &Duplication, Error)};
        Check(Copy != nullptr, Error.c_str());
        Check(Copy->GetWorld()->AttachScene(Renderer.AllocateScene()), "Fog registration after scene attach");
        Check(static_cast<Renderer::FScene*>(Copy->GetWorld()->GetScene())->GetHeightFogs().size() == 1, "Clone fog registration");
        World.GetWorld()->SendAllEndOfFrameUpdates();
        auto Target{Renderer.CreateRenderTarget(96, 96)};
        Engine::FSceneViewFamily Family{};
        Family.mScene = World.GetWorld()->GetScene();
        Family.mTarget = Target.get();
        Family.mView.mLocation = {-5.0f, -5.0f, 3.0f};
        Family.mView.mForward = (Core::FVector{} - Family.mView.mLocation).Normalized();
        Family.mView.mSize = {96.0f, 96.0f};
        Family.mView.mView = Core::FMatrix::LookTo(Family.mView.mLocation, Family.mView.mForward, {0.0f, 0.0f, 1.0f});
        Family.mView.mProjection = Core::FMatrix::Perspective(60.0f, 1.0f, 0.05f, 1000.0f);
        Family.mView.mViewProjection = Family.mView.mView * Family.mView.mProjection;
        Engine::FPrimitiveDrawInterface Elements{};
        Family.mEditorElements = &Elements;
        for (const Engine::EViewMode Mode : {Engine::EViewMode::Lit_Phong, Engine::EViewMode::WorldNormal, Engine::EViewMode::SceneDepth, Engine::EViewMode::Wireframe}) {
            Family.mViewMode = Mode;
            Renderer.BeginRenderingViewFamily(Family);
            Check(Renderer.GetStats().mVisiblePrimitives > 0, "Full renderer geometry");
        }
        CheckDebugMessages(static_cast<ID3D11Device*>(Renderer.GetNativeDevice()));
        Check(World.GetWorld()->DestroyActor(Actor->GetHandle()) && static_cast<Renderer::FScene*>(World.GetWorld()->GetScene())->GetHeightFogs().empty(), "Fog unregister on actor deletion");
        Check(Renderer.ResizeRenderTarget(Target.get(), 83, 61), "Full renderer resize");
        DestroyWindow(Window);
        std::cout << "PASS: full renderer, all buffer modes, Unreal icon, actor clone, deletion, resize\n";
    }

}

/**
 * @brief COM을 준비하고 7주차 이동·후처리·전체 렌더러 통합 검증을 실행합니다.
 * @return 모든 작업을 완료하면 0이며 예외가 발생하면 오류를 출력하고 1입니다.
 */
int main() {
    const HRESULT ComResult{CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)};
    try {
        Tests::Check(SUCCEEDED(ComResult), "COM initialization");
        Tests::CheckNamespaces();
        Tests::CheckMovement();
        Tests::CheckPostProcess();
        Tests::CheckRenderer();
        CoUninitialize();
        std::cout << "All Week 7 integration checks passed.\n";
        return 0;
    } catch (const std::exception& Error) {
        std::cerr << "FAIL: " << Error.what() << '\n';
        return 1;
    }
}
