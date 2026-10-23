#include "pch.h"
#include "Render/FRenderBuffers.h"
#include "Render/FRenderQueue.h"
#include "Render/FRenderScene.h"

#include <algorithm>
#include <utility>

bool FRenderBuffers::Initialize(ID3D11Device* Device, ID3D11DeviceContext* Context) {
    Reset();

    if (Device == nullptr || Context == nullptr || !InitializeConstantBuffer(Device, mViewConstants, sizeof(FViewConstants)) || !InitializeConstantBuffer(Device, mOrientationAxisConstants, sizeof(FViewConstants)) || !mLights.Initialize(Device, Context, 16) || !mDrawRecords.Initialize(Device, Context, 128) || !mLocalTransforms.Initialize(Device, Context, 16) || !mUVTransforms.Initialize(Device, Context, 16)) {
        Reset();
        return false;
    }

    return true;
}

void FRenderBuffers::Reset() {
    ResetScenes();
    mViewConstants.Reset();
    mOrientationAxisConstants.Reset();
    mDrawRecordIndexBuffer.Reset();
    mLights.Reset();
    mDrawRecords.Reset();
    mUVTransforms.Reset();
    mLocalTransforms.Reset();
    mStreams = {};
}

void FRenderBuffers::ResetScenes() {
    mScenes.clear();
    mChangedObjects.clear();
    mSceneTransforms.Reset();
    mSceneId = 0;
    mViewReady = false;
    mOrientationAxisReady = false;
    mHasCameraWorld = false;
}

void FRenderBuffers::ReleaseScene(Uint64 SceneId) {
    mScenes.erase(SceneId);

    if (mSceneId == SceneId) {
        mSceneTransforms.Reset();
        mSceneId = 0;
        mViewReady = false;
        mOrientationAxisReady = false;
        mHasCameraWorld = false;
    }
}

bool FRenderBuffers::PrepareView(ID3D11Device* Device, ID3D11DeviceContext* Context, const FRenderView& View, const FRenderScene& Scene, const FRenderQueue& Queue) {
    mViewReady = false;
    mOrientationAxisReady = false;
    mHasCameraWorld = false;

    if (Device == nullptr || Context == nullptr || View.mTarget == nullptr || !View.mTarget->IsValid()) {
        return false;
    }

    if (!mLights.Upload(Device, Context, Scene.GetLights()) || !PrepareSceneTransforms(Device, Context, Scene) || !UploadModels(Device, Context, Queue.GetDrawRecords()) || !mUVTransforms.Upload(Device, Context, Queue.GetUVTransforms())) {
        return false;
    }

    mSceneId = Scene.GetId();
    mViewReady = UploadViewConstants(Context, mViewConstants, View.mCamera, View.mTarget->GetViewport(), FVector4{}, mHasCameraWorld);

    return mViewReady;
}

bool FRenderBuffers::PrepareView(ID3D11Device* Device, ID3D11DeviceContext* Context, const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport, const FVector4& GridFade, const TArray<FMatrix>& Transforms, const TArray<FMeshDrawRecord>& Records) {
    mViewReady = false;
    mOrientationAxisReady = false;
    mHasCameraWorld = false;
    mSceneId = 0;
    mLights.Clear();
    mUVTransforms.Clear();
    mSceneTransforms.Reset();

    if (Device == nullptr || Context == nullptr || !mLocalTransforms.Upload(Device, Context, Transforms) || !UploadModels(Device, Context, Records)) {
        return false;
    }

    mSceneTransforms = mLocalTransforms.GetSRV()[0];
    mViewReady = UploadViewConstants(Context, mViewConstants, Camera, Viewport, GridFade, mHasCameraWorld);

    return mViewReady;
}

bool FRenderBuffers::PrepareOrientationAxis(ID3D11DeviceContext* Context, const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport) {
    mOrientationAxisReady = false;

    if (!mViewReady) {
        return false;
    }

    bool HasCameraWorld{};

    mOrientationAxisReady = UploadViewConstants(Context, mOrientationAxisConstants, Camera, Viewport, FVector4{}, HasCameraWorld);

    return mOrientationAxisReady;
}

