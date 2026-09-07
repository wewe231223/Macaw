#pragma once

#include "Core/Base/FTransform.h"
#include "UActorComponent.h"

class USceneComponent : public UActorComponent
{
public:
	USceneComponent() = default;
	~USceneComponent() override = default;

	virtual void OnDestroy() override;
	void RemoveChild(USceneComponent* InChild);

	JG_DECLARE_DERIVED_TYPEINFO(USceneComponent, UActorComponent)

	FTransform& GetTransform();
	const FTransform& GetTransform() const;

	void AttachTo(USceneComponent* InParent);

	USceneComponent* GetParent() const;
	const std::vector<USceneComponent*>& GetChildren() const;

	FMatrix GetWorldMatrix() const;

protected:
	void Serialize(FArchive& Archive) override;

private:
	FTransform Transform;

	USceneComponent* Parent = nullptr;
	std::vector<USceneComponent*> Children;
};