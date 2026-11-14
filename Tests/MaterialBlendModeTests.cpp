#include "Asset/UMaterial.h"
#include "Asset/UStaticMesh.h"
#include "Asset/UTexture2D.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Engine/FSceneViewFamily.h"
#include "FMaterialAssetData.h"
#include "FScene.h"
#include "RenderTarget.h"
#include "Renderer/Renderer.h"
#include "Serialization/MemoryArchive.h"

#include <Windows.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <wrl/client.h>

namespace {

    void Require(bool Condition, const Core::FString& Message) {
        if (!Condition) {
            throw std::runtime_error{Message};
        }

        std::cout << "PASS " << Message << '\n';
    }

    class FTestWindow {
    public:
        FTestWindow();
        ~FTestWindow();

    public:
        HWND GetHandle() const;

    private:
        HWND mHandle{};
    };

    FTestWindow::FTestWindow() {
        WNDCLASSW Description{};
        Description.lpfnWndProc = DefWindowProcW;
        Description.hInstance = GetModuleHandleW(nullptr);
        Description.lpszClassName = L"MacawMaterialBlendModeTests";
        RegisterClassW(&Description);

        mHandle = CreateWindowExW(0, Description.lpszClassName, L"Material blend mode tests", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, Description.hInstance, nullptr);
        Require(mHandle != nullptr, "Create hidden render window");
    }

    FTestWindow::~FTestWindow() {
        DestroyWindow(mHandle);
        UnregisterClassW(L"MacawMaterialBlendModeTests", GetModuleHandleW(nullptr));
    }

    HWND FTestWindow::GetHandle() const {
        return mHandle;
    }

    void TestMaterialModes() {
        auto Material{CoreUObject::NewObject<Asset::UMaterial>()};
        Require(Material->GetBlendMode() == Asset::EBlendMode::Opaque, "New material is opaque");
        Require(Material->SetBaseColor({1.0f, 0.0f, 0.0f, 0.5f}) && Material->GetBlendMode() == Asset::EBlendMode::Translucent, "Base color alpha selects translucency");
        Require(Material->SetBlendMode(Asset::EBlendMode::Opaque) && Material->GetBlendMode() == Asset::EBlendMode::Translucent, "Non-unit alpha keeps translucency when opaque is requested");
        Require(Material->SetBaseColor({1.0f, 0.0f, 0.0f, 1.0f}) && Material->GetBlendMode() == Asset::EBlendMode::Opaque, "Restoring alpha selects opaque");
        Require(Material->SetBlendMode(Asset::EBlendMode::Translucent) && Material->SetBaseColor({0.0f, 1.0f, 0.0f, 1.0f}) && Material->GetBlendMode() == Asset::EBlendMode::Translucent, "RGB edits preserve explicitly selected translucency");
        Require(!Material->SetBlendMode(static_cast<Asset::EBlendMode>(255)), "Reject invalid blend mode");
        Require(!Material->SetBaseColor({1.0f, 1.0f, 1.0f, std::numeric_limits<float>::quiet_NaN()}) && Material->GetBaseColor().w == 1.0f, "Reject invalid alpha without changing material");
        Require(Material->SetBaseColor({1.0f, 1.0f, 1.0f, std::nextafter(1.0f, 0.0f)}) && Material->GetBlendMode() == Asset::EBlendMode::Translucent, "Alpha comparison uses exactly one");

        for (const float Alpha : {0.0f, 0.5f, 1.0f}) {
            Material->SetBaseColor({0.2f, 0.4f, 0.6f, Alpha});
            Material->SetBlendMode(Asset::EBlendMode::Translucent);
            Core::TArray<Core::Uint8> Bytes{};
            Serialization::FMemoryArchive Writer{Bytes};
            Asset::FMaterialAssetData Saved{*Material};
            Saved.Serialize(Writer);

            Serialization::FMemoryArchive Reader{static_cast<const Core::TArray<Core::Uint8>&>(Bytes)};
            Asset::FMaterialAssetData Loaded{};
            Loaded.Serialize(Reader);
            auto Copy{CoreUObject::NewObject<Asset::UMaterial>()};
            Core::FString Error{};
            Require(!Writer.HasError() && !Reader.HasError() && Reader.GetRemainingBytes() == 0 && Loaded.ApplyTo(*Copy, Error) && Copy->GetBlendMode() == Asset::EBlendMode::Translucent && Copy->GetBaseColor() == Material->GetBaseColor(), "Material asset round trip preserves blend mode and alpha");
        }

        Core::TArray<Core::Uint8> ReflectedBytes{};
        Serialization::FMemoryArchive ReflectedWriter{ReflectedBytes};
        Material->Serialize(ReflectedWriter);
        auto ReflectedCopy{CoreUObject::NewObject<Asset::UMaterial>()};
        Serialization::FMemoryArchive ReflectedReader{static_cast<const Core::TArray<Core::Uint8>&>(ReflectedBytes)};
        ReflectedCopy->Serialize(ReflectedReader);
        Require(!ReflectedReader.HasError() && ReflectedCopy->GetBlendMode() == Asset::EBlendMode::Translucent, "Reflection serialization preserves explicit blend mode");

        Core::TArray<Core::Uint8> LegacyBytes{};
        Serialization::FMemoryArchive LegacyWriter{LegacyBytes};
        Core::Uint32 Version{1};
        Core::FString Name{"Legacy"};
        Core::FString Shader{"Shaders/Lit.hlsl"};
        Core::FString Empty{};
        Core::FVector4 Color{1.0f, 1.0f, 1.0f, 0.5f};
        float Specular{0.35f};
        float Shininess{32.0f};
        float NormalStrength{1.0f};
        bool TwoSided{};
        LegacyWriter.Serialize("Version", Version);
        LegacyWriter.Serialize("Name", Name);
        LegacyWriter.Serialize("Shader", Shader);
        LegacyWriter.Serialize("BaseColor", Color);
        LegacyWriter.Serialize("BaseColorTexture", Empty);
        LegacyWriter.Serialize("NormalMap", Empty);
        LegacyWriter.Serialize("SpecularStrength", Specular);
        LegacyWriter.Serialize("Shininess", Shininess);
        LegacyWriter.Serialize("NormalStrength", NormalStrength);
        LegacyWriter.Serialize("TwoSided", TwoSided);
        Serialization::FMemoryArchive LegacyReader{static_cast<const Core::TArray<Core::Uint8>&>(LegacyBytes)};
        Asset::FMaterialAssetData Legacy{};
        Legacy.Serialize(LegacyReader);
        auto LegacyMaterial{CoreUObject::NewObject<Asset::UMaterial>()};
        Core::FString Error{};
        Require(!LegacyReader.HasError() && Legacy.ApplyTo(*LegacyMaterial, Error) && LegacyMaterial->GetBlendMode() == Asset::EBlendMode::Translucent, "Legacy material derives translucency from saved alpha");
    }

