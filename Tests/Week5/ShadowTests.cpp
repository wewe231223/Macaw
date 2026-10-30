/**
 * @file ShadowTests.cpp
 * @brief 그림자 아틀라스 할당과 광원별 그림자 렌더링을 검증합니다.
 */

#include "ShadowTests.h"
#include "FShadowAtlasAllocator.h"
#include "FShadowResources.h"
#include "Renderer/Renderer.h"
#include "Engine/PrimitiveMesh.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/UStaticMesh.h"
#include "Engine/UMaterial.h"
#include "Engine/ADirectionalLightActor.h"
#include "Engine/APointLightActor.h"
#include "Engine/ASpotLightActor.h"
#include "Engine/AAmbientLightActor.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/FAssetManager.h"
#include "Engine/Component/UDirectionalLightComponent.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/FWorldContext.h"
#include "Engine/FWorldCloner.h"
#include "Engine/SceneSerializer.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/FObjectDuplicationContext.h"
#include "Editor/FEditorContext.h"
#include "Editor/FViewportStatistics.h"
#include "Engine/Engine.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <iostream>

/**
 * @brief 검증 조건이 거짓이면 실패 메시지로 예외를 발생시킵니다.
 * @param[in] Condition 검증을 통과해야 하는 조건입니다.
 * @param[in] Message 검증 실패 시 예외에 포함할 메시지입니다.
 */
void Require(bool Condition, const char* Message);
/**
 * @brief 렌더 타깃의 중앙 픽셀을 CPU로 읽습니다.
 * @param[in] Renderer 검증에 사용할 렌더러입니다.
 * @param[in] Target 픽셀을 읽을 렌더 타깃입니다.
 * @return 중앙 픽셀의 RGBA 8비트 값입니다.
 */
std::array<Uint8, 4> ReadCenter(FRenderer& Renderer, FRenderTarget& Target);

/**
 * @brief 그림자 비교를 위해 렌더 타깃의 가로 픽셀 행을 읽습니다.
 * @param[in] Renderer 검증에 사용할 렌더러입니다.
 * @param[in] Target 픽셀을 읽을 렌더 타깃입니다.
 * @return 읽어 온 픽셀 바이트 배열입니다.
 */
TArray<Uint8> ReadShadowRow(FRenderer& Renderer, FRenderTarget& Target) {
    auto* View{static_cast<ID3D11ShaderResourceView*>(Target.GetNativeTexture())};
    Microsoft::WRL::ComPtr<ID3D11Resource> Resource{};
    View->GetResource(Resource.GetAddressOf());
    auto* Device{static_cast<ID3D11Device*>(Renderer.GetNativeDevice())};
    auto* Context{static_cast<ID3D11DeviceContext*>(Renderer.GetNativeContext())};
    D3D11_TEXTURE2D_DESC Description{};
    Description.Width = Target.GetWidth();
    Description.Height = 1;
    Description.MipLevels = 1;
    Description.ArraySize = 1;
    Description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Description.SampleDesc.Count = 1;
    Description.Usage = D3D11_USAGE_STAGING;
    Description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> Staging{};
    Require(SUCCEEDED(Device->CreateTexture2D(&Description, nullptr, Staging.GetAddressOf())), "Shadow row staging allocation failed");
    const D3D11_BOX Box{0, Target.GetHeight() / 2, 0, Target.GetWidth(), Target.GetHeight() / 2 + 1, 1};
    Context->CopySubresourceRegion(Staging.Get(), 0, 0, 0, 0, Resource.Get(), 0, &Box);
    D3D11_MAPPED_SUBRESOURCE Data{};
    Require(SUCCEEDED(Context->Map(Staging.Get(), 0, D3D11_MAP_READ, 0, &Data)), "Shadow row readback failed");
    const auto* Pixels{static_cast<const Uint8*>(Data.pData)};
    TArray<Uint8> Row{Pixels, Pixels + Description.Width * 4};
    Context->Unmap(Staging.Get(), 0);
    return Row;
}

/**
 * @brief 아틀라스 할당·캐스케이드 안정성과 광원 그림자 설정의 경계를 검증합니다.
 */
