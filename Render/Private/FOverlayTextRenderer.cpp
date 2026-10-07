#include "pch.h"
#include "Render/FOverlayTextRenderer.h"
#include "Asset/UFont.h"
#include "Asset/FTextGeometry.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <ranges>

namespace {
    FVector4 TransformClip(const FVector3& Position, const FMatrix& Matrix) {
        return FVector4{Position.mX * Matrix.M[0][0] + Position.mY * Matrix.M[1][0] + Position.mZ * Matrix.M[2][0] + Matrix.M[3][0], Position.mX * Matrix.M[0][1] + Position.mY * Matrix.M[1][1] + Position.mZ * Matrix.M[2][1] + Matrix.M[3][1], Position.mX * Matrix.M[0][2] + Position.mY * Matrix.M[1][2] + Position.mZ * Matrix.M[2][2] + Matrix.M[3][2], Position.mX * Matrix.M[0][3] + Position.mY * Matrix.M[1][3] + Position.mZ * Matrix.M[2][3] + Matrix.M[3][3]};
    }
}

bool FOverlayTextRenderer::Initialize(ID3D11Device* Device) {
    Reset();

    UPipeline Pipeline{};

    if (Device == nullptr || !Pipeline.Initialize("./Content/Pipeline/OverlayText.json") || !mPipeline.Initialize(Device, Pipeline)) {
        Reset();
        return false;
    }

    D3D11_SAMPLER_DESC Description{};

    Description.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    Description.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    Description.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    Description.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    Description.MaxLOD = D3D11_FLOAT32_MAX;
    Description.ComparisonFunc = D3D11_COMPARISON_NEVER;

    if (FAILED(Device->CreateSamplerState(&Description, mSampler.GetAddressOf()))) {
        Reset();
        return false;
    }

    mDevice = Device;

    return true;
}

void FOverlayTextRenderer::BindAssetRegistry(const IAssetRegistry* Registry, IAssetRegistryMutator* Mutator) {
    if (mAssetRegistry == Registry && mAssetRegistryMutator == Mutator) {
        return;
    }

    mTextCache.clear();
    mAssetRegistry = Registry;
    mAssetRegistryMutator = Registry != nullptr ? Mutator : nullptr;
}

void FOverlayTextRenderer::BeginFrame(Uint64 FrameSerial) {
    mFrameSerial = FrameSerial;

    constexpr Uint64 MaximumUnusedFrames{120};

    for (auto& [Text, Entries] : mTextCache) {
        std::erase_if(Entries, [this](const FCachedText& Entry) {
            const UFont* Font{mAssetRegistry != nullptr ? mAssetRegistry->ResolveAsset<UFont>(Entry.mFontHandle) : nullptr};

            return Font == nullptr || Font->GetGuid() != Entry.mFontGuid || mFrameSerial < Entry.mLastUsedFrame || mFrameSerial - Entry.mLastUsedFrame > MaximumUnusedFrames;
        });
    }

    std::erase_if(mTextCache, [](const auto& Entry) {
        return Entry.second.empty();
    });
}

void FOverlayTextRenderer::Reset() {
    mPipeline.Reset();
    mSampler.Reset();
    mGlyphs.clear();
    mDraws.clear();
    mTextCache.clear();
    mAssetRegistry = nullptr;
    mAssetRegistryMutator = nullptr;
    mFrameSerial = 0;
    mDevice = nullptr;
}

const TArray<FTextVertex>* FOverlayTextRenderer::GetTextGeometry(const FOverlayTextProbe& Probe, const UFont& Font, FAssetHandle FontHandle) {
    if (mAssetRegistryMutator == nullptr || Probe.mText.empty() || !std::isfinite(Probe.mPixelHeight) || Probe.mPixelHeight <= 0.0f || !std::isfinite(Probe.mLetterSpacing) || !std::isfinite(Probe.mLineSpacing)) {
        return nullptr;
    }

    TArray<FCachedText>& Entries{mTextCache[Probe.mText]};
    auto Found{std::ranges::find_if(Entries, [&Probe, &Font, FontHandle](const FCachedText& Entry) {
        return Entry.mFontHandle == FontHandle && Entry.mFontGuid == Font.GetGuid() && Entry.mPixelHeight == Probe.mPixelHeight && Entry.mLetterSpacing == Probe.mLetterSpacing && Entry.mLineSpacing == Probe.mLineSpacing;
    })};
    const bool NeedsRebuild{Found == Entries.end() || Found->mFontRevision != Font.GetAtlasRevision()};

    if (Found == Entries.end()) {
        FCachedText Entry{};

        Entry.mFontHandle = FontHandle;
        Entry.mFontGuid = Font.GetGuid();
        Entry.mPixelHeight = Probe.mPixelHeight;
        Entry.mLetterSpacing = Probe.mLetterSpacing;
        Entry.mLineSpacing = Probe.mLineSpacing;
        Entries.push_back(std::move(Entry));
        Found = std::prev(Entries.end());
    }

    if (NeedsRebuild) {
        if (!BuildTextGeometry(Font, *mAssetRegistryMutator, FontHandle, Probe.mText, Probe.mPixelHeight, Probe.mLetterSpacing, Probe.mLineSpacing, Found->mVertices)) {
            Entries.erase(Found);
            return nullptr;
        }

        Found->mFontRevision = Font.GetAtlasRevision();
    }

    Found->mLastUsedFrame = mFrameSerial;

    return &Found->mVertices;
}

