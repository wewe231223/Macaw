#include "PCH.h"
#include "UWorld.h"

#include <algorithm>

#include "AActor.h"
#include "Component/UCameraComponent.h"
#include "Component/UCollisionComponent.h"
#include "Component/UStaticMeshComponent.h"

UWorld::~UWorld()
{
    for (const std::unique_ptr<AActor>& Actor : Actors)
    {
        UObjectSystem::Unregister(Actor.get(), Actor->GetHandle());
    }

    Actors.clear();
}

const std::vector<std::unique_ptr<AActor>>& UWorld::GetActors() const
{
    return Actors;
}

FRenderProbe& UWorld::BuildRenderProbe() 
{
	Probe.ActorProbes.clear();

    for (const UStaticMeshComponent* Component : RenderableComponents)
    {
        Component->MakeRender(Probe);
    }

    if (Camera != nullptr)
    {
        Probe.MainCameraProbe.View =
            Camera->GetViewMatrix();

        Probe.MainCameraProbe.Projection =
            Camera->GetProjectionMatrix();

        Probe.MainCameraProbe.ViewProjection =
            Camera->GetViewProjectionMatrix();
    }
    return Probe;
}

void UWorld::Tick(float DeltaTime)
{
    for (const std::unique_ptr<AActor>& Actor : Actors)
    {
        Actor->Tick(DeltaTime);
    }
}

void UWorld::RegisterRenderable(UStaticMeshComponent* Component)
{
    if (Component == nullptr || std::ranges::find(RenderableComponents, Component) != RenderableComponents.end())
    {
        return;
    }

    RenderableComponents.push_back(Component);
    CollisionComponents.push_back(Component);
}

void UWorld::UnregisterRenderable(UStaticMeshComponent* Component)
{
    std::erase(RenderableComponents, Component);
}

void UWorld::SetMainCamera(UCameraComponent* InCamera)
{
    Camera = InCamera;
}

void UWorld::ClearMainCamera(UCameraComponent* InCamera)
{
    if (Camera == InCamera)
    {
        Camera = nullptr;
    }
}

AActor* UWorld::AddActor(std::unique_ptr<AActor> InActor) {
    if (!InActor)
    {
        return nullptr;
    }

    AActor* Actor = InActor.get();

    // 아직 등록되지 않은 Actor만 등록
    if (UObjectSystem::Resolve(Actor->GetHandle()) != Actor)
    {
        UObjectSystem::Register(Actor);
    }

    // 먼저 World가 소유권을 확보
    Actors.push_back(std::move(InActor));

    // 컴포넌트 OnCreate 호출보다 먼저 World가 소유하고 있어야 함
    Actor->SetWorld(this);

    return Actor;
}

AActor* UWorld::PickActor(int MouseX, int MouseY, int ScreenWidth, int ScreenHeight, const UCameraComponent* Camera)
{
    if (Camera == nullptr || ScreenWidth <= 0 || ScreenHeight <= 0)
    {
        return nullptr;
    }

    // 1. Screen Space -> NDC 변환 (-1.0 ~ 1.0)
    float NormX = (2.0f * static_cast<float>(MouseX) / static_cast<float>(ScreenWidth)) - 1.0f;
    float NormY = 1.0f - (2.0f * static_cast<float>(MouseY) / static_cast<float>(ScreenHeight));

    // 2. 카메라의 투영 및 뷰 행렬 가져오기
    const auto& Proj = Camera->GetProjectionMatrix();
    const auto& View = Camera->GetViewMatrix();

    // 3. 뷰 공간 기준 레이 방향 벡터
    FVector3 RayDirView{ NormX / Proj._11, NormY / Proj._22, 1.0f };

    // 4. 뷰 공간 -> 월드 공간 방향 변환 (뷰 행렬 상단 3x3 회전부의 전치 적용)
    FVector3 RayDirWorld{
        RayDirView.x * View._11 + RayDirView.y * View._12 + RayDirView.z * View._13,
        RayDirView.x * View._21 + RayDirView.y * View._22 + RayDirView.z * View._23,
        RayDirView.x * View._31 + RayDirView.y * View._32 + RayDirView.z * View._33
    };
    RayDirWorld.Normalize();

    // 5. 카메라 월드 위치 (광선 시작점: -Eye * R^T 역산)
    FVector3 RayOriginWorld{
        -(View._41 * View._11 + View._42 * View._12 + View._43 * View._13),
        -(View._41 * View._21 + View._42 * View._22 + View._43 * View._23),
        -(View._41 * View._31 + View._42 * View._32 + View._43 * View._33)
    };

    FRay TestRay(RayOriginWorld, RayDirWorld);

    // 6. 월드 액터 순회 피킹
    AActor* ClosestHitActor = nullptr;
    float MinDistance = 100000.0f;

    for (const auto& Actor : Actors)
    {
        if (!Actor) continue;

        for (const auto& Comp : Actor->GetComponents())
        {
            if (auto* Collision = dynamic_cast<UCollisionComponent*>(Comp.get()))
            {
                float Distance = 0.0f;
                if (Collision->Raycast(TestRay, Distance))
                {
                    if (Distance < MinDistance)
                    {
                        MinDistance = Distance;
                        ClosestHitActor = Actor.get();
                    }
                }
                break; // 한 액터에 충돌체가 1개라면 검사 후 탈출
            }
        }
    }

    return ClosestHitActor;
}