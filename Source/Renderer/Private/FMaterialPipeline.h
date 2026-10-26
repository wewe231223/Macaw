#pragma once

#include "RenderCore/Shader/FShader.h"

#include <d3d11.h>
#include <wrl/client.h>

class FMaterialPipeline {
public:
    ReflectTypes(FMaterialPipeline);

public:
    FMaterialPipeline(std::shared_ptr<const FShader> VertexCode, std::shared_ptr<const FShader> PixelCode, bool TwoSided);
    ~FMaterialPipeline();

public:
    ReflectStruct(FMaterialPipeline);
    bool IsCurrent() const;
    bool HasResources() const;
    bool CreateResources(ID3D11Device* Device, FString& Error);
    void Bind(ID3D11DeviceContext* Context) const;

private:
    ReflectMembers();

private:
    std::shared_ptr<const FShader> mVertexCode{};
    ReflectProperty(mVertexCode, "VertexCode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::shared_ptr<const FShader> mPixelCode{};
    ReflectProperty(mPixelCode, "PixelCode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mTwoSided{};
    ReflectProperty(mTwoSided, "TwoSided", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mVertexRevision{};
    ReflectProperty(mVertexRevision, "VertexRevision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mPixelRevision{};
    ReflectProperty(mPixelRevision, "PixelRevision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 메시용 정점 셰이더입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};
    ReflectProperty(mVertexShader, "VertexShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 메시용 픽셀 셰이더입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
    ReflectProperty(mPixelShader, "PixelShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 메시 정점 입력 레이아웃입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout{};
    ReflectProperty(mInputLayout, "InputLayout", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizer{};
    ReflectProperty(mRasterizer, "Rasterizer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
