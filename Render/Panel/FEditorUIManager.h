#pragma once

#include "PCH.h"
#include "IEditorPanel.h"
#include "FControlPanel.h"
#include "FPropertyPanel.h"

#include "Core/Channel/FStateChannel.h"
#include "Core/Channel/FMessageChannel.h"

class FEditorUIManager
{
public:
    void Initialize(
        FStateChannel<FMessageEditorCameraState>::FWriter CamWriter,
        FStateChannel<FMessageEditorCameraState>::FReader CamReader,

        FStateChannel<FMessageEditorTransformState>::FWriter TransformWriter,
        FStateChannel<FMessageEditorTransformState>::FReader TransformReader,

        FMessageChannel::FSender SpawnSender,
        FMessageChannel::FSender SceneSender,
        FMessageChannel::FSender GizmoSender
    )
    {
        Panels.emplace_back(
            std::make_unique<FControlPanel>(
                std::move(CamWriter),
                std::move(CamReader),
                std::move(SpawnSender),
                std::move(SceneSender)
            )
        );

        Panels.emplace_back(
            std::make_unique<FPropertyPanel>(
                std::move(TransformWriter),
                std::move(TransformReader),
                std::move(GizmoSender)
            )
        );
    }

    void Tick()
    {
        for (const std::unique_ptr<IEditorPanel>& Panel : Panels)
        {
            if (Panel != nullptr &&
                Panel->IsVisible())
            {
                Panel->DrawPanel();
            }
        }
    }

private:
    std::vector<std::unique_ptr<IEditorPanel>> Panels;
};