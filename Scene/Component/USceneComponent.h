#pragma once

#include "Core/Base/FTransform.h"
#include "UActorComponent.h"

class USceneComponent : public UActorComponent
{
public:
	USceneComponent() = default;
	~USceneComponent() override = default;

	JG_DECLARE_DERIVED_TYPEINFO(USceneComponent, UActorComponent)

	FTransform& GetTransform();
	const FTransform& GetTransform() const;

protected:
	void Serialize(FArchive& Archive) override;

private:
	FTransform Transform;
};