    Asset::FMeshData MakeMeshData() {
        Asset::FMeshData Data{};
        Data.GetPositions() = {{-0.9f, -0.9f, 0.0f}, {-0.9f, 0.9f, 0.0f}, {0.9f, 0.9f, 0.0f}, {0.9f, -0.9f, 0.0f}};
        Data.GetNormals().assign(4, {0.0f, 0.0f, -1.0f});
        Data.GetTangents().assign(4, {1.0f, 0.0f, 0.0f, 1.0f});
        Data.GetUVChannels() = {{{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}}};
        Data.GetColors().assign(4, {1.0f, 1.0f, 1.0f, 1.0f});
        Data.GetIndices() = {0, 1, 2, 0, 2, 3};
        Data.GetSubMeshes().emplace_back(0, 6, 0);
        Data.GetMaterialSlots().emplace_back("Default", "");
        Data.ComputeBounds();

        return Data;
    }

    Engine::FPrimitiveSceneProxy MakeProxy(Asset::UStaticMesh& Mesh, Asset::UMaterial& Material, float Depth) {
        Engine::FPrimitiveSceneProxy Proxy{};
        Proxy.mMesh = &Mesh;
        Proxy.mMaterial = &Material;
        Proxy.mWorld = Core::FMatrix::Translation({0.0f, 0.0f, Depth});
        Proxy.mBounds = {{-0.9f, -0.9f, Depth}, {0.9f, 0.9f, Depth}};
        Proxy.mCastShadow = false;

        return Proxy;
    }

