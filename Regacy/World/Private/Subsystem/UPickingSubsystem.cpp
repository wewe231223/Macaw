#include "pch.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Core/Base/ErrorHandler.h"
#include "World/Subsystem/UPickingSubsystem.h"
#include "Core/Stat/Stat.h"
#include "Core/Spatial/FBVHBuildOptimization.h"
#include "World/Component/UMeshComponent.h"
#include "World/Component/UBillboardComponent.h"
#include "World/Component/UPrimitiveComponent.h"

#include <chrono>
#include <cmath>

namespace {
    bool AreBoundsFinite(const DirectX::BoundingBox& Box) {
        const float C[]{Box.Center.x, Box.Center.y, Box.Center.z}, E[]{Box.Extents.x, Box.Extents.y, Box.Extents.z};

        for (Uint32 Index{0}; Index < 3; ++Index)
            if (!(E[Index] >= 0.0f) || !std::isfinite(C[Index] - E[Index]) || !std::isfinite(C[Index] + E[Index]))
                return false;

        return true;
    }
}

FWorldRaycastAccelerationStructure::FProxy FWorldRaycastAccelerationStructure::MakeProxy(UPrimitiveComponent* Component, DirectX::BoundingBox& Bounds) {
    FProxy Proxy{};

    Proxy.mComponent.Set(Component);
    Proxy.mEnabled = Component->IsRegistered() && Component->IsVisible();

    const Uint64 Key{ComponentKey(Component->GetHandle())};

    if (Component->GetClass()->IsChildOf(UBillboardComponent::StaticClass())) {
        Proxy.mKind = EProxyKind::Billboard;

        const auto* Billboard{static_cast<const UBillboardComponent*>(Component)};

        Proxy.mBillboardSize = Billboard->GetSize();

        FBillboardDrawData Data{};
        const bool Renderable{Billboard->BuildBillboardDrawData(Data)};

        Proxy.mEnabled = Proxy.mEnabled && Renderable && Proxy.mBillboardSize.mX > 0.0f && Proxy.mBillboardSize.mY > 0.0f;
        Proxy.mBox.Center = Renderable ? Data.mWorld.Translation().ToSimpleMath() : Component->GetComponentLocation().ToSimpleMath();

        const float Radius{0.5f * (std::abs(Proxy.mBillboardSize.mX) + std::abs(Proxy.mBillboardSize.mY))};

        Bounds = {Proxy.mBox.Center, {Radius, Radius, Radius}};
    } else {
        DirectX::XMFLOAT3 Corners[8]{};

        const bool IsMesh{Component->GetClass()->IsChildOf(UMeshComponent::StaticClass())};
        const DirectX::BoundingOrientedBox& LocalBox{Component->GetPickingBox()};
        const DirectX::XMFLOAT4& Orientation{LocalBox.Orientation};

        Proxy.mKind = IsMesh ? EProxyKind::Mesh : EProxyKind::Box;
        Proxy.mHasCustomBox = Orientation.x != 0.0f || Orientation.y != 0.0f || Orientation.z != 0.0f;

        if (Proxy.mHasCustomBox) {
            mLocalBoxes[Key] = LocalBox;
        } else {
            mLocalBoxes.erase(Key);
        }

        LocalBox.GetCorners(Corners);
        DirectX::BoundingBox::CreateFromPoints(Proxy.mBox, 8, Corners, sizeof(Corners[0]));

        const FMatrix WorldMatrix{Component->GetComponentToWorld()};

        for (DirectX::XMFLOAT3& Corner : Corners) {
            DirectX::XMStoreFloat3(&Corner, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&Corner), WorldMatrix.ToSimpleMath()));
        }

        Proxy.mMesh.Update(IsMesh ? static_cast<const UMeshComponent*>(Component)->ResolveMesh() : nullptr, Component->GetComponentTransform());

        DirectX::BoundingBox::CreateFromPoints(Bounds, 8, Corners, sizeof(Corners[0]));

        if (Proxy.mKind == EProxyKind::Mesh && Proxy.mMesh.GetMesh() != nullptr) {
            Proxy.mMesh.GetMesh()->GetBoundingBox().GetCorners(Corners);

            const auto Matrix{Component->GetComponentToWorld().ToSimpleMath()};

            for (DirectX::XMFLOAT3& Corner : Corners) {
                DirectX::XMStoreFloat3(&Corner, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&Corner), Matrix));
            }

            DirectX::BoundingBox MeshBounds{};
            DirectX::BoundingBox::CreateFromPoints(MeshBounds, 8, Corners, sizeof(Corners[0]));
            DirectX::BoundingBox::CreateMerged(Bounds, Bounds, MeshBounds);
        }

        const float Pad{4.0f * std::numeric_limits<float>::epsilon()};

        Bounds.Extents.x += Pad * (std::abs(Bounds.Center.x) + Bounds.Extents.x);
        Bounds.Extents.y += Pad * (std::abs(Bounds.Center.y) + Bounds.Extents.y);
        Bounds.Extents.z += Pad * (std::abs(Bounds.Center.z) + Bounds.Extents.z);
    }

    if (!AreBoundsFinite(Bounds))
        Proxy.mEnabled = false;

    return Proxy;
}

