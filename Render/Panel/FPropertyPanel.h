#pragma once

#include "PCH.h"
#include "ImGui/imgui.h"
#include "IEditorPanel.h"
#include "FEditorInfo.h"
#include "Core/Channel/FStateChannel.h"
#include "Core/Channel/FMessageChannel.h"

class FPropertyPanel : public IEditorPanel
{
public:
    FPropertyPanel(
        FStateChannel<FMessageEditorTransformState>::FWriter InTransformWriter,
        FStateChannel<FMessageEditorTransformState>::FReader InTransformReader,
        FMessageChannel::FSender InEventSender
    )
        : TransformWriter(std::move(InTransformWriter))
        , TransformReader(std::move(InTransformReader))
        , EventSender(std::move(InEventSender))
    {
    }

    void DrawPanel() override {
        // 1. 상태 채널에서 트랜스폼 및 선택 정보 읽기 (Engine -> UI)
        if (auto Result = TransformReader.ReadIfChanged(); Result.Changed && Result.Value != nullptr)
        {
            CachedState = *Result.Value;
        }

        // 선택된 객체가 없으면 패널 자체를 그리지 않음
        if (!CachedState.bIsSelected)
        {
            return;
        }

        ImGui::Begin("Property Window");

        // 2. 단방향 채널: 기즈모 변경 이벤트 전송 (UI -> Engine)
        ImGui::Text("Gizmo Mode");
        int ModeIndex = static_cast<int>(CurrentGizmoMode);
        bool bGizmoChanged = false;

        bGizmoChanged |= ImGui::RadioButton("Translate", &ModeIndex, 0);
        ImGui::SameLine();
        bGizmoChanged |= ImGui::RadioButton("Rotate", &ModeIndex, 1);
        ImGui::SameLine();
        bGizmoChanged |= ImGui::RadioButton("Scale", &ModeIndex, 2);

        if (bGizmoChanged)
        {
            CurrentGizmoMode = static_cast<EGizmoMode>(ModeIndex);
            EventSender.TryEmplace<FMessageChangeGizmoMode>(CurrentGizmoMode);
        }
        ImGui::Separator();

        // 3. 상태 채널: 조작된 트랜스폼 쓰기 (UI -> Engine)
        ImGui::Text("Transform");
        bool bTransformModifiedByUI = false;

        bTransformModifiedByUI |= ImGui::DragFloat3("Position", &CachedState.Position.x, 0.1f);
        bTransformModifiedByUI |= ImGui::DragFloat3("Rotation", &CachedState.Rotation.x, 0.5f);
        bTransformModifiedByUI |= ImGui::DragFloat3("Scale", &CachedState.Scale.x, 0.05f);

        if (bTransformModifiedByUI)
        {
            TransformWriter.Write(CachedState);
        }

        ImGui::End();
    }

private:
    FStateChannel<FMessageEditorTransformState>::FWriter TransformWriter;
    FStateChannel<FMessageEditorTransformState>::FReader TransformReader;

    FMessageChannel::FSender EventSender;

    FMessageEditorTransformState CachedState;
    EGizmoMode CurrentGizmoMode = EGizmoMode::Translate;
};
