#pragma once
#include "Core/Base/FRevisionCursor.h"
#include "Render/FMeshDrawData.h"
#include "Render/FRenderView.h"
#include "Render/Buffer/TGraphicsArray.h"
#include "RenderCore/FUVTransform.h"

#include <array>
#include <map>

class FRenderScene;
class FRenderQueue;

enum class ERenderStream : Uint8 {
    Text,
    TextContext,
    OverlayText,
    Billboard,
    LineDepth,
    LineOverlay,
    Count
};

class FRenderBuffers {
private:
    struct FViewConstants {
        FMatrix mView{};
        FMatrix mProjection{};
        FMatrix mViewProjection{};
        FMatrix mCameraWorld{};
        FVector4 mViewport{};
        FVector4 mGridFade{};
        Uint32 mLightCount{};
        FVector3 mPadding{};
    };

    struct FSceneBuffers {
        FRevisionCursor mAppliedRevision{};
        FGraphicsBuffer mTransforms{};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mResourceView{};
        Uint32 mCapacity{};
    };

    struct FStreamBuffer {
        FGraphicsBuffer mBuffer{};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mResourceView{};
        Uint32 mCapacity{};
    };

    static_assert(sizeof(FViewConstants) == 304);
    static_assert(sizeof(FMeshDrawRecord) == 16);

public:
    bool Initialize(ID3D11Device* Device, ID3D11DeviceContext* Context);
    void Reset();
    void ResetScenes();
    void ReleaseScene(Uint64 SceneId);

    bool PrepareView(ID3D11Device* Device, ID3D11DeviceContext* Context, const FRenderView& View, const FRenderScene& Scene, const FRenderQueue& Queue);
    bool PrepareView(ID3D11Device* Device, ID3D11DeviceContext* Context, const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport, const FVector4& GridFade, const TArray<FMatrix>& Transforms, const TArray<FMeshDrawRecord>& Records);
    bool PrepareOrientationAxis(ID3D11DeviceContext* Context, const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport);

    bool BindCommon(ID3D11DeviceContext* Context, bool OrientationAxis = false) const;
    bool BindModels(ID3D11DeviceContext* Context) const;

    bool UploadStream(ID3D11Device* Device, ID3D11DeviceContext* Context, ERenderStream Stream, const void* Data, Uint32 Count, Uint32 Stride, Uint32 BindFlags);
    ID3D11Buffer* GetStreamBuffer(ERenderStream Stream) const;
    ID3D11ShaderResourceView* GetStreamResourceView(ERenderStream Stream) const;
    bool HasCameraWorld() const;

private:
    bool InitializeConstantBuffer(ID3D11Device* Device, FGraphicsBuffer& Buffer, Uint32 ByteSize);
    bool UploadViewConstants(ID3D11DeviceContext* Context, FGraphicsBuffer& Buffer, const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport, const FVector4& GridFade, bool& HasCameraWorld);
    bool UploadModels(ID3D11Device* Device, ID3D11DeviceContext* Context, const TArray<FMeshDrawRecord>& Records);
    bool PrepareSceneTransforms(ID3D11Device* Device, ID3D11DeviceContext* Context, const FRenderScene& Scene);
    bool UpdateSceneTransforms(ID3D11Device* Device, ID3D11DeviceContext* Context, const FRenderScene& Scene, FSceneBuffers& Buffers);
    bool EnsureDrawRecordIndices(ID3D11Device* Device);
    void BindConstantBuffer(ID3D11DeviceContext* Context, Uint32 Slot, const FGraphicsBuffer& Buffer) const;

private:
    FGraphicsBuffer mViewConstants{};
    FGraphicsBuffer mOrientationAxisConstants{};
    FGraphicsBuffer mDrawRecordIndexBuffer{};
    TGraphicsArray<FLightShaderParameters> mLights{};
    TGraphicsArray<FMeshDrawRecord> mDrawRecords{};
    TGraphicsArray<FUVTransform> mUVTransforms{};
    TGraphicsArray<FMatrix> mLocalTransforms{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mSceneTransforms{};
    std::array<FStreamBuffer, static_cast<std::size_t>(ERenderStream::Count)> mStreams{};
    std::map<Uint64, FSceneBuffers> mScenes{};
    TArray<Uint32> mChangedObjects{};
    Uint64 mSceneId{};
    bool mViewReady{};
    bool mOrientationAxisReady{};
    bool mHasCameraWorld{};
};
