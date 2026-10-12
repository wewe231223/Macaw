#pragma once
#include "World/Component/UActorComponent.h"
#include "World/Component/UPrimitiveComponent.h"
#include "CoreUObject/TObjectRef.h"

class USubUVComponent : public UActorComponent {
public:
    USubUVComponent();
    ~USubUVComponent() override = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(USubUVComponent, UActorComponent);

    bool SetTargetComponent(UPrimitiveComponent* Target);
    UPrimitiveComponent* GetTargetComponent() const;
    void SetTargetMaterialGroup(FName MaterialGroup);
    FName GetTargetMaterialGroup() const;
    bool HasTargetConflict() const;
    void SetUVTransform(const FUVTransform& Transform);
    const FUVTransform& GetUVTransform() const;
    void UpdateUVTransform();
    void Play();
    void Pause();
    void Stop();
    void Restart();
    bool IsPlaying() const;
    void SetLooping(bool Looping);
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
    TObjectRef<UPrimitiveComponent> mTargetComponent{};
    FGuid mPendingTargetGuid{};
    FName mTargetMaterialGroup{};
    FUVTransform mUVTransform{};
    double mElapsedTime{};
    bool mPlaying{true};
    bool mLooping{true};
};