bool FWorldRaycastAccelerationStructure::RaycastProxy(const FProxy& Proxy, const FRay& Ray, float MaxDistance, FWorldRaycastHit& OutHit, const FMatrix* CameraWorld, EPickingTarget Target) const {
    if (!Proxy.mEnabled)
        return false;

    if (Proxy.mMesh.mSource != nullptr)
        _mm_prefetch(reinterpret_cast<const char*>(Proxy.mMesh.mSource.get()), _MM_HINT_T0);

    if (Proxy.mKind == EProxyKind::Billboard) {
        if (CameraWorld == nullptr)
            return false;

        FVector3 Right{CameraWorld->m_[0][0], CameraWorld->m_[0][1], CameraWorld->m_[0][2]};
        FVector3 Up{CameraWorld->m_[1][0], CameraWorld->m_[1][1], CameraWorld->m_[1][2]};

        if (Right.LengthSquared() <= 0.0f || Up.LengthSquared() <= 0.0f)
            return false;

        Right.Normalize();
        Up.Normalize();

        const FVector3 Corner{FVector3{Proxy.mBox.Center} - Right * (Proxy.mBillboardSize.mX * 0.5f) + Up * (Proxy.mBillboardSize.mY * 0.5f)};

        Right *= Proxy.mBillboardSize.mX;

        const FVector3 Down{Up * -Proxy.mBillboardSize.mY};
        const FVector3 Normal{Right.Cross(Down)}, Direction{Ray.direction};
        const float Denominator{Direction.Dot(Normal)};

        if (std::abs(Denominator) <= 0.000001f)
            return false;

        const float HitDistance{(Corner - FVector3{Ray.position}).Dot(Normal) / Denominator};

        if (HitDistance < 0.0f || HitDistance > MaxDistance)
            return false;

        const FVector3 HitOffset{FVector3{Ray.position} + Direction * HitDistance - Corner};
        const float Horizontal{HitOffset.Dot(Right) / Right.LengthSquared()}, Vertical{HitOffset.Dot(Down) / Down.LengthSquared()};

        if (!(Horizontal >= 0.0f && Horizontal <= 1.0f && Vertical >= 0.0f && Vertical <= 1.0f))
            return false;

        OutHit.mDistance = HitDistance;
        OutHit.mPosition = FVector3{Ray.position} + Direction * HitDistance;
        OutHit.mNormal = Normal;
        OutHit.mNormal.Normalize();

        if (OutHit.mNormal.Dot(Direction) > 0.0f) {
            OutHit.mNormal = -OutHit.mNormal;
        }

        return true;
    }

    FRay LocalRay{};
    double DirectionLength{};

    if (!Proxy.mMesh.PrepareRay(Ray, LocalRay, DirectionLength))
        return false;

    if (Proxy.mKind == EProxyKind::Mesh) {
        if (Target == EPickingTarget::Primitives) {
            float LocalDistance{};

            if (!Proxy.mBox.Intersects(LocalRay.position, LocalRay.direction, LocalDistance) || (LocalDistance > static_cast<double>(MaxDistance) * DirectionLength && Proxy.mBox.Contains(LocalRay.position) == DirectX::DISJOINT)) {
                return false;
            }

            if (Proxy.mHasCustomBox) {
                const auto It{mLocalBoxes.find(ComponentKey(Proxy.mComponent.GetHandle()))};

                if (It == mLocalBoxes.end() || !It->second.Intersects(LocalRay.position, LocalRay.direction, LocalDistance) || (LocalDistance > static_cast<double>(MaxDistance) * DirectionLength && It->second.Contains(LocalRay.position) == DirectX::DISJOINT)) {
                    return false;
                }
            }
        }

        if (!Proxy.mMesh.RaycastPrepared(LocalRay, DirectionLength, OutHit, MaxDistance)) {
            return false;
        }
    } else {
        DirectX::BoundingOrientedBox Box{};

        if (Proxy.mHasCustomBox) {
            const auto It{mLocalBoxes.find(ComponentKey(Proxy.mComponent.GetHandle()))};

            if (It == mLocalBoxes.end()) {
                return false;
            }

            Box = It->second;
        } else {
            DirectX::BoundingOrientedBox::CreateFromBoundingBox(Box, Proxy.mBox);
        }

        FWorldRaycastHit LocalHit{};

        if (!RaycastBox(Box, LocalRay, LocalHit, std::numeric_limits<float>::max())) {
            return false;
        }

        OutHit.mDistance = static_cast<float>(LocalHit.mDistance / DirectionLength);
        OutHit.mNormal = Proxy.mMesh.TransformNormal(LocalHit.mNormal);
    }

    if (OutHit.mDistance > MaxDistance) {
        return false;
    }

    OutHit.mPosition = FVector3{Ray.position} + FVector3{Ray.direction} * OutHit.mDistance;

    return true;
}

