/**
 * @file FDetailsPanel.h
 * @brief 선택한 객체의 리플렉션 프로퍼티를 표시하고 편집합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Editor/Panel/FEditorPanel.h"
#include "CoreUObject/Reflection/FPropertyValue.h"

class FProperty;
class UObject;

/**
 * @brief 선택한 객체의 리플렉션 프로퍼티를 표시하고 편집합니다.
 * @ingroup Editor
 */
class FDetailsPanel final : public FEditorPanel {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FDetailsPanel, FEditorPanel);

public:
    /**
     * @brief FDetailsPanel의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in,out] Context 패널 또는 뷰포트가 공유할 편집 컨텍스트입니다.
     */
    explicit FDetailsPanel(FEditorContext* Context);
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FDetailsPanel() override;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FDetailsPanel);

private:
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
     * @brief 객체 타입의 편집 가능한 리플렉션 프로퍼티를 순회하여 표시합니다.
     * @param[in,out] Object 대상 객체입니다.
     */
    void DrawReflectedObject(UObject* Object);
    /**
     * @brief 프로퍼티 타입에 맞는 편집 위젯을 표시하고 변경한 값을 돌려줍니다.
     * @param[in] Property 읽기, 쓰기 또는 알림을 적용할 프로퍼티 메타데이터입니다.
     * @param[in,out] Value 프로퍼티 타입에 대응하는 variant 값입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool DrawPropertyValue(const FProperty* Property, FPropertyValue& Value);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    std::array<char, 128> mComponentSearch{};
    ReflectProperty(mComponentSearch, "ComponentSearch", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
