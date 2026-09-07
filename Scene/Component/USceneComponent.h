#pragma once

#include "Core/Base/FTransform.h"
#include "UActorComponent.h"

class FArchive;
class USceneComponent : public UActorComponent
{
public:
	USceneComponent() = default;
	~USceneComponent() override = default;

	FTransform& GetTransform();
	const FTransform& GetTransform() const;

	JG_DECLARE_DERIVED_TYPEINFO(USceneComponent, UActorComponent);
protected:
	void Serialize(FArchive& Archive) override;

private:
	FTransform Transform;
};