/**
 * @file FStatisticsPanel.h
 * @brief 프레임 시간, 객체, 메모리와 렌더링 통계를 표시합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Editor/Panel/FEditorPanel.h"

/**
 * @brief 프레임 시간, 객체, 메모리와 렌더링 통계를 표시합니다.
 * @ingroup Editor
 */
class FStatisticsPanel final : public FEditorPanel {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FStatisticsPanel, FEditorPanel);

public:
    /**
     * @brief FStatisticsPanel의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in,out] Context 패널 또는 뷰포트가 공유할 편집 컨텍스트입니다.
     */
    explicit FStatisticsPanel(FEditorContext* Context);
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FStatisticsPanel() override;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FStatisticsPanel);

private:
    /**
     * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
     * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
     */
    void DrawContent(float DeltaTime) override;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();
};
