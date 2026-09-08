#include "PCH.h"
#include "FTransformGizmo.h"
#include "../../Core/Asset/UColorMaterial.h"
#include "../../Core/Asset/BasicGeometry/Cylinder.h"
#include "../../Core/Asset/BasicGeometry/Corn.h"

void FTransformGizmo::Initialize(ID3D11Device* Device, FAssetRegistry& AssetRegistry) {
    CylinderMesh    = AssetRegistry.EmplaceAsset<UMesh>(Device,				"CylinderMesh", "./Content/Metadata/CylinderMesh.meta");
    ConeMesh        = AssetRegistry.EmplaceAsset<UMesh>(Device,				"ConeMesh", "./Content/Metadata/ConeMesh.meta");

    RedMaterial     = AssetRegistry.EmplaceAsset<UColorMaterial>(Device,	"Red",		"./Content/Metadata/RedMaterial.meta");
    GreenMaterial   = AssetRegistry.EmplaceAsset<UColorMaterial>(Device,	"Green",	"./Content/Metadata/GreenMaterial.meta");
    BlueMaterial    = AssetRegistry.EmplaceAsset<UColorMaterial>(Device,	"Blue",		"./Content/Metadata/BlueMaterial.meta");

	GizmoPipeline   = AssetRegistry.EmplaceAsset<UPipeline>(Device,			"GizmoPipeline", "./Content/Metadata/BasePipeline.meta");
}

void FTransformGizmo::SetArrow(const FVector3& TargetExtent) {
    float ExtentX = TargetExtent.x;
    float ExtentY = TargetExtent.y;
    float ExtentZ = TargetExtent.z;

    const float ArrowLength = 1.1f;
    const float HalfArrowLength = ArrowLength * 0.5f;
	constexpr float ShaftWidth = 0.2f;

    const float ConeLength = 0.4f;
    const float HalfConeLength = ConeLength * 0.5f;
    const float ConeWidth = 0.4f;

	
	CylinderXAxisTransform = FMatrix::CreateScale(ShaftWidth, ArrowLength, ShaftWidth) * FMatrix::CreateRotationZ(DirectX::XMConvertToRadians(-90.0f)) * FMatrix::CreateTranslation(ExtentX + HalfArrowLength, 0.f, 0.f);
    CylinderYAxisTransform = FMatrix::CreateScale(ShaftWidth, ArrowLength, ShaftWidth) * FMatrix::CreateTranslation(0.f, ExtentY + HalfArrowLength, 0.f);
    CylinderZAxisTransform = FMatrix::CreateScale(ShaftWidth, ArrowLength, ShaftWidth) * FMatrix::CreateRotationX(DirectX::XMConvertToRadians(90.0f)) * FMatrix::CreateTranslation(0.f, 0.f, ExtentZ + HalfArrowLength);

    ConeXAxisTransform = FMatrix::CreateScale(ConeWidth, ConeLength, ConeWidth) * FMatrix::CreateRotationZ(DirectX::XMConvertToRadians(-90.0f)) * FMatrix::CreateTranslation(ExtentX + ArrowLength + HalfConeLength, 0.f, 0.f);
    ConeYAxisTransform = FMatrix::CreateScale(ConeWidth, ConeLength, ConeWidth) * FMatrix::CreateTranslation(0.f, ExtentY + ArrowLength + HalfConeLength, 0.f);
    ConeZAxisTransform = FMatrix::CreateScale(ConeWidth, ConeLength, ConeWidth) * FMatrix::CreateRotationX(DirectX::XMConvertToRadians(90.0f)) * FMatrix::CreateTranslation(0.f, 0.f, ExtentZ + ArrowLength + HalfConeLength);
}

void FTransformGizmo::SetGizmoWorldTransform(FMatrix& TargetWorld, const FVector3& TargetExtent) {
    FVector3 Scale{};
    FQuat Rotation{};
    FVector3 Translation{};

    TargetWorld.Decompose(Scale, Rotation, Translation);

    const FMatrix TargetRotation = FMatrix::CreateFromQuaternion(Rotation);
    const FMatrix TargetTranslation = FMatrix::CreateTranslation(Translation);

	GizmoWorldTransform = TargetRotation * TargetTranslation;

	const FVector3 GizmoExtent = TargetExtent * Scale;
	SetArrow(GizmoExtent);
}



void FTransformGizmo::Render(FRenderProbe& Probe) {

    const FMatrix CylinderXWorld = CylinderXAxisTransform * GizmoWorldTransform;
    const FMatrix CylinderYWorld = CylinderYAxisTransform * GizmoWorldTransform;
    const FMatrix CylinderZWorld = CylinderZAxisTransform * GizmoWorldTransform;

	FActorProbe CylinderXActorProbe{
		.World = CylinderXWorld,
		.MeshHandle = CylinderMesh,
		.MaterialHandle = RedMaterial,
		.PipelineHandle = GizmoPipeline
    };

	FActorProbe CylinderYActorProbe{
		.World = CylinderYWorld,
		.MeshHandle = CylinderMesh,
		.MaterialHandle = GreenMaterial,
		.PipelineHandle = GizmoPipeline
	};

	FActorProbe CylinderZActorProbe{
		.World = CylinderZWorld,
		.MeshHandle = CylinderMesh,
		.MaterialHandle = BlueMaterial,
		.PipelineHandle = GizmoPipeline
	};


	Probe.ActorProbes.emplace_back(CylinderXActorProbe);
	Probe.ActorProbes.emplace_back(CylinderYActorProbe);
	Probe.ActorProbes.emplace_back(CylinderZActorProbe);

	const FMatrix ConeXWorld = ConeXAxisTransform * GizmoWorldTransform;
	const FMatrix ConeYWorld = ConeYAxisTransform * GizmoWorldTransform;
	const FMatrix ConeZWorld = ConeZAxisTransform * GizmoWorldTransform;
    
	FActorProbe ConeXActorProbe{
		.World = ConeXWorld,
		.MeshHandle = ConeMesh,
		.MaterialHandle = RedMaterial,
		.PipelineHandle = GizmoPipeline
	};

	FActorProbe ConeYActorProbe{
		.World = ConeYWorld,
		.MeshHandle = ConeMesh,
		.MaterialHandle = GreenMaterial,
		.PipelineHandle = GizmoPipeline
	};

	FActorProbe ConeZActorProbe{
		.World = ConeZWorld,
		.MeshHandle = ConeMesh,
		.MaterialHandle = BlueMaterial,
		.PipelineHandle = GizmoPipeline
	};

	Probe.ActorProbes.emplace_back(ConeXActorProbe);
	Probe.ActorProbes.emplace_back(ConeYActorProbe);
	Probe.ActorProbes.emplace_back(ConeZActorProbe);
}
