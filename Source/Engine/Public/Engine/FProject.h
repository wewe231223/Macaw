/**
 * @file FProject.h
 * @brief 프로젝트 설정 파일, 시작 장면과 에셋 관리자에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/FAssetManager.h"

namespace Engine {

    /**
     * @brief 프로젝트 설정 파일, 시작 장면과 에셋 관리를 담당합니다.
     */
    class FProject {
    public:
        /**
         * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
         */
        ReflectTypes(FProject);

    public:
        /**
         * @brief FProject의 초기 상태를 구성합니다.
         */
        FProject();

        /**
         * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~FProject();

        /**
         * @brief 소유 상태가 중복되지 않도록 복사·이동 생성을 금지합니다.
         */
        FProject(const Engine::FProject&) = delete;

        /**
         * @brief 소유 상태가 중복되지 않도록 대입을 금지합니다.
         */
        Engine::FProject& operator=(const Engine::FProject&) = delete;

    public:
        /**
         * @brief 이 타입의 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FProject);

        /**
         * @brief 빈 프로젝트 폴더에 템플릿 콘텐츠와 초기 장면 및 설정 파일을 생성합니다.
         * @param[in] ProjectFile 생성하거나 열 프로젝트의 .mproject 파일 경로입니다.
         * @param[in] TemplateDirectory 새 프로젝트에 복사할 템플릿 폴더입니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Create(const std::filesystem::path& ProjectFile, const std::filesystem::path& TemplateDirectory, Core::FString& Error);

        /**
         * @brief 프로젝트 설정의 버전과 콘텐츠 경로를 검증하여 현재 프로젝트로 엽니다.
         * @param[in] ProjectFile 생성하거나 열 프로젝트의 .mproject 파일 경로입니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Open(const std::filesystem::path& ProjectFile, Core::FString& Error);

        /**
         * @brief 프로젝트 이름과 시작 장면 경로를 설정 파일에 저장합니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Save(Core::FString& Error) const;

        /**
         * @brief 콘텐츠 경로를 검증하여 시작 장면을 변경하고 프로젝트 설정을 저장합니다.
         * @param[in] Path 시작 장면으로 지정할 장면 파일의 경로입니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetStartupScene(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 프로젝트의 표시 이름을 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::FString& GetName() const;

        /**
         * @brief 프로젝트 콘텐츠 폴더를 기준으로 한 시작 장면 경로를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::FString& GetStartupScene() const;

        /**
         * @brief 현재 프로젝트 설정 파일의 절대 경로를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const std::filesystem::path& GetFilePath() const;

        /**
         * @brief 프로젝트 설정 파일을 포함하는 루트 폴더를 반환합니다.
         * @return 프로젝트 루트 폴더 경로입니다.
         */
        std::filesystem::path GetRootPath() const;

        /**
         * @brief 현재 프로젝트에서 사용할 셰이더 캐시 폴더를 반환합니다.
         * @return 셰이더 캐시 폴더 경로입니다.
         */
        std::filesystem::path GetShaderCachePath() const;

        /**
         * @brief 프로젝트의 에셋 관리자를 비소유 주소로 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Engine::FAssetManager* GetAssets() const;

    private:
        /**
         * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 프로젝트 설정에 저장된 표시 이름입니다.
         */
        Core::FString mName{};
        ReflectProperty(mName, "Name", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 콘텐츠 폴더 기준으로 저장하는 시작 장면의 상대 경로입니다.
         */
        Core::FString mStartupScene{"Maps/Main.masset"};
        ReflectProperty(mStartupScene, "StartupScene", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 프로젝트 설정 파일의 절대 경로입니다.
         */
        std::filesystem::path mFilePath{};
        ReflectProperty(mFilePath, "FilePath", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프로젝트 콘텐츠를 읽고 캐시하는 소유 에셋 관리자입니다.
         */
        std::unique_ptr<Engine::FAssetManager> mAssets{};
        ReflectProperty(mAssets, "Assets", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