void TestShadowAllocation() {
    FShadowAtlasAllocator Allocator{1024};
    Require(!Allocator.Allocate(0).has_value() && !Allocator.Allocate(127).has_value() && !Allocator.Allocate(2048).has_value(), "Invalid shadow atlas allocation accepted");
    TArray<FShadowAtlasBlock> Blocks{};
    for (Uint32 Size : {512u, 256u, 128u, 64u}) {
        for (Uint32 Index{}; Index < 3; ++Index) {
            const auto Block{Allocator.Allocate(Size)};
            Require(Block.has_value(), "Quadtree allocation failed with free space");
            for (const FShadowAtlasBlock& Other : Blocks) {
                Require(Block->mX >= Other.mX + Other.mSize || Other.mX >= Block->mX + Block->mSize || Block->mY >= Other.mY + Other.mSize || Other.mY >= Block->mY + Block->mSize, "Shadow atlas tiles overlap");
            }
            Blocks.push_back(*Block);
        }
    }
    Allocator.Reset();
    for (Uint32 Index{}; Index < 256; ++Index) {
        Require(Allocator.Allocate(64).has_value(), "Quadtree lost usable atlas space");
    }
    Require(!Allocator.Allocate(64).has_value(), "Full shadow atlas accepted another map");
    Allocator.Reset();
    Require(Allocator.Allocate(1024).has_value(), "Shadow atlas reset did not reclaim space");

    FSceneView View{};
    View.mSize = {512.0f, 512.0f};
    View.mForward = {1.0f, 0.0f, 0.0f};
    View.mProjection = FMatrix::Perspective(60.0f, 1.0f, 0.1f, 100.0f);
    const FVector Direction{0.0f, 0.0f, -1.0f};
    const FMatrix First{FShadowResources::MakeDirectionalMatrix(View, Direction, 0.1f, 20.0f, 1016, {})};
    View.mLocation = {0.0f, 0.00001f, 0.0f};
    const FMatrix Shifted{FShadowResources::MakeDirectionalMatrix(View, Direction, 0.1f, 20.0f, 1016, {})};
    Require(First == Shifted, "Subtexel camera movement destabilized the CSM projection");
    View.mForward = {0.0f, 1.0f, 0.0f};
    View.mLocation = {};
    const FMatrix Rotated{FShadowResources::MakeDirectionalMatrix(View, Direction, 0.1f, 20.0f, 1016, {})};
    Require(std::abs(First(0, 0) - Rotated(0, 0)) < 0.000001f && std::abs(First(1, 1) - Rotated(1, 1)) < 0.000001f, "Camera rotation changed the CSM sphere extent");

    FAssetManager Assets{std::filesystem::absolute("Templates/Project/Content")};
    FWorldContext Context{};
    auto* Light{Cast<ADirectionalLightActor>(Context.GetWorld()->SpawnActor(ADirectionalLightActor::StaticClass()))};
    Require(Light != nullptr, "Shadow light spawn failed");
    UDirectionalLightComponent* Component{Light->GetLightComponent()};
    Require(Component->CreateLightProxy().mIcon != nullptr, "Directional Unreal icon fallback failed");
    for (const UClass* Class : {APointLightActor::StaticClass(), ASpotLightActor::StaticClass(), AAmbientLightActor::StaticClass()}) {
        const AActor* IconActor{Context.GetWorld()->SpawnActor(Class)};
        bool HasIcon{};
        for (const auto& ActorComponent : IconActor->GetComponents()) {
            if (const auto* IconLight{Cast<ULightComponentBase>(ActorComponent.get())}) {
                HasIcon = IconLight->CreateLightProxy().mIcon != nullptr;
            }
        }
        Require(HasIcon, "Unreal light icon did not attach on actor spawn");
    }
    Context.GetWorld()->SetAssets(&Assets);
    FString IconError{};
    Require(Component->CreateLightProxy().mIcon == Assets.LoadTexture("Editor/Icon/DirectionalLight_64x.png", IconError), "Changing the project's asset manager left a stale light icon");
    Context.GetWorld()->SetAssets(nullptr);
    Require(Component->CreateLightProxy().mIcon != nullptr, "Removing project assets lost the Unreal light icon fallback");
    AStaticMeshActor* MeshActor{Context.GetWorld()->SpawnPrimitive(EPrimitiveType::Cube)};
    Require(MeshActor->GetStaticMeshComponent()->SetCastShadow(false) && MeshActor->GetStaticMeshComponent()->SetReceiveShadow(false), "Object shadow property edit failed");
    Require(Component->SetShadowResolution(2048) && Component->SetShadowResolutionScale(0.5f) && Component->SetShadowBias(0.002f) && Component->SetShadowSlopeBias(2.0f) && Component->SetShadowSharpen(1.5f) && Component->SetCascadeCount(3) && Component->SetCascadeTransitionFraction(0.2f) && Component->SetShadowDistance(60.0f), "Shadow settings edit failed");
    Require(!Component->SetShadowResolution(1000) && !Component->SetShadowResolutionScale(std::numeric_limits<float>::quiet_NaN()) && !Component->SetShadowBias(-1.0f) && !Component->SetCascadeCount(5), "Invalid shadow settings accepted");
    FString Error{};
    Require(FSceneSerializer::Save("Intermediate/Week6Shadow.scene.masset", &Context, Error), Error.c_str());
    auto Restored{FSceneSerializer::Load("Intermediate/Week6Shadow.scene.masset", Error)};
    Require(Restored != nullptr, Error.c_str());
    FObjectDuplicationContext Duplication{};
    auto Clone{FWorldCloner::Clone(&Context, EWorldType::PIE, &Duplication, Error)};
    Require(Clone != nullptr, Error.c_str());
    for (const FWorldContext* Copy : {Restored.get(), Clone.get()}) {
        const auto* CopyLight{Cast<ADirectionalLightActor>(Copy->GetWorld()->GetPersistentLevel()->GetActors().front().get())};
        Require(CopyLight != nullptr && CopyLight->GetLightComponent()->GetCascadeCount() == 3 && CopyLight->GetLightComponent()->GetShadowResolution() == 2048 && CopyLight->GetLightComponent()->GetShadowBias() == 0.002f, "Shadow properties did not survive save/load or PIE cloning");
        const auto* CopyMesh{Cast<AStaticMeshActor>(Copy->GetWorld()->GetPersistentLevel()->GetActors().back().get())};
        Require(CopyMesh != nullptr && !CopyMesh->GetStaticMeshComponent()->GetCastShadow() && !CopyMesh->GetStaticMeshComponent()->GetReceiveShadow(), "Object shadow flags did not survive save/load or PIE cloning");
    }
    FViewportStatistics Statistics{};
    Require(Statistics.ExecuteCommand("stat shadow on", Error) && Statistics.IsEnabled(FViewportStatistics::EGroup::Shadow), "Shadow statistics command failed");
}

