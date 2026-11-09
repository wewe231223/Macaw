/**
 * @file FDetailsPanel.h
 * @brief 선택한 객체의 리플렉션 프로퍼티를 표시하고 편집합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Editor/Panel/FEditorPanel.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "Editor/FAssetPicker.h"

namespace CoreUObject {
    class FProperty;
    class UObject;
}

namespace Script {
    class UScriptComponent;
}

namespace Editor {

    class FEditorAssetCatalog;
    class FAssetThumbnailCache;
    class FScriptBuildService;

    /**
     * @brief 선택한 객체의 리플렉션 프로퍼티를 표시하고 편집합니다.
     * @ingroup Editor
     */
    class FDetailsPanel final : public Editor::FEditorPanel {
    public:
        /**
         * @brief FDetailsPanel의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in,out] Context 패널이 조회하고 편집할 에디터 컨텍스트입니다.
         */
        explicit FDetailsPanel(Editor::FEditorContext* Context);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FDetailsPanel() override;

    public:
        /**
         * @brief 브라우저와 공유할 에셋 표시 서비스를 연결합니다.
         * @param Catalog 프로젝트 표시 목록입니다.
         * @param Thumbnails 썸네일 캐시입니다.
         */
        void SetAssetServices(Editor::FEditorAssetCatalog* Catalog, Editor::FAssetThumbnailCache* Thumbnails);

        /**
         * @brief C# 프로젝트 생성과 빌드 상태를 제공할 서비스를 연결합니다.
         * @param Builds 스크립트 빌드 서비스입니다.
         */
        void SetScriptBuildService(Editor::FScriptBuildService* Builds);
        void FinishInactiveSceneEdit();

    private:
        void BeginItemSceneEdit();

        /**
         * @brief C# 프로젝트 상태와 스크립트의 [Field] 필드를 표시합니다.
         * @param Component 표시할 스크립트 컴포넌트입니다.
         */
        void DrawScriptFields(Script::UScriptComponent* Component);

        /**
         * @brief 스크립트 클래스를 고르는 콤보를 표시합니다.
         * @param Label 위젯 이름입니다.
         * @param ClassName 선택한 클래스 이름입니다.
         * @return 선택이 바뀌면 true입니다.
         */
        bool DrawScriptClassPicker(const char* Label, Core::FString& ClassName);

        /**
         * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
         */
        void DrawContent(float DeltaTime) override;

        /**
         * @brief 생성 가능한 컴포넌트의 검색·추가 메뉴를 표시합니다.
         */
        void DrawComponentMenu();

        /**
         * @brief 선택한 액터 하나와 그 컴포넌트를 고르는 트리를 표시합니다.
         */
        void DrawComponentTree();

        /**
         * @brief 객체 타입의 편집 가능한 리플렉션 프로퍼티를 순회하여 표시합니다.
         * @param[in,out] Object 대상 객체입니다.
         */
        void DrawReflectedObject(CoreUObject::UObject* Object);

        /**
         * @brief 프로퍼티 타입에 맞는 편집 위젯을 표시하고 변경한 값을 돌려줍니다.
         * @param[in] Property 그릴 값의 프로퍼티 메타데이터입니다.
         * @param[in,out] Value 프로퍼티 타입에 대응하는 variant 값입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool DrawPropertyValue(const CoreUObject::FProperty* Property, CoreUObject::FPropertyValue& Value);

        /**
         * @brief 메시 또는 재질 선택과 드래그 앤 드롭을 표시합니다.
         * @param Label 위젯 이름입니다.
         * @param Path 선택할 Content 기준 경로입니다.
         * @param Mesh 메시 선택기이면 true입니다.
         * @return 선택 경로가 변경되면 true입니다.
         */
        bool DrawAssetPicker(const char* Label, Core::FString& Path, bool Mesh);

    private:
        /**
         * @brief Details 컴포넌트 검색 입력 버퍼입니다.
         */
        std::array<char, 128> mComponentSearch{};

        /**
         * @brief 브라우저와 동일한 에셋 목록입니다.
         */
        Editor::FEditorAssetCatalog* mAssetCatalog{};

        /**
         * @brief 브라우저와 동일한 썸네일 캐시입니다.
         */
        Editor::FAssetThumbnailCache* mThumbnails{};

        /**
         * @brief C# 프로젝트 생성과 빌드 상태를 제공하는 서비스의 비소유 주소입니다.
         */
        Editor::FScriptBuildService* mScriptBuilds{};

        /**
         * @brief 썸네일과 드롭다운을 표시하는 선택기입니다.
         */
        Editor::FAssetPicker mAssetPicker{};
        Core::Uint64 mSceneEditToken{};
        Core::Uint32 mSceneEditItem{};
    };

}