bool FRenderBuffers::BindCommon(ID3D11DeviceContext* Context, bool OrientationAxis) const {
    if (Context == nullptr || !mViewReady || (OrientationAxis && !mOrientationAxisReady)) {
        return false;
    }

    BindConstantBuffer(Context, 1, OrientationAxis ? mOrientationAxisConstants : mViewConstants);
    Context->PSSetShaderResources(2, 1, mLights.GetSRV());

    return true;
}

bool FRenderBuffers::BindModels(ID3D11DeviceContext* Context) const {
    if (!BindCommon(Context) || mDrawRecords.IsEmpty() || mSceneTransforms == nullptr || !mDrawRecordIndexBuffer.IsValid()) {
        return false;
    }

    Context->VSSetShaderResources(0, 1, mDrawRecords.GetSRV());
    Context->PSSetShaderResources(0, 1, mDrawRecords.GetSRV());
    Context->VSSetShaderResources(15, 1, mSceneTransforms.GetAddressOf());
    Context->PSSetShaderResources(15, 1, mSceneTransforms.GetAddressOf());
    Context->VSSetShaderResources(16, 1, mUVTransforms.GetSRV());
    Context->PSSetShaderResources(16, 1, mUVTransforms.GetSRV());

    ID3D11Buffer* DrawRecordIndexBuffer{mDrawRecordIndexBuffer.GetBuffer()};
    const UINT Stride{sizeof(Uint32)};
    const UINT Offset{};

    Context->IASetVertexBuffers(4, 1, &DrawRecordIndexBuffer, &Stride, &Offset);

    return true;
}

bool FRenderBuffers::UploadStream(ID3D11Device* Device, ID3D11DeviceContext* Context, ERenderStream Stream, const void* Data, Uint32 Count, Uint32 Stride, Uint32 BindFlags) {
    const std::size_t Index{static_cast<std::size_t>(Stream)};

    if (Index >= mStreams.size() || Device == nullptr || Context == nullptr || Data == nullptr || Count == 0 || Stride == 0 || Count > UINT32_MAX / Stride || (BindFlags != D3D11_BIND_VERTEX_BUFFER && BindFlags != D3D11_BIND_SHADER_RESOURCE)) {
        return false;
    }

    FStreamBuffer& Destination{mStreams[Index]};
    const bool Structured{BindFlags == D3D11_BIND_SHADER_RESOURCE};

    if (Destination.mCapacity < Count || Destination.mBuffer.GetStride() != (Structured ? Stride : 0) || Destination.mBuffer.GetBindFlags() != BindFlags) {
        Uint32 Capacity{std::max(Destination.mCapacity, 1u)};

        while (Capacity < Count && Capacity <= UINT32_MAX / 2) {
            Capacity *= 2;
        }

        Capacity = std::max(Capacity, Count);

        if (Capacity > UINT32_MAX / Stride) {
            Capacity = Count;
        }

        FGraphicsBufferDescription Description{};

        Description.mByteSize = Capacity * Stride;
        Description.mStride = Structured ? Stride : 0;
        Description.mUsage = Structured ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC;
        Description.mBindFlags = BindFlags;
        Description.mCpuAccessFlags = Structured ? 0 : D3D11_CPU_ACCESS_WRITE;
        Description.mMiscFlags = Structured ? D3D11_RESOURCE_MISC_BUFFER_STRUCTURED : 0;

        FGraphicsBuffer Buffer{};

        if (!Buffer.Initialize(Device, Description)) {
            return false;
        }

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ResourceView{};

        if (Structured) {
            D3D11_SHADER_RESOURCE_VIEW_DESC ViewDescription{};

            ViewDescription.Format = DXGI_FORMAT_UNKNOWN;
            ViewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            ViewDescription.Buffer.NumElements = Capacity;

            if (FAILED(Device->CreateShaderResourceView(Buffer.GetBuffer(), &ViewDescription, ResourceView.GetAddressOf()))) {
                return false;
            }
        }

        Destination.mBuffer = std::move(Buffer);
        Destination.mResourceView = std::move(ResourceView);
        Destination.mCapacity = Capacity;
    }

    return Structured ? Destination.mBuffer.Update(Context, Data, Count * Stride) : Destination.mBuffer.WriteDiscard(Context, Data, Count * Stride);
}

