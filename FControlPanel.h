#pragma once

#include "PCH.h"
#include "ImGui/imgui.h"
#include "Render/Panel/IEditorPanel.h"
#include "Render/Panel/FEditorInfo.h"
#include "Core/Channel/FStateChannel.h"
#include "Core/Channel/FMessageChannel.h"

std::string GetFilePathFromExplorer();

class FControlPanel : public IEditorPanel
{
public:
    FControlPanel(
        FStateChannel<FMessageEditorCameraState>::FWriter InCamWriter,
        FStateChannel<FMessageEditorCameraState>::FReader InCamReader,
        //FMessageChannel::FSender InWorldCommandSender
        FMessageChannel::FSender InSpawnSender,
        FMessageChannel::FSender InSceneSender
    )
        : CamWriter(std::move(InCamWriter))
        , CamReader(std::move(InCamReader))
        //, WorldCommandSender(std::move(InWorldCommandSender))
        , SpawnSender(std::move(InSpawnSender))
        , SceneSender(std::move(InSceneSender))
    {
    }

    void DrawPanel() override {
        // 1. 상태 채널에서 카메라 정보 읽기 (Engine -> UI)
        if (auto Result = CamReader.ReadIfChanged(); Result.Changed && Result.Value != nullptr)
        {
            CachedCamPos = Result.Value->Position;
            CachedCamRot = Result.Value->Rotation;
            CachedFOV = Result.Value->FOV;
        }

        ImGui::Begin("Control Panel");

        // 2. 단방향 채널: 스폰 이벤트 전송 (UI -> Engine)
        ImGui::Text("Spawn Primitive");

        const char* PrimitiveTypes[] =
        {
            "Cube",
            "Sphere",
            "Plane",
            "Cylinder"
        };

        ImGui::Combo(
            "Type",
            &SelectedPrimitiveIndex,
            PrimitiveTypes,
            IM_ARRAYSIZE(PrimitiveTypes));

        ImGui::InputInt(
            "Number of Objects to Spawn",
            &SpawnCountToRequest);

        if (SpawnCountToRequest < 1)
        {
            SpawnCountToRequest = 1;
        }

        if (ImGui::Button("Spawn Object(s)"))
        {
            SpawnSender.TryEmplace<FMessageSpawnPrimitive>(
                FString(PrimitiveTypes[SelectedPrimitiveIndex]),
                static_cast<uint32>(SpawnCountToRequest));
        }

        ImGui::Separator();

        // =====================================================
        // Scene
        // =====================================================

        ImGui::Text("Scene Management");

        ImGui::InputText(
            "Scene Name",
            SceneNameBuffer,
            IM_ARRAYSIZE(SceneNameBuffer));

        if (ImGui::Button("Save Scene"))
        {
            SceneSender.TryEmplace<FMessageSaveScene>(
                FString(SceneNameBuffer));
        }

        ImGui::SameLine();

        if (ImGui::Button("Load Scene"))
        {
            const std::string FilePath =
                GetFilePathFromExplorer();

            if (!FilePath.empty())
            {
                SceneSender.TryEmplace<FMessageLoadScene>(
                    FString(FilePath));
            }
        }

        ImGui::Separator();

        // =====================================================
        // Camera
        // =====================================================

        ImGui::Text("Camera");

        bool bCameraChanged = false;

        float FOVDegrees =
            CachedFOV * 180.0f / 3.1415926535f;

        if (ImGui::SliderFloat(
            "FOV",
            &FOVDegrees,
            30.0f,
            120.0f))
        {
            CachedFOV =
                FOVDegrees * 3.1415926535f / 180.0f;

            bCameraChanged = true;
        }

        bCameraChanged |= ImGui::DragFloat3(
            "Location",
            &CachedCamPos.x,
            0.1f);

        bCameraChanged |= ImGui::DragFloat3(
            "Rotation",
            &CachedCamRot.x,
            0.01f);

        if (bCameraChanged)
        {
            CamWriter.Write(
                FMessageEditorCameraState
                {
                    CachedCamPos,
                    CachedCamRot,
                    CachedFOV
                });

            CamReader.Read();
        }

        if (ImGui::Button("Undo"))
        {
           // Undo
        }
        ImGui::SameLine();

        if (ImGui::Button("Redo"))
        {
            // Redo
        }
        ImGui::SameLine();

        //if (ImGui::BeginCombo("##History", current_transaction_name)) {
        //    // 트랜잭션 목록 렌더링
        //    ImGui::EndCombo();
        //}

        ImGui::End();
    }

private:
    FStateChannel<FMessageEditorCameraState>::FWriter CamWriter;
    FStateChannel<FMessageEditorCameraState>::FReader CamReader;

   // FMessageChannel::FSender WorldCommandSender;

    FMessageChannel::FSender SpawnSender;
    FMessageChannel::FSender SceneSender;

private:
    char SceneNameBuffer[256] = "NewScene";

    int SelectedPrimitiveIndex = 0;
    int SpawnCountToRequest = 1;

    FVector3 CachedCamPos{};
    FRotator CachedCamRot{};

    // UCameraComponent와 동일하게 radians
    float CachedFOV = 1.0472f;
};