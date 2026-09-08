#pragma once 
#include <d3d11.h>
#include "../../Core/Asset/FAssetRegistry.h"
#include "../../Core/Asset/UMesh.h"
#include "../../Core/Asset/UMaterial.h"
#include "../Pipeline/UPipeline.h"
#include "../../Core/Base/FRenderProbe.h"

class FTransformGizmo {
public:
	FTransformGizmo() = default;
	~FTransformGizmo() = default;

	FTransformGizmo(const FTransformGizmo&) = delete;
	FTransformGizmo& operator=(const FTransformGizmo&) = delete;

	FTransformGizmo(FTransformGizmo&&) = default;
	FTransformGizmo& operator=(FTransformGizmo&&) = default;

public:
	void Initialize(ID3D11Device* Device, FAssetRegistry& AssetRegistry);

	// box 로 cyliner 의 y scale 을 정한다. cyliner 는 기본 3개로 각각 +1 만큼 y up 한 다음, box 의 각 축 길이만큼 y scale 을 정한다. 그 다음 마지막으로 표현하고자 하는 각 축의 방향으로 회전시킨다. 
	// y 축을 표현하고자 하는 경우는 제외하고, x 축을 표현하고자 하는 경우 z 축을 기준으로 90 도 회전, z 축을 표현하고자 하는 경우 x 축을 기준으로 -90 도 회전시킨다. -> 이것을 결합하여 각 축의 cyliner 의 기본 변환으로 한다.  
	void SetArrow(const FVector3& TargetExtent);

	void SetGizmoWorldTransform(FMatrix& TargetWorld, const FVector3& TargetExtent);

	// 각 cyliner 의 변환을 먼저 하고, 그 다음, 타겟의 월드 변환 중 회전만 추출하여 적용한다. 
	void Render(FRenderProbe& Probe);

private:
	FAssetHandle CylinderMesh{};
	FAssetHandle ConeMesh{};

	FAssetHandle RedMaterial{};
	FAssetHandle GreenMaterial{};
	FAssetHandle BlueMaterial{};

	FAssetHandle GizmoPipeline{};

	FMatrix CylinderXAxisTransform{ FMatrix::Identity };
	FMatrix CylinderYAxisTransform{ FMatrix::Identity };
	FMatrix CylinderZAxisTransform{ FMatrix::Identity };

	FMatrix ConeXAxisTransform{ FMatrix::Identity };
	FMatrix ConeYAxisTransform{ FMatrix::Identity };
	FMatrix ConeZAxisTransform{ FMatrix::Identity };

	FMatrix GizmoWorldTransform{ FMatrix::Identity };
};