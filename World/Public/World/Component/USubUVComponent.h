#pragma once
#include "World/Component/UActorComponent.h"
#include "World/Component/UPrimitiveComponent.h"
#include "CoreUObject/TObjectRef.h"
#include "World/Component/USubUVComponent.generated.h"

UCLASS()
class USubUVComponent : public UActorComponent {
public:
    GENERATED_TYPES(USubUVComponent)

public:
    USubUVComponent();
    ~USubUVComponent() override = default;

public:
    GENERATED_BODY(USubUVComponent)

    bool SetTargetComponent(UPrimitiveComponent* Target);
    UPrimitiveComponent* GetTargetComponent() const;
    void SetTargetMaterialGroup(FName MaterialGroup);
    FName GetTargetMaterialGroup() const;
    bool HasTargetConflict() const;
    bool CanSelectTarget(UPrimitiveComponent* Target) const;
    bool CanEditUVTransform() const;
    TArray<FName> GetTargetMaterialGroups() const;
    void SetUVTransform(const FUVTransform& Transform);
    const FUVTransform& GetUVTransform() const;
    void UpdateUVTransform();
    UFUNCTION()
    void Play();
    UFUNCTION()
    void Pause();
    UFUNCTION()
    void Stop();
    UFUNCTION()
    void Restart();
    UFUNCTION()
    bool IsPlaying() const;
    UFUNCTION()
    void SetLooping(bool Looping);
    UFUNCTION()
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
    virtual FUVTransform CalculateUVTransform() const;
    virtual bool CanAdvance() const;
    virtual double GetDuration() const;
    virtual void ResetPlaybackState();
    void ApplyUVTransform();
    void ClearTargetTransform();

private:
    UPROPERTY(Edit, Serialize, Name = "GuidTargetComponent", DisplayName = "Target Component", Category = "UV Animation", Reference, OwnerOnly, Getter = GetTargetComponent, Setter = SetTargetComponent, ObjectFilter = CanSelectTarget, PendingGuid = mPendingTargetGuid)
    TObjectRef<UPrimitiveComponent> mTargetComponent{};
    FGuid mPendingTargetGuid{};
    UPROPERTY(Edit, Serialize, Name = "TargetMaterialGroup", DisplayName = "Material Group", Category = "UV Animation", Getter = GetTargetMaterialGroup, Setter = SetTargetMaterialGroup, OptionsGetter = GetTargetMaterialGroups)
    FName mTargetMaterialGroup{};
    UPROPERTY(Edit, Serialize, Inline, Name = "UVTransform", DisplayName = "UV Transform", Category = "UV Animation", Getter = GetUVTransform, Setter = SetUVTransform, EditCondition = CanEditUVTransform)
    FUVTransform mUVTransform{};
    double mElapsedTime{};
    UPROPERTY(Serialize, Name = "Playing", SerializeName = "bPlaying", Getter = IsPlaying, ReadOnly)
    bool mPlaying{true};
    UPROPERTY(Edit, Serialize, Name = "Looping", SerializeName = "bLooping", DisplayName = "Looping", Category = "UV Animation", Getter = IsLooping, Setter = SetLooping)
    bool mLooping{true};
};
