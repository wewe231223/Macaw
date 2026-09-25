#pragma once

#include "Defines.h"

#include "../../ErrorHandler.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

#include <cstdio>
#include <string>

#include <dxgi1_6.h>
#include <d3d11.h>

const rapidjson::Value* GetObject(const rapidjson::Value& Parent, const char* Name);

const rapidjson::Value* GetArray(const rapidjson::Value& Parent, const char* Name);

const char* GetString(const rapidjson::Value& Parent, const char* Name);

const char* GetString(const rapidjson::Value& Parent, const char* Name, const char* DefaultValue);

bool GetBool(const rapidjson::Value& Parent, const char* Name, bool DefaultValue);

unsigned int GetUint(const rapidjson::Value& Parent, const char* Name, unsigned int DefaultValue);

UINT GetUint(const rapidjson::Value& Parent, const char* Name);

EVertexFormat ParseVertexFormat(const char* Value);

DXGI_FORMAT ConvertVertexFormat(EVertexFormat Value);

EPrimitiveTopology ParsePrimitiveTopology(const char* Value);

D3D11_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(EPrimitiveTopology Value);

EFillMode ParseFillMode(const char* Value);

D3D11_FILL_MODE ConvertFillMode(EFillMode Value);

ECullMode ParseCullMode(const char* Value);

D3D11_CULL_MODE ConvertCullMode(ECullMode Value);

ECompareFunc ParseCompareFunc(const char* Value);

D3D11_COMPARISON_FUNC ConvertCompareFunc(ECompareFunc Value);

D3D11_STENCIL_OP ConvertStencillOp(EStencillOp Value);

EBlend ParseBlend(const char* Value);

D3D11_BLEND ConvertBlend(EBlend Value);

EBlendOp ParseBlendOp(const char* Value);

D3D11_BLEND_OP ConvertBlendOp(EBlendOp Value);