void FWorldRaycastAccelerationStructure::Clear() {
    mTree.Clear();
    mLocalBoxes.clear();
    mLeaves.clear();
    mFreeLeaves.clear();
    mLeafIndices.clear();
    mBuilt = false;
}

bool FWorldRaycastAccelerationStructure::BuildStructure(const TArray<TObjectRef<UPrimitiveComponent>>& Components) {
    Clear();

    TArray<FBuildItem> Items{};

    Items.reserve(Components.size());

    MinMaxBox Bounds{};

    for (const auto& Ref : Components) {
        UPrimitiveComponent* Component{Ref.Get()};

        if (Component == nullptr)
            continue;

        DirectX::BoundingBox Box{};

        MakeProxy(Component, Box);

        if (!AreBoundsFinite(Box)) {
            Clear();
            return false;
        }

        Items.push_back({Box, Ref});
        Bounds.Expand(Box);
    }

    if (Items.empty()) {
        mBuilt = true;
        return true;
    }

    if (Items.size() > (static_cast<std::size_t>(InvalidIndex) + 1) / 2)
        return false;

    TArray<FNode> BuildNodes{};

    BuildNodes.reserve(Items.size() * 2 - 1);
    MakeChild(BuildNodes, Items, 0, static_cast<Uint32>(Items.size()), Bounds);
    BVH8::OptimizeBuildTreelets(BuildNodes);
    mLeaves.reserve(Items.size());
    mTree.Build(BuildNodes, [](const FNode& Node) {
        return Node.mLeft == InvalidIndex;
    }, [&](const FNode& Node) {
        const Uint32 Index{static_cast<Uint32>(mLeaves.size())};
        DirectX::BoundingBox Box{};

        mLeaves.push_back(MakeProxy(Node.mComponent.Get(), Box));
        mLeafIndices.emplace(ComponentKey(Node.mComponent.GetHandle()), Index);

        return Index;
    });

    mBuilt = true;

    return true;
}

