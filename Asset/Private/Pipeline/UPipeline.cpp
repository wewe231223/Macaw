#include "pch.h"
#include "Asset/Pipeline/UPipeline.h"
#include "Core/Base/ErrorHandler.h"
#include "Pipeline/FPipelineParser.h"

#include <memory>

namespace {
    EStencillOp ParseStencilOperation(const char* Value) {
        constexpr std::array Names{"Keep", "Zero", "Replace", "IncrementClamp", "IncrementWrap", "DecrementClamp", "DecrementWrap", "Invert"};

        for (std::size_t Index{}; Index < Names.size(); ++Index) {
            if (std::strcmp(Value, Names[Index]) == 0) {
                return static_cast<EStencillOp>(Index);
            }
        }

        ErrorHandler::Report("ParseStencilOperation", "The stencil operation is invalid.", ErrorHandler::EErrorLevel::Error);

        return EStencillOp::Keep;
    }
}

bool UPipeline::Initialize(const std::filesystem::path& PipelinePath) {
    if (std::filesystem::is_directory(PipelinePath)) {
        return InitializeFamily(PipelinePath);
    }

    if (!UAsset::Initialize(PipelinePath)) {
        return false;
    }

    std::array<std::filesystem::path, static_cast<std::size_t>(ERenderMode::Max)> ModePaths{};

    ModePaths[static_cast<std::size_t>(ERenderMode::Lit)] = PipelinePath;

    return InitializeModes(ModePaths);
}

bool UPipeline::InitializeFamily(const std::filesystem::path& FamilyDirectory) {
    if (!UAsset::Initialize(FamilyDirectory) || !std::filesystem::is_directory(FamilyDirectory)) {
        return false;
    }

    constexpr std::array<const char*, static_cast<std::size_t>(ERenderMode::Max)> ModeNames{"Lit", "Outline", "Unlit", "Wireframe"};
    std::array<std::filesystem::path, static_cast<std::size_t>(ERenderMode::Max)> ModePaths{};
    const std::string FamilyName{FamilyDirectory.filename().generic_string()};

    for (std::size_t Index{0}; Index < ModeNames.size(); ++Index) {
        const std::filesystem::path Path{FamilyDirectory / (FamilyName + "_" + ModeNames[Index] + ".json")};

        if (std::filesystem::is_regular_file(Path)) {
            ModePaths[Index] = Path;
        }
    }

    if (ModePaths[static_cast<std::size_t>(ERenderMode::Lit)].empty()) {
        return false;
    }

    return InitializeModes(ModePaths);
}

