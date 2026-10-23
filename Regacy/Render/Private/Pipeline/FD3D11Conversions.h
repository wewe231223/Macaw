#pragma once

#include <d3d11.h>
#include "RenderCore/Pipeline/FPipelineDescription.h"

DXGI_FORMAT ConvertVertexFormat(EVertexFormat Value);
D3D11_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(EPrimitiveTopology Value);
D3D11_FILL_MODE ConvertFillMode(EFillMode Value);
D3D11_CULL_MODE ConvertCullMode(ECullMode Value);
D3D11_COMPARISON_FUNC ConvertCompareFunc(ECompareFunc Value);
D3D11_STENCIL_OP ConvertStencillOp(EStencillOp Value);
D3D11_BLEND ConvertBlend(EBlend Value);
D3D11_BLEND_OP ConvertBlendOp(EBlendOp Value);