void FWorldRaycastAccelerationStructure::InsertComponent(UPrimitiveComponent* Component) {
    if (!mBuilt || Component == nullptr)
        return;

    const Uint64 Key{ComponentKey(Component->GetHandle())};

    if (mLeafIndices.contains(Key))
        return;

    DirectX::BoundingBox Box{};
    auto Proxy{MakeProxy(Component, Box)};

    if (!AreBoundsFinite(Box)) {
        mLocalBoxes.erase(Key);
        return;
    }

    const Uint32 Index{mFreeLeaves.empty() ? static_cast<Uint32>(mLeaves.size()) : mFreeLeaves.back()};

    ErrorHandler::Report(Index > BVH8::IndexMask, "UPickingSubsystem", "World BVH leaf capacity", ErrorHandler::EErrorLevel::Critical);

    if (mFreeLeaves.empty())
        mLeaves.push_back(std::move(Proxy));
    else {
        mFreeLeaves.pop_back();
        mLeaves[Index] = std::move(Proxy);
    }

    mTree.Insert(Index, Box);
    mLeafIndices.emplace(Key, Index);
}

void FWorldRaycastAccelerationStructure::UpdateComponent(UPrimitiveComponent* Component) {
    if (!mBuilt || Component == nullptr)
        return;

    const auto It{mLeafIndices.find(ComponentKey(Component->GetHandle()))};

    if (It == mLeafIndices.end()) {
        InsertComponent(Component);
        return;
    }

    auto& Proxy{mLeaves[It->second]};
    DirectX::BoundingBox Box{};

    Proxy = MakeProxy(Component, Box);

    if (!AreBoundsFinite(Box)) {
        RemoveComponent(Component);
        return;
    }

    mTree.Update(It->second, Box);
}

void FWorldRaycastAccelerationStructure::RemoveComponent(UPrimitiveComponent* Component) {
    if (Component == nullptr)
        return;

    mLocalBoxes.erase(ComponentKey(Component->GetHandle()));

    const auto It{mLeafIndices.find(ComponentKey(Component->GetHandle()))};

    if (It == mLeafIndices.end())
        return;

    const Uint32 Index{It->second};

    mTree.Remove(Index);
    mLeaves[Index] = {};
    mFreeLeaves.push_back(Index);
    mLeafIndices.erase(It);
}

