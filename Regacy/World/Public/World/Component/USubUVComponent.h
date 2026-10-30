#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/UPrimitiveComponent.h"
#include "CoreUObject/TObjectRef.h"

class FProperty;

class USubUVComponent : public UActorComponent {
public:
    ReflectTypes(USubUVComponent, UActorComponent);

public:
    USubUVComponent();
    ~USubUVComponent() override = default;

public:
    ReflectClass(USubUVComponent, EClassFlags::None);

    bool SetTargetComponent(UPrimitiveComponent* Target);
    UPrimitiveComponent* GetTargetComponent() const;
    void SetTargetMaterialSlot(FName MaterialSlot);
    FName GetTargetMaterialSlot() const;
    bool HasTargetConflict() const;
    bool CanSelectTarget(UPrimitiveComponent* Target) const;
    bool CanEditUVTransform() const;
    TArray<FName> GetTargetMaterialSlots() const;
    void SetUVTransform(const FUVTransform& Transform);
    const FUVTransform& GetUVTransform() const;
    void UpdateUVTransform();
    ReflectFunction(Play);

    void Play();
    ReflectFunction(Pause);

    void Pause();
    ReflectFunction(Stop);

    void Stop();
    ReflectFunction(Restart);

    void Restart();
    ReflectFunction(IsPlaying);

    bool IsPlaying() const;
    ReflectFunction(SetLooping, FName{"Looping"});

    void SetLooping(bool Looping);
    ReflectFunction(IsLooping);

    bool IsLooping() const;
    void SetElapsedTime(double ElapsedTime);
    double GetElapsedTime() const;
    void OnRegister() override;
    void OnUnregister() override;
    void Activate() override;
    void TickComponent(float DeltaTime) override;
    bool ResolveLoadedReferences() override;
    void Serialize(FArchive& Archive) override;
    void UpdateTickEnabled();

private:
    static void SerializeTargetComponentProperty(void* Container, FArchive& Archive, const FProperty& Property);

    virtual FUVTransform CalculateUVTransform() const;
    virtual bool CanAdvance() const;
    virtual double GetDuration() const;
    virtual void ResetPlaybackState();

    void ApplyUVTransform();
    void ClearTargetTransform();

private:
    ReflectAccessor(mTargetComponent, GetTargetComponent, SetTargetComponent, "GuidTargetComponent", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Reference | EPropertyFlags::OwnerOnly, &SerializeTargetComponentProperty, ReflectEditor("UV Animation", "Target Component"), ReflectFilter(CanSelectTarget));

    TObjectRef<UPrimitiveComponent> mTargetComponent{};
    FGuid mPendingTargetGuid{};
    ReflectAccessor(mTargetMaterialSlot, GetTargetMaterialSlot, SetTargetMaterialSlot, "TargetMaterialSlot", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mTargetMaterialSlot), ReflectEditor("UV Animation", "Material Slot"), ReflectOptions(GetTargetMaterialSlots));

    FName mTargetMaterialSlot{};
    ReflectAccessor(mUVTransform, GetUVTransform, SetUVTransform, "UVTransform", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Inline, ReflectSerializer(mUVTransform), ReflectEditor("UV Animation", "UV Transform"), ReflectCondition(CanEditUVTransform));

    FUVTransform mUVTransform{};
    double mElapsedTime{};
    ReflectReadOnlyAccessor(mPlaying, IsPlaying, "Playing", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mPlaying), ReflectSerializationName("bPlaying"));

    bool mPlaying{true};
    ReflectAccessor(mLooping, IsLooping, SetLooping, "Looping", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mLooping), ReflectEditor("UV Animation", "Looping"), ReflectSerializationName("bLooping"));

    bool mLooping{true};

    ReflectMembers();
};
