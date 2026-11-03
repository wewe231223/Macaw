/**
 * @file FGameExporter.h
 * @brief 프로젝트를 독립 실행 게임 폴더로 내보내는 기능을 선언합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <filesystem>

namespace Engine {
    class FProject;
}

namespace Editor {

    /**
     * @brief 게임 런타임 실행 파일, DLL, 프로젝트 Content와 템플릿을 하나의 폴더로 복사합니다.
     * @ingroup Editor
     */
    class FGameExporter {
    public:
        /**
         * @brief 시작 장면을 검증한 뒤 게임 런타임과 프로젝트 파일을 내보낼 폴더에 복사합니다.
         * @param[in] Project 내보낼 프로젝트입니다.
         * @param[in] RuntimeDirectory MacawGame.exe와 런타임 DLL이 있는 폴더입니다.
         * @param[in] Destination 내보낼 폴더입니다. 없으면 생성하고 같은 이름의 파일은 덮어씁니다.
         * @param[out] Error 실패 원인을 받을 문자열입니다.
         * @return 모든 파일을 복사했으면 true, 검증이나 복사에 실패하면 false입니다.
         * @note 실행 파일은 프로젝트 파일 이름에 .exe를 붙여 복사하고, 프로젝트 파일은 Game.mproject로 복사합니다.
         * @note 시작 장면을 불러올 수 없거나 활성 카메라가 없으면 아무것도 복사하지 않습니다.
         */
        static bool Export(const Engine::FProject& Project, const std::filesystem::path& RuntimeDirectory, const std::filesystem::path& Destination, Core::FString& Error);
    };

}