Uint32 FWorldRaycastAccelerationStructure::MakeChild(TArray<FNode>& BuildNodes, TArray<FBuildItem>& Items, Uint32 First, Uint32 Count, const MinMaxBox& Bounds) {
    FNode Node{};

    Node.BoundingBox.Center = {Bounds.mMinX * 0.5f + Bounds.mMaxX * 0.5f, Bounds.mMinY * 0.5f + Bounds.mMaxY * 0.5f, Bounds.mMinZ * 0.5f + Bounds.mMaxZ * 0.5f};
    Node.BoundingBox.Extents = {Bounds.mMaxX * 0.5f - Bounds.mMinX * 0.5f, Bounds.mMaxY * 0.5f - Bounds.mMinY * 0.5f, Bounds.mMaxZ * 0.5f - Bounds.mMinZ * 0.5f};
    Node.BoundingBox.Extents.x += 4.0f * std::numeric_limits<float>::epsilon() * (std::abs(Node.BoundingBox.Center.x) + Node.BoundingBox.Extents.x);
    Node.BoundingBox.Extents.y += 4.0f * std::numeric_limits<float>::epsilon() * (std::abs(Node.BoundingBox.Center.y) + Node.BoundingBox.Extents.y);
    Node.BoundingBox.Extents.z += 4.0f * std::numeric_limits<float>::epsilon() * (std::abs(Node.BoundingBox.Center.z) + Node.BoundingBox.Extents.z);

    const Uint32 NodeIndex{static_cast<Uint32>(BuildNodes.size())};

    BuildNodes.push_back(Node);

    if (Count == 1) {
        BuildNodes[NodeIndex].BoundingBox = Items[First].BoundingBox;
        BuildNodes[NodeIndex].mComponent = Items[First].mComponent;
        return NodeIndex;
    }

    const auto& BoundingBox{Node.BoundingBox};

    MinMaxBox Bin[3][Slice]{};
    Uint32 BinCounts[3][Slice]{};
    const float Centers[3]{BoundingBox.Center.x, BoundingBox.Center.y, BoundingBox.Center.z};
    const float Extents[3]{BoundingBox.Extents.x, BoundingBox.Extents.y, BoundingBox.Extents.z};

    const auto GetBinIndex{[&](const DirectX::BoundingBox& Box, Uint32 Axis) {
        const float BoxCenters[3]{Box.Center.x, Box.Center.y, Box.Center.z};
        const float Normalized{Extents[Axis] > 0.0f ? (BoxCenters[Axis] - Centers[Axis]) / Extents[Axis] : -1.0f};

        return static_cast<Uint32>(std::clamp((Normalized + 1.0f) * Slice / 2, 0.0f, static_cast<float>(Slice - 1)));
    }};

    for (Uint32 Offset{0}; Offset < Count; ++Offset) {
        const auto& Box{Items[First + Offset].BoundingBox};

        for (Uint32 Axis{0}; Axis < 3; ++Axis) {
            const Uint32 Index{GetBinIndex(Box, Axis)};

            Bin[Axis][Index].Expand(Box);
            ++BinCounts[Axis][Index];
        }
    }

    Uint32 BestAxis{0};
    Uint32 BestLeftEnd{0};
    float BestCost{std::numeric_limits<float>::max()};
    MinMaxBox BestLeftBox{};
    MinMaxBox BestRightBox{};

    for (Uint32 Axis{0}; Axis < 3; ++Axis) {
        MinMaxBox RightBoxes[Slice]{};
        Uint32 RightCounts[Slice];

        RightBoxes[Slice - 1] = Bin[Axis][Slice - 1];
        RightCounts[Slice - 1] = BinCounts[Axis][Slice - 1];

        for (int Index{Slice - 2}; Index >= 0; --Index) {
            RightBoxes[Index] = MinMaxBox::Merge(Bin[Axis][Index], RightBoxes[Index + 1]);
            RightCounts[Index] = BinCounts[Axis][Index] + RightCounts[Index + 1];
        }

        MinMaxBox LeftBox{};
        Uint32 LeftCount{0};

        for (int LeftEnd{0}; LeftEnd < Slice - 1; ++LeftEnd) {
            LeftBox = MinMaxBox::Merge(LeftBox, Bin[Axis][LeftEnd]);
            LeftCount += BinCounts[Axis][LeftEnd];

            const MinMaxBox& RightBox{RightBoxes[LeftEnd + 1]};
            const Uint32 RightCount{RightCounts[LeftEnd + 1]};

            if (LeftCount == 0 || RightCount == 0)
                continue;

            float Cost{LeftCount * LeftBox.SurfaceArea() + RightCount * RightBox.SurfaceArea()};

            if (Cost < BestCost) {
                BestCost = Cost;
                BestLeftEnd = LeftEnd;
                BestAxis = Axis;
                BestLeftBox = LeftBox;
                BestRightBox = RightBox;
            }
        }
    }

    Uint32 LeftCount{Count / 2};

    if (BestCost < std::numeric_limits<float>::max()) {
        auto Begin{Items.begin() + First};
        auto Middle{std::partition(Begin, Begin + Count, [&](const FBuildItem& Item) {
            return GetBinIndex(Item.BoundingBox, BestAxis) <= BestLeftEnd;
        })};

        LeftCount = static_cast<Uint32>(Middle - Begin);
    }

    if (BestCost == std::numeric_limits<float>::max() || LeftCount == 0 || LeftCount == Count) {
        LeftCount = Count / 2;
        BestLeftBox = {};
        BestRightBox = {};

        for (Uint32 Offset{0}; Offset < Count; ++Offset) {
            if (Offset < LeftCount)
                BestLeftBox.Expand(Items[First + Offset].BoundingBox);
            else
                BestRightBox.Expand(Items[First + Offset].BoundingBox);
        }
    }

    BuildNodes[NodeIndex].mLeft = MakeChild(BuildNodes, Items, First, LeftCount, BestLeftBox);
    BuildNodes[NodeIndex].mRight = MakeChild(BuildNodes, Items, First + LeftCount, Count - LeftCount, BestRightBox);

    return NodeIndex;
}

