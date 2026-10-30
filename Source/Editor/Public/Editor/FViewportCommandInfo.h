/**
 * @file FViewportCommandInfo.h
 * @brief 툴바, 메뉴와 입력 처리가 공유하는 뷰포트 명령의 표시 정보와 단축키를 정의합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "Editor/EViewportCommand.h"
#include "Editor/EEditorIcon.h"
#include "Engine/FViewportInput.h"
#include <span>

/**
 * @brief UI 라이브러리에 의존하지 않는 뷰포트 명령 정의입니다.
 * @ingroup Editor
 * @note 이 타입은 표시 정보와 입력 매핑만 제공하며 실행 가능 여부와 실제 동작은 FEditorViewportClient가 처리합니다.
 */
struct FViewportCommandInfo {
public:
    /**
     * @brief 리플렉션 등록에 사용할 소유 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FViewportCommandInfo);

public:
    /**
     * @brief 이 명령 정의의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FViewportCommandInfo);

    /**
     * @brief 툴바, 메뉴와 입력 매핑에서 공유하는 명령 목록을 반환합니다.
     * @return 프로그램 수명 동안 유효한 명령 정의의 읽기 전용 뷰입니다.
     * @note 입력 처리 순서와 도구 표시 순서는 이 목록의 순서를 따릅니다.
     */
    static std::span<const FViewportCommandInfo> GetCommands();

    /**
     * @brief 동작 식별자로 공통 명령 정의를 조회합니다.
     * @param[in] Command 찾을 명령 식별자입니다.
     * @return 프로그램 수명 동안 유효한 정의의 비소유 주소이며 등록되지 않은 명령이면 nullptr입니다.
     */
    static const FViewportCommandInfo* Find(EViewportCommand Command);

    /**
     * @brief 단축키 문자를 메뉴와 도움말에 사용할 이름으로 변환합니다.
     * @return 영문 키의 이름, Space 또는 Tab이며 단축키가 없으면 빈 문자열입니다.
     */
    FString GetShortcutLabel() const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수의 접근 권한을 선언합니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 뷰포트 클라이언트에 전달할 동작 식별자입니다.
     */
    EViewportCommand mCommand{};
    ReflectProperty(mCommand, "Command", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 명령에 대응하는 버튼 및 메뉴 항목 이름입니다.
     */
    const char* mLabel{};
    ReflectProperty(mLabel, "Label", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 명령에 대응하는 공통 스타일 아이콘입니다.
     */
    EEditorIcon mIcon{};
    ReflectProperty(mIcon, "Icon", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 영문 대문자, 공백 또는 탭으로 표현하는 단축키이며 0이면 키에 연결하지 않습니다.
     */
    char mShortcut{};
    ReflectProperty(mShortcut, "Shortcut", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief FViewportInput에서 이 명령의 요청 여부를 읽고 쓸 bool 멤버를 가리키며 nullptr이면 키 입력에 연결하지
     * 않습니다.
     */
    bool FViewportInput::* mInput{};
    ReflectProperty(mInput, "Input", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 선택·이동·회전·스케일 도구 목록에 포함되는지 지정하며 모드 순환 명령은 포함하지 않습니다.
     */
    bool mTransformMode{};
    ReflectProperty(mTransformMode, "TransformMode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