    Core::FVector4 ReadCenter(Renderer::FRenderer& Renderer, Engine::FRenderTarget& Target) {
        auto* Device{static_cast<ID3D11Device*>(Renderer.GetNativeDevice())};
        auto* Context{static_cast<ID3D11DeviceContext*>(Renderer.GetNativeContext())};
        auto* Native{static_cast<Renderer::FDXRenderTarget*>(&Target)};
        ID3D11Texture2D* Texture{Native->GetOutputColor()->GetTexture()};
        D3D11_TEXTURE2D_DESC Description{};
        Texture->GetDesc(&Description);
        Description.Usage = D3D11_USAGE_STAGING;
        Description.BindFlags = 0;
        Description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        Description.MiscFlags = 0;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> Staging{};
        Require(SUCCEEDED(Device->CreateTexture2D(&Description, nullptr, Staging.GetAddressOf())), "Create render readback texture");
        Context->CopyResource(Staging.Get(), Texture);
        D3D11_MAPPED_SUBRESOURCE Mapped{};
        Require(SUCCEEDED(Context->Map(Staging.Get(), 0, D3D11_MAP_READ, 0, &Mapped)), "Read rendered pixels");
        const auto* Pixel{static_cast<const Core::Uint8*>(Mapped.pData) + 32 * Mapped.RowPitch + 32 * 4};
        const Core::FVector4 Color{Pixel[0] / 255.0f, Pixel[1] / 255.0f, Pixel[2] / 255.0f, Pixel[3] / 255.0f};
        Context->Unmap(Staging.Get(), 0);

        return Color;
    }

    void RequireColor(Renderer::FRenderer& Renderer, Engine::FSceneViewFamily& Family, const Core::FVector& Expected, const Core::FString& Message) {
        Renderer.BeginRenderingViewFamily(Family);
        const Core::FVector4 Color{ReadCenter(Renderer, *Family.mTarget)};
        std::cout << "COLOR " << Color.x << ' ' << Color.y << ' ' << Color.z << '\n';
        Require(std::abs(Color.x - Expected.x) < 0.015f && std::abs(Color.y - Expected.y) < 0.015f && std::abs(Color.z - Expected.z) < 0.015f, Message);
    }

