#pragma once

#include "Core/Types.h"

namespace Asset {
    class UAnimSequence;
}

namespace Editor {

    class FAnimationPreviewScene;

    class FAnimSequenceTimeline {
    public:
        FAnimSequenceTimeline();
        ~FAnimSequenceTimeline();

    public:
        void Reset(const Asset::UAnimSequence& Sequence);
        void Draw(Editor::FAnimationPreviewScene& Preview, const Asset::UAnimSequence& Sequence, float Height);
        void StepFrame(Editor::FAnimationPreviewScene& Preview, const Asset::UAnimSequence& Sequence, Core::Int32 Direction);

    private:
        void DrawTimeArea(Editor::FAnimationPreviewScene& Preview, const Asset::UAnimSequence& Sequence, float Height);
        void DrawTransportBar(Editor::FAnimationPreviewScene& Preview, const Asset::UAnimSequence& Sequence);

    private:
        Core::Int32 mEndFrame{1};
        Core::Uint32 mSequenceRevision{};
    };

}
