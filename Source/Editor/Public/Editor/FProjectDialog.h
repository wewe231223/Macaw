/**
 * @file FProjectDialog.h
 * @brief 프로젝트 파일 생성 및 열기용 Windows 대화상자에 대한 선언입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <filesystem>

/**
 * @brief 프로젝트 파일 생성 및 열기용 Windows 대화상자를 담당합니다.
 */
class FProjectDialog {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FProjectDialog);

public:
    /**
     * @brief 이 타입의 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FProjectDialog);
    /**
     * @brief 프로젝트 생성 또는 열기 대화상자로 선택한 파일 경로를 반환합니다.
     * @param[in] Owner 대화상자의 소유 Windows 창 핸들입니다.
     * @param[in] Create true이면 새 프로젝트를 생성하고 false이면 기존 프로젝트를 엽니다.
     * @return 선택한 프로젝트 파일 경로이며 취소하면 빈 경로입니다.
     */
    static std::filesystem::path Choose(void* Owner, bool Create);
    /**
     * @brief 소유 창이 지정된 폴더 선택 대화상자를 열고 선택 경로를 반환합니다.
     * @param[in] Owner 대화상자를 소유할 창의 네이티브 핸들입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    static std::filesystem::path ChooseDirectory(void* Owner);

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();
};
