#pragma once

struct FTransform;
struct FVector;

namespace PhysicsRuntime {
    class UBodySetup;
}

namespace Engine {
    class FPrimitiveDrawInterface;
    class UWorld;
}

namespace Engine::PhysicsBodyDrawing {
    void DrawBodySetup(Engine::FPrimitiveDrawInterface& Draw, const PhysicsRuntime::UBodySetup& Setup, const FTransform& Transform, const FVector& Color, bool DepthTest, float Thickness, float Opacity = 1.0f);
    void DrawWorldBodies(Engine::FPrimitiveDrawInterface& Draw, const Engine::UWorld& World, float Thickness);
}
