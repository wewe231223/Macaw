/**
 * @file FEditorPanel.h
 * @brief ImGui 패널의 이름, 가시성과 내용 그리기 수명을 관리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

class FEditorContext;

/**
 * @brief ImGui 패널의 이름, 가시성과 내용 그리기 수명을 관리합니다.
 * @ingroup Editor
 */
class FEditorPanel {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FEditorPanel);

public:
    /**
     * @brief FEditorPanel의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in,out] Context 패널 또는 뷰포트가 공유할 편집 컨텍스트입니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in] Closable 사용자가 패널을 닫을 수 있는지 지정합니다.
     */
    FEditorPanel(FEditorContext* Context, FString Name, bool Closable = false);
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    virtual ~FEditorPanel();
    /**
     * @brief 고유한 객체 수명을 유지하기 위해 복사 또는 이동 생성을 금지합니다.
     */
    FEditorPanel(const FEditorPanel&) = delete;
    /**
     * @brief 고유한 객체 수명을 유지하기 위해 대입을 금지합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FEditorPanel& operator=(const FEditorPanel&) = delete;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FEditorPanel);

    /**
     * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
     * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
     */
    void Draw(float DeltaTime);

    /**
     * @brief 이름 또는 표시 문자열을 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const FString& GetName() const;
    /**
     * @brief 패널 창의 표시 여부를 반환합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsVisible() const;
    /**
     * @brief 패널 창의 표시 상태를 설정합니다.
     * @param[in] Visible 창이나 패널을 표시할지 지정합니다.
     */
    void SetVisible(bool Visible);

    /**
     * @brief 연결된 공통 편집 컨텍스트를 반환합니다.
     * @return 연결된 공통 편집 컨텍스트의 비소유 주소입니다.
     */
    FEditorContext* GetEditorContext() const;
    /**
     * @brief 패널이 사용할 편집 컨텍스트의 비소유 주소를 교체합니다.
     * @param[in] Context 조회 또는 직렬화에 사용할 컨텍스트의 비소유 주소입니다.
     */
    void SetEditorContext(FEditorContext* Context);

private:
    /**
     * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
     * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
     */
    virtual void DrawContent(float DeltaTime) = 0;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 연결된 공통 편집 컨텍스트입니다.
     */
    FEditorContext* mContext{};
    ReflectProperty(mContext, "Context", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이름 또는 표시 문자열입니다.
     */
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 패널 닫기 버튼을 표시할지 여부입니다.
     */
    bool mClosable{};
    ReflectProperty(mClosable, "Closable", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 패널 표시 여부입니다.
     */
    bool mVisible{true};
    ReflectProperty(mVisible, "Visible", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