bool UPipeline::InitializeModes(const std::array<std::filesystem::path, static_cast<std::size_t>(ERenderMode::Max)>& ModePaths) {
    std::array<FPipelineDescription, static_cast<std::size_t>(ERenderMode::Max)> Descriptions{};
    std::array<bool, static_cast<std::size_t>(ERenderMode::Max)> EnabledModes{};

    EnabledModes.fill(true);

    const std::size_t LitIndex{static_cast<std::size_t>(ERenderMode::Lit)};

    if (!LoadPipelineDescription(ModePaths[LitIndex], Descriptions[LitIndex])) {
        return false;
    }

    for (std::size_t Index{0}; Index < Descriptions.size(); ++Index) {
        if (Index == LitIndex) {
            continue;
        }

        if (!ModePaths[Index].empty()) {
            if (!LoadPipelineDescription(ModePaths[Index], Descriptions[Index])) {
                return false;
            }

            continue;
        }

        Descriptions[Index] = Descriptions[LitIndex];

        if (Descriptions[Index].mPrimitiveTopology != EPrimitiveTopology::TriangleList && Descriptions[Index].mPrimitiveTopology != EPrimitiveTopology::TriangleStrip) {
            continue;
        }

        if (Index == static_cast<std::size_t>(ERenderMode::Outline)) {
            const bool HasNormal{std::ranges::any_of(Descriptions[Index].mInputLayout, [](const FInputElementDescription& Element) {
                return Element.mSemanticName == "NORMAL" && Element.mSemanticIndex == 0;
            })};

            if (!HasNormal) {
                EnabledModes[Index] = false;
                continue;
            }

            Descriptions[Index].mGeometryShader.mSource = "./Content/Shader/Outline.hlsl";
            Descriptions[Index].mGeometryShader.mEntryPoint = "MainGS";
            Descriptions[Index].mGeometryShader.mProfile = "gs_5_0";
            Descriptions[Index].mGeometryShader.mStage = EShaderStage::Geometry;
            Descriptions[Index].mBHasGeometryShader = true;
            Descriptions[Index].mPixelShader.mSource = "./Content/Shader/Outline.hlsl";
            Descriptions[Index].mPixelShader.mEntryPoint = "MainPS";
            Descriptions[Index].mRasterizer.mFillMode = EFillMode::Solid;
            Descriptions[Index].mRasterizer.mCullMode = ECullMode::Front;
            Descriptions[Index].mDepthStencil.mDepthWriteEnable = false;
            Descriptions[Index].mDepthStencil.mStencilEnable = true;
            Descriptions[Index].mDepthStencil.mStencilReadMask = 1;
            Descriptions[Index].mDepthStencil.mStencilWriteMask = 0;
            Descriptions[Index].mDepthStencil.mStencilFunc = ECompareFunc::NotEqual;
            Descriptions[Index].mDepthStencil.mStencilPassOp = EStencillOp::Keep;
            Descriptions[Index].mDepthStencil.mStencilFailOp = EStencillOp::Keep;
            Descriptions[Index].mDepthStencil.mStencilDepthFailOp = EStencillOp::Keep;
        } else if (Index == static_cast<std::size_t>(ERenderMode::Wireframe)) {
            Descriptions[Index].mRasterizer.mFillMode = EFillMode::Wireframe;
        }
    }

    Reset();
    mDescriptions = std::move(Descriptions);
    mEnabledModes = EnabledModes;
    mOptionFilePath = ModePaths[LitIndex];
    mPrimaryIndex = LitIndex;
    mModeIndex = LitIndex;

    return true;
}

void UPipeline::Reset() {
    mDescriptions = {};
    mEnabledModes = {};
    mPrimaryIndex = 0;
    mModeIndex = 0;
    ++mRenderRevision;
}

void UPipeline::SetRenderMode(ERenderMode Mode) {
    const std::size_t RequestedIndex{static_cast<std::size_t>(Mode)};

    if (RequestedIndex < mDescriptions.size() && mEnabledModes[RequestedIndex]) {
        mModeIndex = RequestedIndex;
    } else {
        mModeIndex = mPrimaryIndex;
    }
}

bool UPipeline::RenderModeSettable(ERenderMode Mode) const {
    const std::size_t RequestedIndex{static_cast<std::size_t>(Mode)};

    return RequestedIndex < mDescriptions.size() && mEnabledModes[RequestedIndex];
}

ERenderMode UPipeline::GetRenderMode() const {
    return static_cast<ERenderMode>(mModeIndex);
}

