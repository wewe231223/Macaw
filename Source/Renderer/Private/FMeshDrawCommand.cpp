#include "FMeshDrawCommand.h"

namespace Renderer {

    Renderer::FMeshDrawCommand::FMeshDrawCommand(Renderer::FMaterialPipeline* Pipeline, Asset::UMaterial* Material, Asset::UStaticMesh* Mesh, const Renderer::FScenePrimitive* Primitive, Core::Uint32 FirstIndex, Core::Uint32 IndexCount)
		: mPipeline{Pipeline},
		  mMaterial{Material},
		  mMesh{Mesh},
		  mPrimitive{Primitive},
		  mFirstIndex{FirstIndex},
		  mIndexCount{IndexCount} {
    }

}
