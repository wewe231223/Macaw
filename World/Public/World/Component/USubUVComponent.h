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
    UPROPERTY(Name = "GuidTargetComponent", Reference, ReadOnly)
    TObjectRef<UPrimitiveComponent> mTargetComponent{};
    FGuid mPendingTargetGuid{};
    UPROPERTY(Name = "TargetMaterialGroup", ReadOnly)
    FName mTargetMaterialGroup{};
    UPROPERTY(Name = "UVTransform", ReadOnly)
    FUVTransform mUVTransform{};
    double mElapsedTime{};
    UPROPERTY(Name = "Playing", ReadOnly)
    bool mPlaying{true};
    UPROPERTY(Name = "Looping", ReadOnly)
    bool mLooping{true};
};