bool UPipeline::LoadPipelineDescription(const std::filesystem::path& Path, FPipelineDescription& OutDescription) {
    FILE* File{nullptr};

#ifdef _WIN32
    _wfopen_s(&File, Path.c_str(), L"rb");
#else
    File = std::fopen(Path.string().c_str(), "rb");
#endif

    if (File == nullptr) {
        ErrorHandler::Report("Pipeline::LoadPipelineDescription", "Failed to open the pipeline option file.", ErrorHandler::EErrorLevel::Critical);
        return false;
    }

    std::unique_ptr<char[]> ReadBufferPtr{new char[65536]};
    rapidjson::FileReadStream Stream{File, ReadBufferPtr.get(), sizeof(ReadBufferPtr.get())};

    rapidjson::Document Root{};

    Root.ParseStream(Stream);

    std::fclose(File);

    if (Root.HasParseError() || !Root.IsObject()) {
        ErrorHandler::Report("Pipeline::LoadPipelineDescription", "Failed to parse the pipeline option file as a JSON object.", ErrorHandler::EErrorLevel::Error);
        return false;
    }

    FPipelineDescription Description{};

    const rapidjson::Value* VS{GetObject(Root, "VertexShader")};

    if (VS == nullptr) {
        return false;
    }

    const char* VSSource{GetString(*VS, "Source")};
    const char* VSEntryPoint{GetString(*VS, "EntryPoint")};
    const char* VSProfile{GetString(*VS, "Profile")};

    if (VSSource == nullptr || VSEntryPoint == nullptr || VSProfile == nullptr) {
        return false;
    }

    Description.mVertexShader.mSource = VSSource;
    Description.mVertexShader.mEntryPoint = VSEntryPoint;
    Description.mVertexShader.mProfile = VSProfile;
    Description.mVertexShader.mStage = EShaderStage::Vertex;

    const rapidjson::Value* PS{GetObject(Root, "PixelShader")};

    if (PS == nullptr) {
        return false;
    }

    const char* PSSource{GetString(*PS, "Source")};
    const char* PSEntryPoint{GetString(*PS, "EntryPoint")};
    const char* PSProfile{GetString(*PS, "Profile")};

    if (PSSource == nullptr || PSEntryPoint == nullptr || PSProfile == nullptr) {
        return false;
    }

    Description.mPixelShader.mSource = PSSource;
    Description.mPixelShader.mEntryPoint = PSEntryPoint;
    Description.mPixelShader.mProfile = PSProfile;
    Description.mPixelShader.mStage = EShaderStage::Pixel;

    if (Root.HasMember("GeometryShader")) {
        const rapidjson::Value& GS{Root["GeometryShader"]};

        if (!GS.IsObject()) {
            return false;
        }

        const char* GSSource{GetString(GS, "Source")};
        const char* GSEntryPoint{GetString(GS, "EntryPoint")};
        const char* GSProfile{GetString(GS, "Profile")};

        if (GSSource == nullptr || GSEntryPoint == nullptr || GSProfile == nullptr) {
            return false;
        }

        Description.mGeometryShader.mSource = GSSource;
        Description.mGeometryShader.mEntryPoint = GSEntryPoint;
        Description.mGeometryShader.mProfile = GSProfile;
        Description.mGeometryShader.mStage = EShaderStage::Geometry;
        Description.mBHasGeometryShader = true;
    }

    const rapidjson::Value* InputLayout{GetArray(Root, "InputLayout")};

    if (InputLayout == nullptr) {
        return false;
    }

    Description.mInputLayout.reserve(InputLayout->Size());

    for (const rapidjson::Value& Element : InputLayout->GetArray()) {
        if (!Element.IsObject()) {
            ErrorHandler::Report("Pipeline::LoadPipelineDescription", "Each input-layout element must be a JSON object.", ErrorHandler::EErrorLevel::Error);
            return false;
        }

        const char* SemanticName{GetString(Element, "SemanticName")};
        const char* Format{GetString(Element, "Format")};

        if (SemanticName == nullptr || Format == nullptr) {
            return false;
        }

        FInputElementDescription Input{};

        Input.mSemanticName = SemanticName;
        Input.mSemanticIndex = GetUint(Element, "SemanticIndex", 0);
        Input.mFormat = ParseVertexFormat(Format);
        Input.mInputSlot = GetUint(Element, "InputSlot", 0);
        Input.mAlignedByteOffset = GetUint(Element, "AlignedByteOffset");
        Input.mInputClassification = std::strcmp(GetString(Element, "InputClassification", "PerVertex"), "PerInstance") == 0 ? EInputClassification::PerInstance : EInputClassification::PerVertex;
        Input.mInstanceDataStepRate = GetUint(Element, "InstanceDataStepRate", 0);

        Description.mInputLayout.emplace_back(std::move(Input));
    }

    const char* PrimitiveTopology{GetString(Root, "PrimitiveTopology")};

    if (PrimitiveTopology == nullptr) {
        return false;
    }

    Description.mPrimitiveTopology = ParsePrimitiveTopology(PrimitiveTopology);

    const rapidjson::Value* Rasterizer{GetObject(Root, "Rasterizer")};

    if (Rasterizer == nullptr) {
        return false;
    }

    const char* FillMode{GetString(*Rasterizer, "FillMode")};
    const char* CullMode{GetString(*Rasterizer, "CullMode")};

    if (FillMode == nullptr || CullMode == nullptr) {
        return false;
    }

    Description.mRasterizer.mFillMode = ParseFillMode(FillMode);
    Description.mRasterizer.mCullMode = ParseCullMode(CullMode);
    Description.mRasterizer.mFrontCounterClockwise = GetBool(*Rasterizer, "FrontCounterClockwise", false);
    Description.mRasterizer.mDepthClipEnable = GetBool(*Rasterizer, "DepthClipEnable", true);
    Description.mRasterizer.mScissorEnable = GetBool(*Rasterizer, "ScissorEnable", false);

    const rapidjson::Value* DepthStencil{GetObject(Root, "DepthStencil")};

    if (DepthStencil == nullptr) {
        return false;
    }

    Description.mDepthStencil.mDepthEnable = GetBool(*DepthStencil, "DepthEnable", true);
    Description.mDepthStencil.mDepthWriteEnable = GetBool(*DepthStencil, "DepthWriteEnable", true);
    Description.mDepthStencil.mDepthFunc = ParseCompareFunc(GetString(*DepthStencil, "DepthFunc", "LessEqual"));
    Description.mDepthStencil.mStencilEnable = GetBool(*DepthStencil, "StencilEnable", true);
    Description.mDepthStencil.mStencilReadMask = static_cast<Uint8>(GetUint(*DepthStencil, "StencilReadMask", 255));
    Description.mDepthStencil.mStencilWriteMask = static_cast<Uint8>(GetUint(*DepthStencil, "StencilWriteMask", 255));
    Description.mDepthStencil.mStencilFunc = ParseCompareFunc(GetString(*DepthStencil, "StencilFunc", "Always"));
    Description.mDepthStencil.mStencilPassOp = ParseStencilOperation(GetString(*DepthStencil, "StencilPassOp", "Replace"));
    Description.mDepthStencil.mStencilFailOp = ParseStencilOperation(GetString(*DepthStencil, "StencilFailOp", "Keep"));
    Description.mDepthStencil.mStencilDepthFailOp = ParseStencilOperation(GetString(*DepthStencil, "StencilDepthFailOp", "Keep"));

    const rapidjson::Value* Blend{GetObject(Root, "Blend")};

    if (Blend == nullptr) {
        return false;
    }

    Description.mBlend.mBlendEnable = GetBool(*Blend, "BlendEnable", false);
    Description.mBlend.mSrcBlend = ParseBlend(GetString(*Blend, "SrcBlend", "One"));
    Description.mBlend.mDestBlend = ParseBlend(GetString(*Blend, "DestBlend", "Zero"));
    Description.mBlend.mBlendOp = ParseBlendOp(GetString(*Blend, "BlendOp", "Add"));
    Description.mBlend.mSrcBlendAlpha = ParseBlend(GetString(*Blend, "SrcBlendAlpha", "One"));
    Description.mBlend.mDestBlendAlpha = ParseBlend(GetString(*Blend, "DestBlendAlpha", "Zero"));
    Description.mBlend.mBlendOpAlpha = ParseBlendOp(GetString(*Blend, "BlendOpAlpha", "Add"));

    OutDescription = std::move(Description);

    return true;
}

void UPipeline::Serialize(FArchive& Ar) {
    UAsset::Serialize(Ar);
}

ERenderMode UPipeline::ResolveRenderMode(ERenderMode Mode) const {
    const std::size_t Index{static_cast<std::size_t>(Mode)};

    return Index < mDescriptions.size() && mEnabledModes[Index] ? Mode : static_cast<ERenderMode>(mPrimaryIndex);
}

const FPipelineDescription* UPipeline::GetDescription(ERenderMode Mode) const {
    const std::size_t Index{static_cast<std::size_t>(Mode)};

    return Index < mDescriptions.size() && mEnabledModes[Index] ? &mDescriptions[Index] : nullptr;
}

Uint64 UPipeline::GetRenderRevision() const {
    return mRenderRevision;
}