/**
 * @brief 광원별 그림자 생성과 그림자 표시 설정의 GPU 출력을 검증합니다.
 * @param[in] Renderer 검증에 사용할 렌더러입니다.
 */
void TestShadowRendering(FRenderer& Renderer) {
    auto Scene{Renderer.AllocateScene()};
    auto Target{Renderer.CreateRenderTarget(128, 128)};
    auto Plane{NewObject<UStaticMesh>()};
    auto Cube{NewObject<UStaticMesh>()};
    Require(Plane->SetMeshData(GetPrimitiveMesh(EPrimitiveType::Plane)) && Cube->SetMeshData(GetPrimitiveMesh(EPrimitiveType::Cube)), "Shadow test geometry failed");
    auto Material{NewObject<UMaterial>()};
    Material->SetTwoSided(true);
    Material->SetSpecularStrength(0.0f);
    FPrimitiveSceneProxy Receiver{};
    Receiver.mMesh = Plane.get();
    Receiver.mMaterial = Material.get();
    Receiver.mWorld = FMatrix::Scale({12.0f, 12.0f, 1.0f});
    Receiver.mBounds = Plane->GetBounds().Transformed(Receiver.mWorld);
    Scene->AddPrimitive(Receiver);
    FPrimitiveSceneProxy Caster{};
    Caster.mMesh = Cube.get();
    Caster.mMaterial = Material.get();
    Caster.mWorld = FMatrix::Translation({-2.0f, 0.0f, 2.0f});
    Caster.mBounds = Cube->GetBounds().Transformed(Caster.mWorld);
    Scene->AddPrimitive(Caster);
    FSceneView View{};
    View.mSize = {128.0f, 128.0f};
    View.mLocation = {0.0f, 0.0f, 8.0f};
    View.mForward = {0.0f, 0.0f, -1.0f};
    View.mView = FMatrix::LookTo(View.mLocation, View.mForward, {0.0f, 1.0f, 0.0f});
    View.mProjection = FMatrix::Orthographic(8.0f, 8.0f, 0.1f, 30.0f);
    View.mOrthographic = true;
    View.mViewProjection = View.mView * View.mProjection;
    FSceneViewFamily Family{Scene.get(), Target.get(), View, nullptr, EViewMode::Lit_Lambert};
    const auto Render{[&]() {
        Renderer.BeginFrame(256, 256);
        Renderer.BeginRenderingViewFamily(Family);
        return ReadCenter(Renderer, *Target);
    }};
    FLightSceneProxy Light{};
    Light.mType = ELightType::Directional;
    Light.mDirection = FVector{1.0f, 0.0f, -1.0f}.Normalized();
    Light.mCascadeCount = 1;
    Light.mShadowResolution = 256;
    Light.mShadowDistance = 25.0f;
    Scene->AddLight(Light);
    const auto Shadowed{Render()};
    Require(Shadowed[0] < 20 && Renderer.GetStats().mShadowMaps == 1, "Directional shadow did not occlude the receiver");
    const TArray<Uint8> FilteredRow{ReadShadowRow(Renderer, *Target)};
    bool HasFilteredEdge{};
    for (Uint32 X{42}; X < 86; ++X) {
        HasFilteredEdge = HasFilteredEdge || (FilteredRow[X * 4] > 20 && FilteredRow[X * 4] < 150);
    }
    Require(HasFilteredEdge, "PCF did not produce partially shadowed edge pixels");
    for (const EViewMode Mode : {EViewMode::Lit_Gouraud, EViewMode::Lit_Phong}) {
        Family.mViewMode = Mode;
        Require(Render()[0] < 20, "A lighting model ignored per-pixel shadows");
    }
    Family.mViewMode = EViewMode::Lit_Lambert;
    Light.mCascadeCount = 2;
    Scene->AddLight(Light);
    for (const float Height : {11.5f, 12.0f, 12.5f, 13.0f, 13.5f}) {
        Family.mView.mLocation.z = Height;
        Family.mView.mView = FMatrix::LookTo(Family.mView.mLocation, View.mForward, {0.0f, 1.0f, 0.0f});
        Family.mView.mViewProjection = Family.mView.mView * Family.mView.mProjection;
        Require(Render()[0] < 20 && Renderer.GetStats().mShadowMaps == 2, "CSM transition exposed a gap in shadow coverage");
    }
    Family.mView = View;
    Light.mCascadeCount = 1;
    Scene->AddLight(Light);
    Render();
    const FShadowAtlasView Atlas{Renderer.GetShadowAtlasView()};
    Require(Atlas.mSize == 16384 && Atlas.mTexture != nullptr && Atlas.mTiles.size() == 1 && Atlas.mDepthTexture != nullptr, "Shadow atlas preview is unavailable");
    Microsoft::WRL::ComPtr<ID3D11Resource> Resource{};
    static_cast<ID3D11ShaderResourceView*>(Atlas.mDepthTexture)->GetResource(Resource.GetAddressOf());
    Microsoft::WRL::ComPtr<ID3D11Texture2D> DepthTexture{};
    Require(SUCCEEDED(Resource.As(&DepthTexture)), "Shadow atlas is not a Texture2D");
    D3D11_TEXTURE2D_DESC Description{};
    DepthTexture->GetDesc(&Description);
    Require(Description.Width == 16384 && Description.Height == 16384 && Description.ArraySize == 1 && Description.Format == DXGI_FORMAT_R24G8_TYPELESS, "Shadow atlas dimensions or D24S8 storage mismatch");
    Scene->RemovePrimitive(Receiver.mId);
    FPrimitiveSceneProxy HiddenReceiver{Receiver};
    HiddenReceiver.mId = FGuid{};
    HiddenReceiver.mWorld = Receiver.mWorld * FMatrix::Translation({0.0f, 0.0f, -0.05f});
    HiddenReceiver.mBounds = Plane->GetBounds().Transformed(HiddenReceiver.mWorld);
    HiddenReceiver.mReceiveShadow = false;
    HiddenReceiver.mCastShadow = false;
    Scene->AddPrimitive(HiddenReceiver);
    Scene->AddPrimitive(Receiver);
    Require(Render()[0] < 20 && Renderer.GetStats().mDrawCalls == 2, "Batched receivers lost their individual shadow flags");
    Scene->RemovePrimitive(HiddenReceiver.mId);
    Receiver.mReceiveShadow = false;
    Scene->AddPrimitive(Receiver);
    Require(Render()[0] > 150, "ReceiveShadow did not disable shadow reception");
    Receiver.mReceiveShadow = true;
    Scene->AddPrimitive(Receiver);
    Caster.mCastShadow = false;
    Scene->AddPrimitive(Caster);
    Require(Render()[0] > 150, "CastShadow did not disable shadow casting");
    Caster.mCastShadow = true;
    Scene->AddPrimitive(Caster);
    Light.mCastShadows = false;
    Scene->AddLight(Light);
    Require(Render()[0] > 150 && Renderer.GetStats().mShadowMaps == 0, "CastShadows did not disable light shadows");
    Light.mCastShadows = true;
    Scene->AddLight(Light);
    Family.mShowFlags.mShadows = false;
    Require(Render()[0] > 150 && Renderer.GetStats().mShadowMaps == 0, "Shadow ShowFlag was ignored");
    Family.mShowFlags.mShadows = true;
    Family.mViewMode = EViewMode::Unlit;
    Require(Render()[0] > 250 && Renderer.GetStats().mShadowMaps == 0, "Unlit view rendered shadows");
    Family.mViewMode = EViewMode::Lit_Lambert;
    Scene->RemoveLight(Light.mId);
    Light.mType = ELightType::Spot;
    Light.mPosition = {-4.0f, 0.0f, 4.0f};
    Light.mDirection = FVector{1.0f, 0.0f, -1.0f}.Normalized();
    Light.mAttenuationRadius = 20.0f;
    Light.mLightFalloffExponent = 0.0f;
    Scene->AddLight(Light);
    Require(Render()[0] < 20 && Renderer.GetStats().mShadowMaps == 1, "Spot shadow did not occlude the receiver");
    Light.mType = ELightType::Point;
    Scene->AddLight(Light);
    Require(Render()[0] < 20 && Renderer.GetStats().mShadowMaps == 6, "Point shadow did not occlude the receiver");
    Light.mShadowResolution = 8192;
    Scene->AddLight(Light);
    Render();
    Require(Renderer.GetStats().mShadowMaps == 6 && Renderer.GetStats().mShadowReducedLights == 1 && Renderer.GetStats().mShadowDroppedLights == 0, "Atlas exhaustion did not reduce the complete six-face allocation");
    Light.mShadowResolution = 256;
    Scene->AddLight(Light);
    Caster.mWorld = FMatrix::Translation({-2.0f, 3.0f, 2.0f});
    Caster.mBounds = Cube->GetBounds().Transformed(Caster.mWorld);
    Scene->UpdatePrimitiveTransform(Caster.mId, Caster.mWorld, Caster.mBounds);
    Require(Render()[0] > 150, "Moving a shadow caster left a stale shadow");
    Light.mPosition = {-4.0f, 6.0f, 4.0f};
    Scene->AddLight(Light);
    Require(Render()[0] < 20, "Moving a point light left a stale shadow");
    FLightSceneProxy Directional{};
    Directional.mType = ELightType::Directional;
    Directional.mDirection = {0.0f, 0.0f, -1.0f};
    Directional.mCascadeCount = 4;
    Directional.mColor = {0.0f, 1.0f, 0.0f, 1.0f};
    Scene->AddLight(Directional);
    FLightSceneProxy Spot{};
    Spot.mType = ELightType::Spot;
    Spot.mPosition = {0.0f, 0.0f, 5.0f};
    Spot.mDirection = {0.0f, 0.0f, -1.0f};
    Spot.mColor = {0.0f, 0.0f, 1.0f, 1.0f};
    Scene->AddLight(Spot);
    Render();
    Require(Renderer.GetStats().mShadowMaps == 11 && Renderer.GetStats().mDirectionalLights == 1 && Renderer.GetStats().mPointLights == 1 && Renderer.GetStats().mSpotLights == 1, "Concurrent directional/point/spot shadow maps failed");
    const auto Mixed{ReadCenter(Renderer, *Target)};
    Require(Mixed[0] < 20 && Mixed[1] > 200 && Mixed[2] > 30, "One light's shadow incorrectly darkened other lights");
    UEngine Engine{};
    Require(Engine.Initialize(&Renderer), "Shadow console engine initialization failed");
    FEditorContext Context{&Engine};
    Context.ExecuteConsoleCommand("Shadow Atlas");
    Require(Context.IsShadowAtlasOpen(), "Shadow Atlas command did not open the viewer");
    Context.SetShadowAtlasOpen(false);
    Require(!Context.IsShadowAtlasOpen(), "Shadow atlas viewer could not close");
    Engine.Shutdown();
    std::cout << "Directional/spot/point shadows, CSM transitions, PCF edges, atlas fallback, flags, movement, preview and commands passed\n";
}
