#pragma once

#include "Core/Base/FTransform.h"
#include "Core/Base/TObjectRef.h"
#include "UActorComponent.h"

class FArchive;
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

	JG_DECLARE_DERIVED_TYPEINFO(USceneComponent, UActorComponent);
protected:
	void Serialize(FArchive& Archive) override;

	void AttachTo(USceneComponent* InParent);

	USceneComponent* GetParent() const;
	const std::vector<TObjectRef<USceneComponent>>& GetChildren() const;

	FMatrix GetWorldMatrix() const;

protected:
	void Serialize(FArchive& Archive) override;

private:
	FTransform Transform;

	TObjectRef<USceneComponent> Parent;
	std::vector<TObjectRef<USceneComponent>> Children;
};