bool FOverlayTextRenderer::ProjectAnchor(const FOverlayTextProbe& Probe, const TArray<FTextVertex>& Vertices, const CameraProbe& Camera, const D3D11_VIEWPORT& Viewport, FVector2& Position) const {
    const FVector4 Clip{TransformClip(Probe.mWorldAnchor, Camera.mViewProjection)};

    if (!std::isfinite(Clip.mX) || !std::isfinite(Clip.mY) || !std::isfinite(Clip.mZ) || !std::isfinite(Clip.mW) || Clip.mW <= 0.00001f || Clip.mZ < 0.0f || Clip.mZ > Clip.mW || Viewport.Width <= 0.0f || Viewport.Height <= 0.0f) {
        return false;
    }

    Position = FVector2{(Clip.mX / Clip.mW + 1.0f) * 0.5f * Viewport.Width, (1.0f - Clip.mY / Clip.mW) * 0.5f * Viewport.Height};

    float Top{Position.mY};

    for (Uint32 Index{}; Index < 8; ++Index) {
        const FVector3 Corner{Probe.mWorldAnchor + FVector3{(Index & 1u) != 0 ? Probe.mWorldBoundsExtent.mX : -Probe.mWorldBoundsExtent.mX, (Index & 2u) != 0 ? Probe.mWorldBoundsExtent.mY : -Probe.mWorldBoundsExtent.mY, (Index & 4u) != 0 ? Probe.mWorldBoundsExtent.mZ : -Probe.mWorldBoundsExtent.mZ}};
        const FVector4 Bound{TransformClip(Corner, Camera.mViewProjection)};

        if (std::isfinite(Bound.mY) && std::isfinite(Bound.mW) && Bound.mW > 0.00001f && Bound.mZ >= 0.0f && Bound.mZ <= Bound.mW) {
            Top = std::min(Top, (1.0f - Bound.mY / Bound.mW) * 0.5f * Viewport.Height);
        }
    }

    float HalfHeight{};

    for (const FTextVertex& Glyph : Vertices) {
        HalfHeight = std::max(HalfHeight, Glyph.mLocalPosition.mY);
    }

    Position.mX += Probe.mScreenOffset.mX;
    Position.mY = Top + Probe.mScreenOffset.mY - HalfHeight;

    return std::isfinite(Position.mX) && std::isfinite(Position.mY);
}

void FOverlayTextRenderer::Render(ID3D11DeviceContext* Context, FFrameResource& FrameResource, const CameraProbe& Camera, const D3D11_VIEWPORT& Viewport, const TArray<FOverlayTextProbe>& Probes, FRenderAssetResources& Resources) {
    mGlyphs.clear();
    mDraws.clear();

    if (Context == nullptr || mDevice == nullptr || mAssetRegistry == nullptr || mAssetRegistryMutator == nullptr || !FrameResource.BindCommon(Context)) {
        return;
    }

    constexpr std::size_t MaximumGlyphs{UINT32_MAX / sizeof(FGlyphInstance)};

    for (const FOverlayTextProbe& Probe : Probes) {
        FVector2 Anchor{};
        const FAssetHandle FontHandle{Probe.mFontHandle ? Probe.mFontHandle : mAssetRegistry->FindAsset(FAssetPath{"/Game/Font/NotoSansKR-Medium.ttf"})};
        const UFont* Font{mAssetRegistry->ResolveAsset<UFont>(FontHandle)};

        if (Font == nullptr) {
            continue;
        }

        const TArray<FTextVertex>* Vertices{GetTextGeometry(Probe, *Font, FontHandle)};

        if (Vertices == nullptr || Vertices->empty() || !ProjectAnchor(Probe, *Vertices, Camera, Viewport, Anchor)) {
            continue;
        }

        ID3D11ShaderResourceView* Atlas{Resources.GetFontAtlas(*Font, Context)};

        if (Atlas == nullptr || Vertices->size() > MaximumGlyphs - mGlyphs.size()) {
            continue;
        }

        const Uint32 First{static_cast<Uint32>(mGlyphs.size())};

        for (const FTextVertex& Source : *Vertices) {
            FTextVertex Glyph{Source};

            Glyph.mLocalPosition = FVector2{Anchor.mX + Source.mLocalPosition.mX, Anchor.mY - Source.mLocalPosition.mY};

            if (Glyph.mSize.mX > 0.0f && Glyph.mSize.mY > 0.0f && Glyph.mLocalPosition.mX < Viewport.Width && Glyph.mLocalPosition.mY < Viewport.Height && Glyph.mLocalPosition.mX + Glyph.mSize.mX > 0.0f && Glyph.mLocalPosition.mY + Glyph.mSize.mY > 0.0f) {
                mGlyphs.push_back(FGlyphInstance{Glyph, Probe.mColor});
            }
        }

        if (mGlyphs.size() != First) {
            mDraws.push_back(FTextDraw{Atlas, First, static_cast<Uint32>(mGlyphs.size()) - First});
        }
    }

    if (mGlyphs.empty() || !FrameResource.UploadStream(mDevice, Context, EFrameStream::OverlayText, mGlyphs.data(), static_cast<Uint32>(mGlyphs.size()), sizeof(FGlyphInstance), D3D11_BIND_VERTEX_BUFFER)) {
        return;
    }

    ID3D11Buffer* Buffer{FrameResource.GetStreamBuffer(EFrameStream::OverlayText)};
    const UINT Stride{sizeof(FGlyphInstance)};
    const UINT Offset{};

    mPipeline.Bind(Context, ERenderMode::Lit);
    Context->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
    Context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    Context->PSSetSamplers(0, 1, mSampler.GetAddressOf());

    for (const FTextDraw& Draw : mDraws) {
        Context->PSSetShaderResources(3, 1, &Draw.mAtlas);
        Context->DrawInstanced(6, Draw.mGlyphCount, 0, Draw.mFirstGlyph);
    }

    ID3D11ShaderResourceView* NullResource{nullptr};

    Context->PSSetShaderResources(3, 1, &NullResource);
}
