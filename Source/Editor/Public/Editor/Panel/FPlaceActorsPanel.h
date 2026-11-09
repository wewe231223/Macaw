/**
 * @file FPlaceActorsPanel.h
 * @brief 검색과 분류를 통해 액터 프리셋을 배치하는 에디터 패널입니다.
 * @ingroup Editor
 */

#pragma once

#include "Editor/Panel/FEditorPanel.h"

#include <d3d11.h>
#include <wrl/client.h>

namespace Editor {

    /**
     * @brief 액터 배치 목록의 캐시 아이콘 종류입니다.
     */
    enum class EPlaceActorIcon : Core::Uint8 {
        /**
         * @brief 해당 값이나 대상을 지정하지 않은 상태입니다.
         */
        None,

        /**
         * @brief 빌보드 액터 아이콘입니다.
         */
        Billboard,

        /**
         * @brief 주변광 액터 아이콘입니다.
         */
        AmbientLight,

        /**
         * @brief 방향성 광원 액터 아이콘입니다.
         */
        DirectionalLight,

        /**
         * @brief 점 광원 액터 아이콘입니다.
         */
        PointLight,

        /**
         * @brief 스포트라이트 액터 아이콘입니다.
         */
        SpotLight,

        /**
         * @brief 높이 안개 액터 아이콘입니다.
         */
        HeightFog,

        /**
         * @brief 아이콘 캐시 크기에 사용할 종류 수입니다.
         */
        Count
    };

    /**
     * @brief 검색과 분류를 통해 액터 프리셋을 배치하는 에디터 패널입니다.
     * @ingroup Editor
     */
    class FPlaceActorsPanel final : public Editor::FEditorPanel {
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
         * @brief 액터 배치 아이콘을 다음 갱신 시 다시 생성하도록 표시합니다.
         */
        void InvalidateIcons();

        /**
         * @brief 액터 배치 목록의 캐시된 아이콘 텍스처를 해제합니다.
         */
        void ClearIcons();

    private:
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
         * @param[in] Icon 항목에 표시할 아이콘 종류입니다.
         */
        void DrawEntry(const Core::FString& Preset, const Core::FString& Label, const char* Components, Editor::EPlaceActorIcon Icon);

        /**
         * @brief 프로젝트 에셋과 렌더러에서 액터 배치 아이콘을 준비합니다.
         */
        void UpdateIcons();

    private:
        /**
         * @brief 액터 프리셋의 검색 문자열 버퍼입니다.
         */
        std::array<char, 128> mSearch{};

        /**
         * @brief 배치 패널에서 선택한 액터 분류 인덱스입니다.
         */
        Core::Int32 mCategory{};

        /**
         * @brief 배치 목록의 종류별 아이콘 텍스처 캐시입니다.
         */
        std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, static_cast<std::size_t>(Editor::EPlaceActorIcon::Count)> mIcons{};

        /**
         * @brief 배치 아이콘 캐시의 재생성 필요 여부입니다.
         */
        bool mIconsDirty{true};
    };

}
