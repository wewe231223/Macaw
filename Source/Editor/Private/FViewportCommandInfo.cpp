/**
 * @file FViewportCommandInfo.cpp
 * @brief 뷰포트 명령의 공통 정의, 식별자 조회와 단축키 이름 변환을 구현합니다.
 * @ingroup Editor
 */

#include "Editor/FViewportCommandInfo.h"
#include <array>

/**
 * @brief UI 표시와 입력 처리에서 함께 사용하는 고정 명령 정의를 보관합니다.
 */
namespace {
    /**
     * @brief 선택, 이동, 회전, 스케일을 포함한 뷰포트의 공통 명령 목록입니다.
     * @note 입력 처리에서는 모드 선택, 모드 순환, 좌표계 전환, 스냅 전환 순서를 유지합니다.
     */
    const std::array<FViewportCommandInfo, 13> CommandInfos{{{EViewportCommand::Select, "Select", EEditorIcon::Tools, 'Q', &FViewportInput::mSelectMode, true}, {EViewportCommand::Translate, "Translate", EEditorIcon::Translate, 'W', &FViewportInput::mTranslateMode, true}, {EViewportCommand::Rotate, "Rotate", EEditorIcon::Rotate, 'E', &FViewportInput::mRotateMode, true}, {EViewportCommand::Scale, "Scale", EEditorIcon::Scale, 'R', &FViewportInput::mScaleMode, true}, {EViewportCommand::CycleTransformMode, "Cycle transform mode", EEditorIcon::Tools, ' ', &FViewportInput::mCycleGizmo}, {EViewportCommand::ToggleCoordinateSystem, "World/Local", EEditorIcon::World, '\t', &FViewportInput::mToggleSpace}, {EViewportCommand::WorldSpace, "World", EEditorIcon::World}, {EViewportCommand::LocalSpace, "Local", EEditorIcon::Local}, {EViewportCommand::ToggleGrid, "Show Grid", EEditorIcon::Grid}, {EViewportCommand::ToggleTranslationSnap, "Translation Snap", EEditorIcon::Grid}, {EViewportCommand::ToggleRotationSnap, "Rotation Snap", EEditorIcon::Rotate}, {EViewportCommand::ToggleScaleSnap, "Scale Snap", EEditorIcon::Scale}, {EViewportCommand::ToggleTransformSnap, "Snap", EEditorIcon::Grid, 'X', &FViewportInput::mToggleSnap}}};
}

std::span<const FViewportCommandInfo> FViewportCommandInfo::GetCommands() {
    return CommandInfos;
}

const FViewportCommandInfo* FViewportCommandInfo::Find(EViewportCommand Command) {
    for (const FViewportCommandInfo& Info : CommandInfos) {
        if (Info.mCommand == Command) {
            return &Info;
        }
    }

    return nullptr;
}

FString FViewportCommandInfo::GetShortcutLabel() const {
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

    return FString{Name.data()};
}