bool FWorldRaycastAccelerationStructure::Raycast(const FRay& Ray, const FWorldRaycastQuery& Query, FWorldRaycastHit& OutHit, EPickingTarget Target, const FMatrix* CameraWorld) const {
    OutHit = {};

    FRay WorldRay{};

    if (!Query.PrepareRay(Ray, WorldRay)) {
        return false;
    }

    float Distance{Query.mMaxDistance};

    mTree.Raycast(WorldRay, Distance, [&](Uint32 LeafIndex, float& ClosestDistance) {
        const FProxy& Proxy{mLeaves[LeafIndex]};
        UPrimitiveComponent* Component{Proxy.mComponent.Get()};

        if (Component == nullptr || !Component->IsRegistered() || Component->IsBeingDestroyed() || Query.Excludes(*Component) || (Target == EPickingTarget::MeshSurfaces && Proxy.mKind != EProxyKind::Mesh)) {
            return false;
        }

        FWorldRaycastHit Hit{};

        if (!RaycastProxy(Proxy, WorldRay, ClosestDistance, Hit, CameraWorld, Target)) {
            return false;
        }

        if (OutHit.mComponent && Hit.mDistance >= ClosestDistance) {
            return false;
        }

        ClosestDistance = Hit.mDistance;
        Hit.mComponent = Proxy.mComponent;
        OutHit = Hit;

        return true;
    });

    return OutHit.mComponent.IsValid();
}

void FWorldRaycastAccelerationStructure::WarmupRaycast() const {
    (void)std::chrono::steady_clock::now();

    BVH8::FNode Node{};
    BVH8::FTrianglePacket Packet{};

    for (Uint32 Index{0}; Index < 8; ++Index) {
        Node.MinX[Index] = Node.MinY[Index] = -1.0f;
        Node.MaxX[Index] = Node.MaxY[Index] = 1.0f;
        Node.MinZ[Index] = -0.01f;
        Node.MaxZ[Index] = 0.01f;
        Node.Children[Index] = BVH8::MakeReference(0, 8, true);
        Packet.V0[0][Index] = Packet.V0[1][Index] = -1.0f;
        Packet.Edge1[1][Index] = 2.0f;
        Packet.Edge2[0][Index] = 2.0f;
    }

    for (bool Parallel : {false, true})
        for (bool Reverse : {false, true}) {
            const auto Direction{DirectX::XMVector3Normalize(DirectX::XMVectorSet(Parallel ? 0.0f : 0.1f, Parallel ? 0.0f : 0.1f, Reverse ? -1.0f : 1.0f, 0.0f))};
            const FRay Ray{DirectX::XMVectorSubtract(DirectX::XMVectorSet(-0.25f, -0.25f, 0, 0), DirectX::XMVectorScale(Direction, 2.0f)), Direction};
            float Distance{10.0f};

            BVH8::RaycastTrianglePackets(&Node, BVH8::MakeReference(0, 8), BVH8::FRayData{Ray}, Ray, Distance, &Packet, Reverse);
        }

    const FMatrix Camera{FMatrix::Identity};
    const size_t Count{(std::min)(mLeaves.size(), size_t{16})};

    for (size_t Index{0}; Index < Count; ++Index) {
        const auto& Proxy{mLeaves[Index * mLeaves.size() / Count]};

        if (!Proxy.mEnabled)
            continue;

        auto Center{DirectX::XMLoadFloat3(&Proxy.mBox.Center)};
        float Radius{(std::max)(Proxy.mBillboardSize.mX, Proxy.mBillboardSize.mY)};

        if (Proxy.mKind != EProxyKind::Billboard) {
            if (!Proxy.mMesh.mValid)
                continue;

            const auto Scale{DirectX::XMVectorSetW(DirectX::XMLoadFloat3(&Proxy.mMesh.mInverseScale), 1.0f)}, Rotation{DirectX::XMLoadFloat4(&Proxy.mMesh.mRotation)};

            Center = DirectX::XMVectorAdd(DirectX::XMVector3Rotate(DirectX::XMVectorDivide(Center, Scale), Rotation), DirectX::XMLoadFloat3(&Proxy.mMesh.mPosition));

            const auto Extents{DirectX::XMVectorAbs(DirectX::XMVectorDivide(DirectX::XMLoadFloat3(&Proxy.mBox.Extents), Scale))};

            Radius = (std::max)({DirectX::XMVectorGetX(Extents), DirectX::XMVectorGetY(Extents), DirectX::XMVectorGetZ(Extents)}) * 2.0f;
        }

        if (!std::isfinite(Radius) || Radius > std::numeric_limits<float>::max() * 0.5f || DirectX::XMVector3IsNaN(Center) || DirectX::XMVector3IsInfinite(Center))
            continue;

        Radius = (std::max)(Radius, 1.0f);

        for (bool Parallel : {false, true}) {
            const auto Direction{DirectX::XMVector3Normalize(DirectX::XMVectorSet(Parallel ? 0.0f : 0.3f, Parallel ? 0.0f : 0.2f, 1.0f, 0.0f))};
            const FRay Ray{DirectX::XMVectorSubtract(Center, DirectX::XMVectorScale(Direction, Radius * 2.0f)), Direction};
            float Distance{std::numeric_limits<float>::max()};

            FWorldRaycastHit Hit{};
            RaycastProxy(Proxy, Ray, Distance, Hit, &Camera);

            if (Index % 4 == 0) {
                Raycast(Ray, FWorldRaycastQuery{}, Hit, EPickingTarget::Primitives, &Camera);
            }
        }
    }
}

