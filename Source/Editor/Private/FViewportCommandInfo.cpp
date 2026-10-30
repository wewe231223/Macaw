/**
 * @file FViewportCommandInfo.cpp
 * @brief 뷰포트 명령의 공통 정의, 식별자 조회와 단축키 이름 변환을 구현합니다.
 * @ingroup Editor
 */

#include "Editor/FViewportCommandInfo.h"
#include <array>

namespace Editor {

    /**
     * @brief UI 표시와 입력 처리에서 함께 사용하는 고정 명령 정의를 보관합니다.
     */
    namespace {
        /**
         * @brief 선택, 이동, 회전, 스케일을 포함한 뷰포트의 공통 명령 목록입니다.
         * @note 입력 처리에서는 모드 선택, 모드 순환, 좌표계 전환, 스냅 전환 순서를 유지합니다.
         */
        const std::array<Editor::FViewportCommandInfo, 13> CommandInfos{{{Editor::EViewportCommand::Select, "Select", Editor::EEditorIcon::Tools, 'Q', &Engine::FViewportInput::mSelectMode, true}, {Editor::EViewportCommand::Translate, "Translate", Editor::EEditorIcon::Translate, 'W', &Engine::FViewportInput::mTranslateMode, true}, {Editor::EViewportCommand::Rotate, "Rotate", Editor::EEditorIcon::Rotate, 'E', &Engine::FViewportInput::mRotateMode, true}, {Editor::EViewportCommand::Scale, "Scale", Editor::EEditorIcon::Scale, 'R', &Engine::FViewportInput::mScaleMode, true}, {Editor::EViewportCommand::CycleTransformMode, "Cycle transform mode", Editor::EEditorIcon::Tools, ' ', &Engine::FViewportInput::mCycleGizmo}, {Editor::EViewportCommand::ToggleCoordinateSystem, "World/Local", Editor::EEditorIcon::World, '\t', &Engine::FViewportInput::mToggleSpace}, {Editor::EViewportCommand::WorldSpace, "World", Editor::EEditorIcon::World}, {Editor::EViewportCommand::LocalSpace, "Local", Editor::EEditorIcon::Local}, {Editor::EViewportCommand::ToggleGrid, "Show Grid", Editor::EEditorIcon::Grid}, {Editor::EViewportCommand::ToggleTranslationSnap, "Translation Snap", Editor::EEditorIcon::Grid}, {Editor::EViewportCommand::ToggleRotationSnap, "Rotation Snap", Editor::EEditorIcon::Rotate}, {Editor::EViewportCommand::ToggleScaleSnap, "Scale Snap", Editor::EEditorIcon::Scale}, {Editor::EViewportCommand::ToggleTransformSnap, "Snap", Editor::EEditorIcon::Grid, 'X', &Engine::FViewportInput::mToggleSnap}}};
    }

    std::span<const Editor::FViewportCommandInfo> Editor::FViewportCommandInfo::GetCommands() {
        return CommandInfos;
    }

    const Editor::FViewportCommandInfo* Editor::FViewportCommandInfo::Find(Editor::EViewportCommand Command) {
        for (const Editor::FViewportCommandInfo& Info : CommandInfos) {
            if (Info.mCommand == Command) {
                return &Info;
            }
        }

        return nullptr;
    }

    Core::FString Editor::FViewportCommandInfo::GetShortcutLabel() const {
        if (mShortcut == '\0') {
            return {};
        }

        if (mShortcut == ' ') {
            return "Space";
        }

        if (mShortcut == '\t') {
            return "Tab";
        }

        const std::array<char, 2> Name{mShortcut, '\0'};

        return Core::FString{Name.data()};
    }

}
