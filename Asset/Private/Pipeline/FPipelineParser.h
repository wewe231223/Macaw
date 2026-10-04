#pragma once
#include "RenderCore/Pipeline/FPipelineDescription.h"
#include "Core/Base/ErrorHandler.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

#include <cstdio>
#include <string>

const rapidjson::Value* GetObject(const rapidjson::Value& Parent, const char* Name);

const rapidjson::Value* GetArray(const rapidjson::Value& Parent, const char* Name);

const char* GetString(const rapidjson::Value& Parent, const char* Name);

const char* GetString(const rapidjson::Value& Parent, const char* Name, const char* DefaultValue);

bool GetBool(const rapidjson::Value& Parent, const char* Name, bool DefaultValue);

unsigned int GetUint(const rapidjson::Value& Parent, const char* Name, unsigned int DefaultValue);

unsigned int GetUint(const rapidjson::Value& Parent, const char* Name);

EVertexFormat ParseVertexFormat(const char* Value);

EPrimitiveTopology ParsePrimitiveTopology(const char* Value);

EFillMode ParseFillMode(const char* Value);

ECullMode ParseCullMode(const char* Value);

ECompareFunc ParseCompareFunc(const char* Value);

EBlend ParseBlend(const char* Value);

EBlendOp ParseBlendOp(const char* Value);

