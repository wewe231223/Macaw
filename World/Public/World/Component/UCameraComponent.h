#pragma once
#include "World/Component/USceneComponent.h"
#include "Core/Archive/FArchive.h"
#include "World/Component/UCameraComponent.generated.h"

UCLASS()
class UCameraComponent : public USceneComponent {
public:
    GENERATED_TYPES(UCameraComponent)

public:
    inline static const FMatrix CameraBasis{
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f};

public:
    UCameraComponent();
    ~UCameraComponent() override = default;

public:
    GENERATED_BODY(UCameraComponent)

    FMatrix GetViewMatrix() const;
    FMatrix GetProjectionMatrix() const;
    FMatrix GetViewProjectionMatrix() const;

    UFUNCTION()
    float GetFOV() const;
    UFUNCTION()
    float GetAspectRatio() const;
    UFUNCTION()
    float GetNearPlane() const;
    UFUNCTION()
    float GetFarPlane() const;

    UFUNCTION()
    void SetFOV(float InFOV);
    UFUNCTION()
    void SetAspectRatio(float InAspectRatio);
    UFUNCTION()
    void SetNearPlane(float InNearPlane);
    UFUNCTION()
    void SetFarPlane(float InFarPlane);

    void OnRegister() override;
    void OnUnregister() override;

    void SetMoveSensitivity(float InMoveSensitivity);
    float GetMoveSensitivity() const;

    void SetRotationSensitivity(float InRotationSensitivity);
    float GetRotationSensitivity() const;

protected:
    void Serialize(FArchive& Archive) override;

private:
    UPROPERTY(Serialize, Name = "FOV", Getter = GetFOV, Category = "Camera", DisplayName = "FOV", Edit, Setter = SetFOV, Min = 0.01f, Max = 3.13f)
    float mFov{1.0472f}; // 약 60도, 라디안
    UPROPERTY(Serialize, Name = "AspectRatio", Getter = GetAspectRatio, Category = "Camera", DisplayName = "Aspect Ratio", Edit, Setter = SetAspectRatio, Min = 0.01f)
    float mAspectRatio{16.0f / 9.0f};
    UPROPERTY(Serialize, Name = "NearPlane", Getter = GetNearPlane, Category = "Camera", DisplayName = "Near Plane", Edit, Setter = SetNearPlane, Min = 0.001f)
    float mNearPlane{0.1f};
    UPROPERTY(Serialize, Name = "FarPlane", Getter = GetFarPlane, Category = "Camera", DisplayName = "Far Plane", Edit, Setter = SetFarPlane, Min = 0.001f)
    float mFarPlane{1000.0f};
    float mMoveSensitivity{5.0f};
    float mRotationSensitivity{0.1f};
};