ID3D11Buffer* FRenderBuffers::GetStreamBuffer(ERenderStream Stream) const {
    const std::size_t Index{static_cast<std::size_t>(Stream)};

    return Index < mStreams.size() ? mStreams[Index].mBuffer.GetBuffer() : nullptr;
}

ID3D11ShaderResourceView* FRenderBuffers::GetStreamResourceView(ERenderStream Stream) const {
    const std::size_t Index{static_cast<std::size_t>(Stream)};

    return Index < mStreams.size() ? mStreams[Index].mResourceView.Get() : nullptr;
}

bool FRenderBuffers::HasCameraWorld() const {
    return mViewReady && mHasCameraWorld;
}

bool FRenderBuffers::InitializeConstantBuffer(ID3D11Device* Device, FGraphicsBuffer& Buffer, Uint32 ByteSize) {
    FGraphicsBufferDescription Description{};

    Description.mByteSize = ByteSize;
    Description.mUsage = D3D11_USAGE_DYNAMIC;
    Description.mBindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Description.mCpuAccessFlags = D3D11_CPU_ACCESS_WRITE;

    return Buffer.Initialize(Device, Description);
}

bool FRenderBuffers::UploadViewConstants(ID3D11DeviceContext* Context, FGraphicsBuffer& Buffer, const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport, const FVector4& GridFade, bool& HasCameraWorld) {
    if (Context == nullptr || Viewport.Width <= 0.0f || Viewport.Height <= 0.0f) {
        return false;
    }

    FViewConstants Constants{};

    Constants.mView = Camera.mView;
    Constants.mProjection = Camera.mProjection;
    Constants.mViewProjection = Camera.mViewProjection;
    HasCameraWorld = Camera.mView.TryInverse(Constants.mCameraWorld);
    Constants.mViewport = FVector4{Viewport.Width, Viewport.Height, 1.0f / Viewport.Width, 1.0f / Viewport.Height};
    Constants.mGridFade = GridFade;
    Constants.mLightCount = mLights.GetCount();

    return Buffer.WriteDiscard(Context, &Constants, sizeof(Constants));
}

bool FRenderBuffers::UploadModels(ID3D11Device* Device, ID3D11DeviceContext* Context, const TArray<FMeshDrawRecord>& Records) {
    return mDrawRecords.Upload(Device, Context, Records) && (mDrawRecords.IsEmpty() || EnsureDrawRecordIndices(Device));
}

bool FRenderBuffers::PrepareSceneTransforms(ID3D11Device* Device, ID3D11DeviceContext* Context, const FRenderScene& Scene) {
    FSceneBuffers& Buffers{mScenes[Scene.GetId()]};

    if (!Buffers.mAppliedRevision.IsCurrent(Scene.GetRevision()) || Buffers.mResourceView == nullptr) {
        if (!UpdateSceneTransforms(Device, Context, Scene, Buffers)) {
            return false;
        }

        Buffers.mAppliedRevision.Commit(Scene.GetRevision());
    }

    mSceneTransforms = Buffers.mResourceView;

    return true;
}

