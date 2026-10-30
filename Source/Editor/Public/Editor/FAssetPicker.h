/**
 * @file FAssetPicker.h
 * @brief 썸네일·검색·드래그 앤 드롭을 공유하는 에셋 선택기입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FAssetFile.h"

namespace Editor {

    class FEditorAssetCatalog;
    class FAssetThumbnailCache;

    /**
     * @brief 실제 상속 재질을 표시하며 타입이 맞는 에셋으로 교체하는 위젯입니다.
     */
    class FAssetPicker {
    public:
        /**
         * @brief 리플렉션 소유 타입입니다.
         */
        ReflectTypes(FAssetPicker);

    public:
        /**
         * @brief 빈 검색 상태를 구성합니다.
         */
        FAssetPicker() = default;

    public:
        /**
         * @brief 정적 메타데이터를 반환합니다.
         * @return 등록한 구조체 정보입니다.
         */
        ReflectStruct(FAssetPicker);

        /**
         * @brief 썸네일과 검색 가능한 선택기를 표시합니다.
         * @param Catalog 표시 목록입니다.
         * @param Thumbnails 공유 썸네일 캐시입니다.
         * @param Label 위젯 이름입니다.
         * @param Path 변경할 실제 에셋 경로이며 빈 경로는 상속 또는 기본값입니다.
         * @param Type 허용할 에셋 종류입니다.
         * @param InheritedPath 오버라이드가 없을 때 사용하는 실제 에셋 경로입니다.
         * @param EmptyLabel 기본값 선택의 설명입니다.
         * @param InheritedLabel 상속 경로의 출처 설명입니다.
         * @return 선택 또는 드롭으로 경로를 바꾸면 true입니다.
         */
        bool Draw(Editor::FEditorAssetCatalog& Catalog, Editor::FAssetThumbnailCache& Thumbnails, const char* Label, Core::FString& Path, Engine::EAssetType Type, const Core::FString& InheritedPath = {}, const char* EmptyLabel = "None", const char* InheritedLabel = "Mesh");

    private:
        /**
         * @brief 자동 멤버 등록 접근을 허용합니다.
         */
        ReflectMembers();

        /**
         * @brief 목록을 검색하여 썸네일과 함께 표시합니다.
         * @param Catalog 에셋 목록입니다.
         * @param Thumbnails 썸네일 캐시입니다.
         * @param Path 선택 경로입니다.
         * @param Type 허용 종류입니다.
         * @param EmptyLabel 기본값 선택의 설명입니다.
         * @return 목록에서 값을 선택하면 true입니다.
         */
        bool DrawOptions(Editor::FEditorAssetCatalog& Catalog, Editor::FAssetThumbnailCache& Thumbnails, Core::FString& Path, Engine::EAssetType Type, const char* EmptyLabel);

        /**
         * @brief 직전 위젯에서 같은 종류의 드롭을 받습니다.
         * @param Catalog 경로와 타입을 검증할 목록입니다.
         * @param Path 교체할 경로입니다.
         * @param Type 허용할 종류입니다.
         * @return 유효한 경로가 전달되면 true입니다.
         */
        static bool AcceptDrop(Editor::FEditorAssetCatalog& Catalog, Core::FString& Path, Engine::EAssetType Type);

    private:
        /**
         * @brief 현재 열린 선택기의 검색 문자열입니다.
         */
        std::array<char, 128> mSearch{};
        ReflectProperty(mSearch, "Search", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
