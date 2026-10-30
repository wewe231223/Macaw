/**
 * @file FPlaceActorsPanel.h
 * @brief 검색과 분류를 통해 액터 프리셋을 배치하는 에디터 패널입니다.
 * @ingroup Editor
 */

#pragma once

#include "Editor/Panel/FEditorPanel.h"

namespace Editor {

    /**
     * @brief 검색과 분류를 통해 액터 프리셋을 배치하는 에디터 패널입니다.
     * @ingroup Editor
     */
    class FPlaceActorsPanel final : public Editor::FEditorPanel {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FPlaceActorsPanel, Editor::FEditorPanel);

    public:
        /**
         * @brief 편집 컨텍스트를 연결하고 액터 배치 패널을 생성합니다.
         * @param[in] Context 액터 생성과 선택을 수행할 편집 컨텍스트의 비소유 주소입니다.
         */
        explicit FPlaceActorsPanel(Editor::FEditorContext* Context);

        /**
         * @brief FPlaceActorsPanel 객체의 수명을 끝내고 소유 자원을 정리합니다.
         */
        ~FPlaceActorsPanel() override;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FPlaceActorsPanel);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

        /**
         * @brief 검색·분류와 액터 배치 항목을 패널 안에 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 초 단위 시간입니다.
         */
        void DrawContent(float DeltaTime) override;

        /**
         * @brief 아이콘·설명과 드래그 페이로드를 갖는 액터 프리셋 항목을 그립니다.
         * @param[in] Preset 생성하거나 표시할 액터 프리셋의 식별자입니다.
         * @param[in] Label 배치 항목에 표시할 이름입니다.
         * @param[in] Components 기본 컴포넌트의 설명 문자열입니다.
         * @param[in] Icon 항목에 표시할 아이콘 문자열입니다.
         */
        void DrawEntry(const Core::FString& Preset, const Core::FString& Label, const char* Components, const char* Icon);

    private:
        /**
         * @brief 액터 프리셋의 검색 문자열 버퍼입니다.
         */
        std::array<char, 128> mSearch{};
        ReflectProperty(mSearch, "Search", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 배치 패널에서 선택한 액터 분류 인덱스입니다.
         */
        Core::Int32 mCategory{};
        ReflectProperty(mCategory, "Category", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
