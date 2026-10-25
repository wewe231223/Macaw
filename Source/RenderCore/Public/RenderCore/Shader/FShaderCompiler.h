#pragma once

#include "RenderCore/Shader/FShaderCompilation.h"
#include "RenderCore/Shader/FShaderDescription.h"

class FShaderCompiler {
public:
    ReflectTypes(FShaderCompiler);

public:
    ReflectStruct(FShaderCompiler);
    bool Compile(const FShaderDescription& Description, FShaderCompilation& Compilation, FString& Diagnostics) const;

private:
    ReflectMembers();
};
