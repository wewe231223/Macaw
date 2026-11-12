#pragma once

#include "Core/Types.h"
#include "Core/Math/FBox.h"

#include <memory>

namespace Asset {
    class FAssetManager;
    class UAnimSequence;
}

namespace Engine {
    class FWorldContext;
    class UWorld;
    class USkeletalMeshComponent;
    class UAnimSingleNodeInstance;
}

namespace RenderCore {
    class IRendererModule;
}

namespace Editor {

    class FAnimationPreviewScene {
    public:
        FAnimationPreviewScene(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets);
        ~FAnimationPreviewScene();

    public:
        bool SetMesh(const Core::FString& MeshPath, const Core::FString& SkeletonPath, Core::FString& Error);
        void SetAnimation(const Asset::UAnimSequence* Sequence);

        void Play();
        void Pause();
        void Seek(double Time);
        void Tick(float DeltaTime);
        void RefreshPose();

        void SetLooping(bool Looping);
        void SetPlayRate(float PlayRate);
        void SetPreviewMode(Core::Int32 Mode);

        bool IsReady() const;
        bool IsPlaying() const;
        double GetPosition() const;
        float GetPlayRate() const;
        bool IsLooping() const;
        Core::Int32 GetPreviewMode() const;
        const Core::FBox& GetBounds() const;
        Engine::UWorld* GetWorld() const;
        Engine::USkeletalMeshComponent* GetComponent() const;

    private:
        Engine::UAnimSingleNodeInstance* GetInstance() const;
        void UpdateRootMotion();
        void SynchronizeScene();

    private:
        RenderCore::IRendererModule* mRenderer{};
        Asset::FAssetManager* mAssets{};
        const Asset::UAnimSequence* mSequence{};
        std::unique_ptr<Engine::FWorldContext> mWorldContext{};
        Engine::USkeletalMeshComponent* mComponent{};
        bool mLooping{true};
        float mPlayRate{1.0f};
        Core::Int32 mPreviewMode{};
        Core::FBox mBounds{};
    };

}
