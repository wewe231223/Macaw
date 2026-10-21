#pragma once
#include "Asset/FObjInfo.h"
#include "Asset/FSurfaceParameters.h"
#include "Asset/FMeshData.h"
#include <functional>

struct FImportedMaterial {
    FName mName{};
    FSurfaceParameters mParameters{};
};

class FObjImporter {
public:
    using FTextureResolver = std::function<FAssetReference(const std::filesystem::path&)>;
    using FMaterialResolver = std::function<FAssetReference(const std::filesystem::path&, FName)>;

public:
    FObjImporter() = default;
    ~FObjImporter() = default;

    FObjImporter(const FObjImporter&) = delete;
    FObjImporter& operator=(const FObjImporter&) = delete;

    FObjImporter(FObjImporter&&) noexcept = default;
    FObjImporter& operator=(FObjImporter&&) noexcept = default;

public:
    static bool LoadMaterialFile(const std::filesystem::path& MtlPath, const FTextureResolver& TextureResolver, TArray<FImportedMaterial>& OutMaterials);
    bool ImportMesh(const std::filesystem::path& SourcePath, bool FlipUV, const FMaterialResolver& MaterialResolver, FMeshData& OutData);

private:
    //Obj 파일 로드
    bool LoadObjFile(const FString& FilePath, FGeometry& OutGeometry);
    //한줄 나누기
    static TArray<FString> SplitTokens(const FString& Line);

    //obj의 f값을 인덱스로 바꾸기 위해 1을 뺍니다.
    //음수값이라면 뒤에서부터 가져옵니다.
    Int32 NormalizeIndex(Int32 RawIndex, Int32 ArraySize) const;

private:
    //파싱된 ObjInfo로 Vertex Position, Index, Normal, TexCoord 배열을 만든다.
    bool BuildGeometry(const FObjInfo& ObjInfo, FGeometry& OutGeometry) const;

    //다각형이라면 계산을 다르게 처리
    bool BuildPolygonGeometry(const FObjInfo& ObjInfo, FGeometry& OutGeometry) const;

    //1개 버텍스 데이터 저장
    void AddPNTIArray(const FFaceVertex& TargetVertex, const FObjInfo& ObjInfo, FGeometry& OutGeometry, std::unordered_map<FFaceVertexKey, Uint32, FFaceVertexKeyHash>& CacheMap) const;

    //정점들의 Normal 평균 구하기
    FVector ComputeFaceNormal(const TArray<FFaceVertex>& PolygonVertices, const TArray<FVector>& Positions, Int32 PositionCount) const;

private:
    FVector mPositionCoordTransX{FVector{1.f, 0.f, 0.f}};
    FVector mPositionCoordTransY{FVector{0.f, 1.f, 0.f}};
    FVector mPositionCoordTransZ{FVector{0.f, 0.f, 1.f}};
};
