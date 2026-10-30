/**
 * @file FImGuiThemeCatalog.h
 * @brief 기본 테마와 ImThemes TOML에서 읽은 이름·작성자·설명·ImGui 스타일을 관리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <imgui.h>
#include <filesystem>

namespace Editor {

    /**
     * @brief 기본 테마와 ImThemes TOML에서 읽은 이름·작성자·설명·ImGui 스타일을 관리합니다.
     */
    class FImGuiThemeCatalog {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FImGuiThemeCatalog);

    public:
        /**
         * @brief 기본 설정으로 FImGuiThemeCatalog 객체를 초기화합니다.
         */
        FImGuiThemeCatalog();

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FImGuiThemeCatalog);

        /**
         * @brief ImThemes TOML의 단일 테마 또는 테마 배열을 검증한 뒤 카탈로그에 추가하거나 교체합니다.
         * @param[in] Path 읽거나 기록할 파일 시스템 경로입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 전체 파일을 수락하면 true이며 파싱·검증 실패 시 기존 카탈로그를 유지하고 false입니다.
         */
        bool Load(const std::filesystem::path& Path, Core::FString& Error);
        /**
         * @brief 기본 테마를 포함한 테마 개수를 반환합니다.
         * @return 현재 카탈로그에 보관한 테마 수입니다.
         */
        Core::Uint32 GetCount() const;
        /**
         * @brief 이름이 정확히 일치하는 테마 인덱스를 찾습니다.
         * @param[in] Name 조회하거나 지정할 이름입니다.
         * @return 일치하는 테마의 0 기반 인덱스이며 없으면 기본 테마 인덱스 0입니다.
         */
        Core::Uint32 Find(const Core::FString& Name) const;
        /**
         * @brief 인덱스에 해당하는 테마 이름을 반환합니다.
         * @param[in] Index 테마 목록의 유효한 0 기반 인덱스입니다.
         * @return 소유 객체가 보관하는 이름의 읽기 전용 문자열 참조입니다.
         * @note Index가 테마 개수 이상이면 std::out_of_range가 발생합니다.
         */
        const Core::FString& GetName(Core::Uint32 Index) const;
        /**
         * @brief 인덱스에 해당하는 테마 작성자를 반환합니다.
         * @param[in] Index 테마 목록의 유효한 0 기반 인덱스입니다.
         * @return 카탈로그가 보관하는 작성자 문자열의 읽기 전용 참조입니다.
         * @note Index가 테마 개수 이상이면 std::out_of_range가 발생합니다.
         */
        const Core::FString& GetAuthor(Core::Uint32 Index) const;
        /**
         * @brief 인덱스에 해당하는 테마 설명을 반환합니다.
         * @param[in] Index 테마 목록의 유효한 0 기반 인덱스입니다.
         * @return 카탈로그가 보관하는 설명 문자열의 읽기 전용 참조입니다.
         * @note Index가 테마 개수 이상이면 std::out_of_range가 발생합니다.
         */
        const Core::FString& GetDescription(Core::Uint32 Index) const;
        /**
         * @brief 인덱스에 해당하는 배율 적용 전 테마 스타일을 반환합니다.
         * @param[in] Index 테마 목록의 유효한 0 기반 인덱스입니다.
         * @return 카탈로그가 소유하는 ImGui 스타일의 읽기 전용 참조입니다.
         * @note Index가 테마 개수 이상이면 std::out_of_range가 발생합니다.
         */
        const ImGuiStyle& GetStyle(Core::Uint32 Index) const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 테마 인덱스로 조회할 이름 목록이며 0번은 Macaw Dark입니다.
         */
        Core::TArray<Core::FString> mNames{};
        ReflectProperty(mNames, "Names", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 각 테마와 같은 인덱스로 정렬한 작성자 문자열입니다.
         */
        Core::TArray<Core::FString> mAuthors{};
        ReflectProperty(mAuthors, "Authors", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 각 테마와 같은 인덱스로 정렬한 설명 문자열입니다.
         */
        Core::TArray<Core::FString> mDescriptions{};
        ReflectProperty(mDescriptions, "Descriptions", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 배율을 적용하기 전의 테마별 ImGui 스타일입니다.
         */
        Core::TArray<ImGuiStyle> mStyles{};
        ReflectProperty(mStyles, "Styles", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
