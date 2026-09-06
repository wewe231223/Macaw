#pragma once 
#include "../Asset/FAssetHandle.h"

struct ActorProbe {
	FMatrix World;
	FAssetHandle MeshHandle;
	FAssetHandle MaterialHandle;
	FAssetHandle PipelineHandle;
};

struct CameraProbe {
	FMatrix ViewProjection{}; 
	FMatrix View{};
	FMatrix Projection{};
};

struct FRenderProbe {
	TArray<ActorProbe> ActorProbes{};
	CameraProbe MainCameraProbe{}; 
};