void UPickingSubsystem::RegisterComponent(UPrimitiveComponent* Component) {
    if (Component == nullptr)
        return;

    const auto Handle{Component->GetHandle()};
    const Uint64 Key{(static_cast<Uint64>(Handle.mGeneration) << 32) | Handle.mIndex};

    if (!mRegisteredComponentKeys.insert(Key).second)
        return;

    mComponents.emplace_back(Component);

    if (mRaycastAccelerationStructure.IsBuilt())
        UpdateComponent(Component);
}

void UPickingSubsystem::UnregisterComponent(UPrimitiveComponent* Component) {
    if (Component == nullptr)
        return;

    const auto Handle{Component->GetHandle()};
    const Uint64 Key{(static_cast<Uint64>(Handle.mGeneration) << 32) | Handle.mIndex};

    mRegisteredComponentKeys.erase(Key);
    mDirtyComponentKeys.erase(Key);
    std::erase_if(mDirtyComponents, [Handle](const auto& Ref) {
        return Ref.GetHandle().mIndex == Handle.mIndex && Ref.GetHandle().mGeneration == Handle.mGeneration;
    });

    mRaycastAccelerationStructure.RemoveComponent(Component);
    std::erase_if(mComponents, [Component](const TObjectRef<UPrimitiveComponent>& Ref) {
        return Ref.Get() == Component;
    });
}

bool UPickingSubsystem::RebuildAccelerationStructure() {
    if (!mRaycastAccelerationStructure.BuildStructure(mComponents))
        return false;

    mDirtyComponents.clear();
    mDirtyComponentKeys.clear();
    mRaycastAccelerationStructure.WarmupRaycast();

    return true;
}

void UPickingSubsystem::UpdateComponent(UPrimitiveComponent* Component) {
    if (Component == nullptr || !ContainsComponent(Component))
        return;

    const auto Handle{Component->GetHandle()};
    const Uint64 Key{(static_cast<Uint64>(Handle.mGeneration) << 32) | Handle.mIndex};

    if (mDirtyComponentKeys.insert(Key).second)
        mDirtyComponents.emplace_back(Component);
}

