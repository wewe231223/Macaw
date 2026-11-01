#include "FMeshDataValidation.h"
#include "Asset/FMeshData.h"
#include "Core/FilePath.h"

#include <algorithm>
#include <d3d11.h>
#include <limits>

namespace Asset {

    bool Asset::ValidateMeshData(Asset::FMeshData& MeshData) {
        const std::size_t VertexCount{MeshData.GetPositions().size()};
        const std::size_t ChannelCount{std::max<std::size_t>(1, MeshData.GetUVChannels().size())};
        constexpr std::size_t MaximumBytes{std::numeric_limits<Core::Uint32>::max()};

        if (VertexCount == 0 || MeshData.GetIndices().empty() || MeshData.GetIndices().size() % 3 != 0 || VertexCount > MaximumBytes / (sizeof(float) * 7) || MeshData.GetIndices().size() > MaximumBytes / sizeof(Core::Uint32) || ChannelCount > D3D11_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES / (sizeof(float) * 2) || VertexCount > MaximumBytes / (ChannelCount * sizeof(float) * 2)) {
            return false;
        }

        if ((!MeshData.GetNormals().empty() && MeshData.GetNormals().size() != VertexCount) || (!MeshData.GetTangents().empty() && MeshData.GetTangents().size() != VertexCount) || (!MeshData.GetColors().empty() && MeshData.GetColors().size() != VertexCount)) {
            return false;
        }

        for (Core::Uint32 Index : MeshData.GetIndices()) {
            if (Index >= VertexCount) {
                return false;
            }
        }

        for (const Core::FVector& Normal : MeshData.GetNormals()) {
            if (!Normal.IsFinite()) {
                return false;
            }
        }

        for (const Core::FVector4& Tangent : MeshData.GetTangents()) {
            if (!Tangent.IsFinite()) {
                return false;
            }
        }

        for (const Core::FVector4& Color : MeshData.GetColors()) {
            if (!Color.IsFinite()) {
                return false;
            }
        }

        for (const Core::TArray<Core::FVector2>& Channel : MeshData.GetUVChannels()) {
            if (Channel.size() != VertexCount) {
                return false;
            }

            for (const Core::FVector2& UV : Channel) {
                if (!UV.IsFinite()) {
                    return false;
                }
            }
        }

        Core::FVector Minimum{MeshData.GetPositions().front()};
        Core::FVector Maximum{Minimum};

        for (const Core::FVector& Position : MeshData.GetPositions()) {
            if (!Position.IsFinite()) {
                return false;
            }

            Minimum = Minimum.Min(Position);
            Maximum = Maximum.Max(Position);
        }

        if (MeshData.GetMaterialSlots().empty()) {
            MeshData.GetMaterialSlots().emplace_back("Default", Core::FString{});
        }

        if (MeshData.GetSubMeshes().empty()) {
            MeshData.GetSubMeshes().emplace_back(0, static_cast<Core::Uint32>(MeshData.GetIndices().size()), 0);
        }

        Core::Uint32 NextIndex{};

        for (const Asset::FSubMesh& SubMesh : MeshData.GetSubMeshes()) {
            if (SubMesh.GetFirstIndex() != NextIndex || SubMesh.GetIndexCount() == 0 || SubMesh.GetIndexCount() % 3 != 0 || SubMesh.GetIndexCount() > MeshData.GetIndices().size() - NextIndex || SubMesh.GetMaterialIndex() >= MeshData.GetMaterialSlots().size()) {
                return false;
            }

            NextIndex += SubMesh.GetIndexCount();
        }

        if (NextIndex != MeshData.GetIndices().size()) {
            return false;
        }

        for (const Asset::FMeshMaterialSlot& Slot : MeshData.GetMaterialSlots()) {
            const std::filesystem::path Path{Core::FilePath::FromUtf8(Slot.GetMaterialPath())};

            if (Slot.GetMaterialPath().find('\0') != Core::FString::npos || (!Path.empty() && (Path.has_root_path() || Path.extension() != L".masset"))) {
                return false;
            }

            for (const auto& Part : Path) {
                if (Part == L"..") {
                    return false;
                }
            }
        }

        MeshData.SetBounds({Minimum, Maximum});
        return true;
    }

}