bool FRenderBuffers::UpdateSceneTransforms(ID3D11Device* Device, ID3D11DeviceContext* Context, const FRenderScene& Scene, FSceneBuffers& Buffers) {
    const TArray<FMatrix>& Transforms{Scene.GetObjectTransforms()};

    if (Transforms.size() > UINT32_MAX / sizeof(FMatrix)) {
        return false;
    }

    mChangedObjects.clear();

    bool FullUpload{Buffers.mResourceView == nullptr || Scene.CollectChangedObjects(Buffers.mAppliedRevision.GetRevision(), mChangedObjects) == ERenderUpdateMode::Full};

    if (Buffers.mCapacity < Transforms.size() || Buffers.mResourceView == nullptr) {
        Uint32 Capacity{std::max(Buffers.mCapacity, 1u)};

        while (Capacity < Transforms.size() && Capacity <= UINT32_MAX / sizeof(FMatrix) / 2) {
            Capacity *= 2;
        }

        Capacity = std::max(Capacity, static_cast<Uint32>(Transforms.size()));

        FGraphicsBufferDescription Description{};

        Description.mByteSize = Capacity * sizeof(FMatrix);
        Description.mStride = sizeof(FMatrix);
        Description.mBindFlags = D3D11_BIND_SHADER_RESOURCE;
        Description.mMiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

        FGraphicsBuffer Buffer{};

        if (!Buffer.Initialize(Device, Description)) {
            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC ViewDescription{};

        ViewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        ViewDescription.Buffer.NumElements = Capacity;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ResourceView{};

        if (FAILED(Device->CreateShaderResourceView(Buffer.GetBuffer(), &ViewDescription, ResourceView.GetAddressOf()))) {
            return false;
        }

        Buffers.mTransforms = std::move(Buffer);
        Buffers.mResourceView = std::move(ResourceView);
        Buffers.mCapacity = Capacity;
        Buffers.mAppliedRevision.Invalidate();
        FullUpload = true;
    }

    if (Transforms.empty() || (!FullUpload && mChangedObjects.empty())) {
        return true;
    }

    if (FullUpload || mChangedObjects.back() >= Transforms.size()) {
        return Buffers.mTransforms.Update(Context, Transforms.data(), static_cast<Uint32>(Transforms.size() * sizeof(FMatrix)));
    }

    for (std::size_t Begin{}; Begin < mChangedObjects.size();) {
        std::size_t End{Begin + 1};

        while (End < mChangedObjects.size() && mChangedObjects[End] == mChangedObjects[End - 1] + 1) {
            ++End;
        }

        const Uint32 ObjectIndex{mChangedObjects[Begin]};

        if (!Buffers.mTransforms.Update(Context, Transforms.data() + ObjectIndex, static_cast<Uint32>((End - Begin) * sizeof(FMatrix)), ObjectIndex * sizeof(FMatrix))) {
            Buffers.mAppliedRevision.Invalidate();
            return false;
        }

        Begin = End;
    }

    return true;
}

bool FRenderBuffers::EnsureDrawRecordIndices(ID3D11Device* Device) {
    const Uint32 Capacity{mDrawRecords.GetCapacity()};

    if (mDrawRecordIndexBuffer.GetByteSize() / sizeof(Uint32) >= Capacity) {
        return true;
    }

    TArray<Uint32> DrawRecordIndices{};

    DrawRecordIndices.resize(Capacity);

    for (Uint32 Index{}; Index < Capacity; ++Index) {
        DrawRecordIndices[Index] = Index;
    }

    FGraphicsBufferDescription Description{};

    Description.mByteSize = Capacity * sizeof(Uint32);
    Description.mUsage = D3D11_USAGE_IMMUTABLE;
    Description.mBindFlags = D3D11_BIND_VERTEX_BUFFER;

    return mDrawRecordIndexBuffer.Initialize(Device, Description, DrawRecordIndices.data());
}

void FRenderBuffers::BindConstantBuffer(ID3D11DeviceContext* Context, Uint32 Slot, const FGraphicsBuffer& Buffer) const {
    ID3D11Buffer* ConstantBuffer{Buffer.GetBuffer()};

    Context->VSSetConstantBuffers(Slot, 1, &ConstantBuffer);
    Context->HSSetConstantBuffers(Slot, 1, &ConstantBuffer);
    Context->DSSetConstantBuffers(Slot, 1, &ConstantBuffer);
    Context->GSSetConstantBuffers(Slot, 1, &ConstantBuffer);
    Context->PSSetConstantBuffers(Slot, 1, &ConstantBuffer);
}