void UPickingSubsystem::SynchronizeProxies() const {
    if (!mRaycastAccelerationStructure.IsBuilt()) {
        mRaycastAccelerationStructure.BuildStructure(mComponents);
    }

    for (std::size_t Index{0}; Index < mDirtyComponents.size(); ++Index)
        mRaycastAccelerationStructure.UpdateComponent(mDirtyComponents[Index].Get());

    mDirtyComponents.clear();
    mDirtyComponentKeys.clear();
}

bool UPickingSubsystem::Raycast(const FRay& Ray, const FWorldRaycastQuery& Query, FWorldRaycastHit& OutHit, EPickingTarget Target, const FMatrix* CameraWorld) const {
    const Stat::FScopedPickingStatTimer PickingTimer{};

    SynchronizeProxies();
    return mRaycastAccelerationStructure.Raycast(Ray, Query, OutHit, Target, CameraWorld);
}

bool UPickingSubsystem::ContainsComponent(const UPrimitiveComponent* Component) const {
    if (Component == nullptr)
        return false;

    const auto Handle{Component->GetHandle()};

    return mRegisteredComponentKeys.contains((static_cast<Uint64>(Handle.mGeneration) << 32) | Handle.mIndex);
}

const TArray<TObjectRef<UPrimitiveComponent>>& UPickingSubsystem::GetRegisteredComponents() const {
    return mComponents;
}

void UPickingSubsystem::OnDeinitialize() {
    mComponents.clear();
    mRegisteredComponentKeys.clear();
    mDirtyComponents.clear();
    mDirtyComponentKeys.clear();
    mRaycastAccelerationStructure.Clear();
}

UPickingSubsystem::UPickingSubsystem() {
    InitializeClass(StaticClass());
}

void FWorldRaycastAccelerationStructure::MinMaxBox::Expand(const DirectX::BoundingBox& Box) {
    if (mMinX > Box.Center.x - Box.Extents.x)
        mMinX = Box.Center.x - Box.Extents.x;

    if (mMinY > Box.Center.y - Box.Extents.y)
        mMinY = Box.Center.y - Box.Extents.y;

    if (mMinZ > Box.Center.z - Box.Extents.z)
        mMinZ = Box.Center.z - Box.Extents.z;

    if (mMaxX < Box.Center.x + Box.Extents.x)
        mMaxX = Box.Center.x + Box.Extents.x;

    if (mMaxY < Box.Center.y + Box.Extents.y)
        mMaxY = Box.Center.y + Box.Extents.y;

    if (mMaxZ < Box.Center.z + Box.Extents.z)
        mMaxZ = Box.Center.z + Box.Extents.z;
}

float FWorldRaycastAccelerationStructure::MinMaxBox::SurfaceArea() const {
    const float DX{mMaxX - mMinX};
    const float DY{mMaxY - mMinY};
    const float DZ{mMaxZ - mMinZ};

    if (DX < 0.0f || DY < 0.0f || DZ < 0.0f)
        return 0.0f;

    return 2.0f * (DX * DY + DY * DZ + DZ * DX);
}

FWorldRaycastAccelerationStructure::MinMaxBox FWorldRaycastAccelerationStructure::MinMaxBox::Merge(const MinMaxBox& Box1, const MinMaxBox& Box2) {
    MinMaxBox Result{};

    Result.mMinX = (std::min)(Box1.mMinX, Box2.mMinX);
    Result.mMinY = (std::min)(Box1.mMinY, Box2.mMinY);
    Result.mMinZ = (std::min)(Box1.mMinZ, Box2.mMinZ);

    Result.mMaxX = (std::max)(Box1.mMaxX, Box2.mMaxX);
    Result.mMaxY = (std::max)(Box1.mMaxY, Box2.mMaxY);
    Result.mMaxZ = (std::max)(Box1.mMaxZ, Box2.mMaxZ);

    return Result;
}

bool FWorldRaycastAccelerationStructure::IsBuilt() const {
    return mBuilt;
}

Uint64 FWorldRaycastAccelerationStructure::ComponentKey(FObjectHandle Handle) {
    return (static_cast<Uint64>(Handle.mGeneration) << 32) | Handle.mIndex;
}
