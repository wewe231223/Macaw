#include "pch.h"
#include "Pipeline/FD3D11Conversions.h"
#include "Core/Base/ErrorHandler.h"

DXGI_FORMAT ConvertVertexFormat(EVertexFormat Value) {
    switch (Value) {
        case EVertexFormat::Float2:
            return DXGI_FORMAT_R32G32_FLOAT;
        case EVertexFormat::Float3:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case EVertexFormat::Float4:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case EVertexFormat::Uint:
            return DXGI_FORMAT_R32_UINT;
        default:
            ErrorHandler::Report("ConvertVertexFormat", "The vertex format is invalid.", ErrorHandler::EErrorLevel::Error);
            return DXGI_FORMAT_UNKNOWN;
    }
}

D3D11_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(EPrimitiveTopology Value) {
    switch (Value) {
        case EPrimitiveTopology::PointList:
            return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        case EPrimitiveTopology::LineList:
            return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case EPrimitiveTopology::LineStrip:
            return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case EPrimitiveTopology::TriangleList:
            return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case EPrimitiveTopology::TriangleStrip:
            return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        default:
            ErrorHandler::Report("ConvertPrimitiveTopology", "The primitive topology is invalid.", ErrorHandler::EErrorLevel::Error);
            return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

D3D11_FILL_MODE ConvertFillMode(EFillMode Value) {
    switch (Value) {
        case EFillMode::Solid:
            return D3D11_FILL_SOLID;
        case EFillMode::Wireframe:
            return D3D11_FILL_WIREFRAME;
        default:
            ErrorHandler::Report("ConvertFillMode", "The fill mode is invalid.", ErrorHandler::EErrorLevel::Error);
            return D3D11_FILL_SOLID;
    }
}

D3D11_CULL_MODE ConvertCullMode(ECullMode Value) {
    switch (Value) {
        case ECullMode::None:
            return D3D11_CULL_NONE;
        case ECullMode::Front:
            return D3D11_CULL_FRONT;
        case ECullMode::Back:
            return D3D11_CULL_BACK;
        default:
            ErrorHandler::Report("ConvertCullMode", "The cull mode is invalid.", ErrorHandler::EErrorLevel::Error);
            return D3D11_CULL_BACK;
    }
}

D3D11_COMPARISON_FUNC ConvertCompareFunc(ECompareFunc Value) {
    switch (Value) {
        case ECompareFunc::Never:
            return D3D11_COMPARISON_NEVER;
        case ECompareFunc::Less:
            return D3D11_COMPARISON_LESS;
        case ECompareFunc::Equal:
            return D3D11_COMPARISON_EQUAL;
        case ECompareFunc::LessEqual:
            return D3D11_COMPARISON_LESS_EQUAL;
        case ECompareFunc::Greater:
            return D3D11_COMPARISON_GREATER;
        case ECompareFunc::NotEqual:
            return D3D11_COMPARISON_NOT_EQUAL;
        case ECompareFunc::GreaterEqual:
            return D3D11_COMPARISON_GREATER_EQUAL;
        case ECompareFunc::Always:
            return D3D11_COMPARISON_ALWAYS;
        default:
            ErrorHandler::Report("ConvertCompareFunc", "The comparison function is invalid.", ErrorHandler::EErrorLevel::Error);
            return D3D11_COMPARISON_ALWAYS;
    }
}

D3D11_STENCIL_OP ConvertStencillOp(EStencillOp Value) {
    switch (Value) {
        case EStencillOp::Keep:
            return D3D11_STENCIL_OP_KEEP;
        case EStencillOp::Zero:
            return D3D11_STENCIL_OP_ZERO;
        case EStencillOp::Replace:
            return D3D11_STENCIL_OP_REPLACE;
        case EStencillOp::IncrementClamp:
            return D3D11_STENCIL_OP_INCR_SAT;
        case EStencillOp::IncrementWrap:
            return D3D11_STENCIL_OP_INCR;
        case EStencillOp::DecrementClamp:
            return D3D11_STENCIL_OP_DECR_SAT;
        case EStencillOp::DecrementWrap:
            return D3D11_STENCIL_OP_DECR;
        case EStencillOp::Invert:
            return D3D11_STENCIL_OP_INVERT;
        default:
            ErrorHandler::Report("ConvertCompareFunc", "The comparison function is invalid.", ErrorHandler::EErrorLevel::Error);
            return D3D11_STENCIL_OP_KEEP;
    }
}

D3D11_BLEND ConvertBlend(EBlend Value) {
    switch (Value) {
        case EBlend::Zero:
            return D3D11_BLEND_ZERO;
        case EBlend::One:
            return D3D11_BLEND_ONE;
        case EBlend::SrcAlpha:
            return D3D11_BLEND_SRC_ALPHA;
        case EBlend::InvSrcAlpha:
            return D3D11_BLEND_INV_SRC_ALPHA;
        case EBlend::DestAlpha:
            return D3D11_BLEND_DEST_ALPHA;
        case EBlend::InvDestAlpha:
            return D3D11_BLEND_INV_DEST_ALPHA;
        case EBlend::SrcColor:
            return D3D11_BLEND_SRC_COLOR;
        case EBlend::InvSrcColor:
            return D3D11_BLEND_INV_SRC_COLOR;
        case EBlend::DestColor:
            return D3D11_BLEND_DEST_COLOR;
        case EBlend::InvDestColor:
            return D3D11_BLEND_INV_DEST_COLOR;
        default:
            ErrorHandler::Report("ConvertBlend", "The blend value is invalid.", ErrorHandler::EErrorLevel::Error);
            return D3D11_BLEND_ONE;
    }
}

D3D11_BLEND_OP ConvertBlendOp(EBlendOp Value) {
    switch (Value) {
        case EBlendOp::Add:
            return D3D11_BLEND_OP_ADD;
        case EBlendOp::Subtract:
            return D3D11_BLEND_OP_SUBTRACT;
        case EBlendOp::RevSubtract:
            return D3D11_BLEND_OP_REV_SUBTRACT;
        case EBlendOp::Min:
            return D3D11_BLEND_OP_MIN;
        case EBlendOp::Max:
            return D3D11_BLEND_OP_MAX;
        default:
            ErrorHandler::Report("ConvertBlendOp", "The blend operation is invalid.", ErrorHandler::EErrorLevel::Error);
            return D3D11_BLEND_OP_ADD;
    }
}