    void TestRendering() {
        FTestWindow Window{};
        Renderer::FRenderer Renderer{};
        Renderer.SetProjectPaths(std::filesystem::absolute("Templates/Project/Content"), std::filesystem::absolute("Build/MaterialBlendModeTests/ShaderCache"));
        Require(Renderer.Initialize(Window.GetHandle(), 64, 64), "Initialize renderer");
        auto Mesh{CoreUObject::NewObject<Asset::UStaticMesh>()};
        Require(Mesh->SetMeshData(MakeMeshData()), "Create test mesh");
        auto Blue{CoreUObject::NewObject<Asset::UMaterial>()};
        auto Red{CoreUObject::NewObject<Asset::UMaterial>()};
        auto Green{CoreUObject::NewObject<Asset::UMaterial>()};

        for (Asset::UMaterial* Material : {Blue.get(), Red.get(), Green.get()}) {
            Material->SetTwoSided(true);
        }

        Blue->SetBaseColor({0.0f, 0.0f, 1.0f, 1.0f});
        Red->SetBaseColor({1.0f, 0.0f, 0.0f, 0.5f});
        Green->SetBaseColor({0.0f, 1.0f, 0.0f, 0.5f});

        for (const Engine::EViewMode Mode : {Engine::EViewMode::Unlit, Engine::EViewMode::Lit_Phong, Engine::EViewMode::Lit_Lambert, Engine::EViewMode::Lit_Gouraud}) {
            Require(Renderer.GetMaterialPipeline(*Red, Mode, false, true) != nullptr && Renderer.GetMaterialPipeline(*Red, Mode, true, true) != nullptr, "Compile static and skinned translucent lighting pipelines");
        }

        auto Scene{Renderer.AllocateScene()};
        auto Target{Renderer.CreateRenderTarget(64, 64)};
        auto BlueProxy{MakeProxy(*Mesh, *Blue, 0.8f)};
        auto RedProxy{MakeProxy(*Mesh, *Red, 0.6f)};
        auto GreenProxy{MakeProxy(*Mesh, *Green, 0.2f)};
        Scene->AddPrimitive(GreenProxy);
        Scene->AddPrimitive(BlueProxy);
        Scene->AddPrimitive(RedProxy);
        Engine::FSceneViewFamily Family{};
        Family.mScene = Scene.get();
        Family.mTarget = Target.get();
        Family.mView.mSize = {64.0f, 64.0f};
        Family.mView.mLocation = {0.0f, 0.0f, -1.0f};
        Family.mView.mForward = {0.0f, 0.0f, 1.0f};
        Family.mViewMode = Engine::EViewMode::Unlit;
        Family.mShowFlags.mSky = false;
        Family.mShowFlags.mFog = false;
        Family.mShowFlags.mFXAA = false;
        Family.mShowFlags.mShadows = false;
        RequireColor(Renderer, Family, {0.25f, 0.5f, 0.25f}, "Translucent meshes blend back to front regardless of insertion order");
        Green->SetBaseColor({0.0f, 1.0f, 0.0f, 0.0f});
        RequireColor(Renderer, Family, {0.5f, 0.0f, 0.5f}, "Zero alpha preserves objects behind the mesh");
        Green->SetBaseColor({0.0f, 1.0f, 0.0f, 1.0f});
        RequireColor(Renderer, Family, {0.0f, 1.0f, 0.0f}, "Restoring alpha moves mesh to opaque depth rendering");
        Green->SetBaseColor({0.0f, 1.0f, 0.0f, 0.5f});

        auto Texture{CoreUObject::NewObject<Asset::UTexture2D>()};
        Require(Texture->SetPixels(1, 1, Asset::ETextureFormat::RGBA8, {255, 255, 255, 255}), "Create sprite texture");
        Engine::FPrimitiveSceneProxy Sprite{};
        Sprite.mWorld = Core::FMatrix::Translation({0.0f, 0.0f, 0.4f});
        Sprite.mSprite.emplace();
        Sprite.mSprite->mTexture = Texture.get();
        Sprite.mSprite->mColor = {1.0f, 1.0f, 0.0f, 0.5f};
        Sprite.mCastShadow = false;
        Scene->AddPrimitive(Sprite);
        RequireColor(Renderer, Family, {0.375f, 0.75f, 0.125f}, "Sprite between translucent meshes shares the same depth ordering");
        Sprite.mWorld = Core::FMatrix::Translation({0.0f, 0.0f, 0.1f});
        Scene->AddPrimitive(Sprite);
        RequireColor(Renderer, Family, {0.625f, 0.75f, 0.125f}, "Moving a sprite updates ordering against translucent meshes");
        Family.mShowFlags.mSprites = false;
        RequireColor(Renderer, Family, {0.25f, 0.5f, 0.25f}, "Sprite visibility flag preserves mesh translucency");

        Engine::FLightSceneProxy Ambient{};
        Ambient.mCastShadows = false;
        Scene->AddLight(Ambient);
        Family.mShowFlags.mSky = true;

        for (const Engine::EViewMode Mode : {Engine::EViewMode::Lit_Phong, Engine::EViewMode::Lit_Lambert, Engine::EViewMode::Lit_Gouraud}) {
            Family.mViewMode = Mode;
            RequireColor(Renderer, Family, {0.25f, 0.5f, 0.25f}, "Forward translucent lighting remains bound after opaque lighting and sky");
        }

        auto MixedMesh{CoreUObject::NewObject<Asset::UStaticMesh>()};
        Asset::FMeshData MixedData{MakeMeshData()};
        MixedData.GetIndices() = {0, 1, 2, 0, 2, 3, 0, 1, 2, 0, 2, 3};
        MixedData.GetSubMeshes().emplace_back(6, 6, 1);
        MixedData.GetMaterialSlots().emplace_back("Translucent", "");
        Require(MixedMesh->SetMeshData(std::move(MixedData)), "Create mixed material mesh");
        auto MixedProxy{MakeProxy(*MixedMesh, *Blue, 0.5f)};
        MixedProxy.mMaterial = nullptr;
        MixedProxy.mMaterials.emplace_back(Blue.get());
        MixedProxy.mMaterials.emplace_back(Red.get());
        auto MixedScene{Renderer.AllocateScene()};
        MixedScene->AddPrimitive(MixedProxy);
        Family.mScene = MixedScene.get();
        Family.mViewMode = Engine::EViewMode::Unlit;
        Family.mShowFlags.mSky = false;
        RequireColor(Renderer, Family, {0.5f, 0.0f, 0.5f}, "Material slots route opaque and translucent submeshes independently");

        auto ShadowScene{Renderer.AllocateScene()};
        GreenProxy.mCastShadow = true;
        ShadowScene->AddPrimitive(GreenProxy);
        const Core::FRevision Before{static_cast<Renderer::FScene*>(ShadowScene.get())->GetCasterRevision()};
        Green->SetBaseColor({0.0f, 1.0f, 0.0f, 1.0f});
        Require(static_cast<Renderer::FScene*>(ShadowScene.get())->GetCasterRevision() != Before, "Changing blend mode invalidates cached shadow caster state");
    }

}

int main() {
    try {
        TestMaterialModes();
        TestRendering();
    } catch (const std::exception& Error) {
        std::cerr << "FAIL " << Error.what() << '\n';

        return 1;
    }

    return 0;
}
