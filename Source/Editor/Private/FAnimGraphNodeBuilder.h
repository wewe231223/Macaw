/**
 * @file FAnimGraphNodeBuilder.h
 * @brief 애니메이션 그래프 노드를 블루프린트 모양(색 헤더, 가장자리 핀, 옆으로 나가는 링크)으로 그리는 도우미입니다.
 * @ingroup Editor
 */

#pragma once

#include <imgui.h>
#include <imgui_node_editor.h>

namespace Editor {

    /**
     * @brief 노드 하나를 헤더 → 핀 행 → 본문 순서로 그립니다.
     * @details 언리얼 블루프린트처럼 헤더를 노드 색으로 칠하고, 입력 핀은 왼쪽 가장자리, 출력 핀은 오른쪽 가장자리에 아이콘으로 놓습니다.
     * 링크는 핀 아이콘 중심에서 옆으로 나가므로 노드 본문을 가리지 않습니다. 출력 핀은 첫 핀 행의 오른쪽에 둡니다.
     *
     * 사용 순서: Begin → Header → (InputPin / OutputPin으로 핀 행) → 본문 위젯 → End.
     * 노드 에디터 캔버스 안의 위치는 캔버스 창 기준이므로, 오른쪽 정렬은 노드 시작 X에서 계산합니다.
     */
    class FAnimGraphNodeBuilder {
    public:

        /**
         * @brief 노드 제목과 종류 표시가 들어가도록 필요한 너비를 반영합니다.
         * @param Title 노드 머리글의 제목입니다.
         * @param Type 노드 머리글에 표시할 종류 이름입니다.
         */
        void FitHeader(const char* Title, const char* Type);

        /**
         * @brief 텍스트 너비와 여백을 노드의 요구 너비에 반영합니다.
         * @param Text 표시하거나 크기를 계산할 텍스트입니다.
         * @param Padding 텍스트 너비에 추가할 여백(픽셀)입니다.
         */
        void FitText(const char* Text, float Padding = 0.0f);

        /**
         * @brief 입출력 핀 이름과 아이콘이 들어가도록 노드 너비를 반영합니다.
         * @param Input 입력 핀 표시 이름입니다.
         * @param Output 출력 핀 표시 이름이며 없으면 nullptr입니다.
         */
        void FitPins(const char* Input, const char* Output = nullptr);

        /**
         * @brief 속성 이름·값 열의 최대 너비를 갱신합니다.
         * @param Label UI 항목의 표시 이름입니다.
         * @param Value 표시·비교하거나 적용할 값입니다.
         */
        void FitProperty(const char* Label, const char* Value);

        /**
         * @brief 노드를 시작합니다.
         * @param[in] Id 노드 식별자입니다.
         * @param[in] MinWidth 노드 내용의 최소 너비이며 픽셀 단위입니다.
         */
        void Begin(ax::NodeEditor::NodeId Id, float MinWidth);

        /**
         * @brief 헤더에 제목과 부제를 그리고 헤더 색을 정합니다.
         * @param[in] Title 노드 이름입니다.
         * @param[in] Type 노드 종류이며 제목과 다르면 부제로 표시합니다.
         */
        void Header(const char* Title, const char* Type);

        /**
         * @brief 왼쪽 가장자리에 입력 핀 하나를 한 줄로 그립니다.
         * @param[in] Id 핀 식별자입니다.
         * @param[in] Label 핀 이름입니다.
         * @param[in] Connected 연결되어 있으면 아이콘을 채웁니다.
         * @param[in] Color 핀 아이콘 색입니다.
         */
        void InputPin(ax::NodeEditor::PinId Id, const char* Label, bool Connected, const ImVec4& Color);

        /**
         * @brief 오른쪽 가장자리에 출력 핀을 그립니다.
         * @param[in] Id 핀 식별자입니다.
         * @param[in] Label 핀 이름입니다.
         * @param[in] Connected 연결되어 있으면 아이콘을 채웁니다.
         * @param[in] Color 핀 아이콘 색입니다.
         * @param[in] SameLine 바로 앞 입력 핀과 같은 줄에 놓으려면 true입니다.
         */
        void OutputPin(ax::NodeEditor::PinId Id, const char* Label, bool Connected, const ImVec4& Color, bool SameLine);

        /**
         * @brief 핀 행과 본문 사이에 여백을 둡니다.
         */
        void BeginBody();

        /**
         * @brief 노드 안에 지정한 색상으로 텍스트를 표시합니다.
         * @param Text 표시하거나 크기를 계산할 텍스트입니다.
         * @param Color 도형 또는 텍스트의 표시 색상입니다.
         */
        void DrawText(const char* Text, const ImVec4& Color) const;

        /**
         * @brief 노드의 속성 이름과 현재 값을 정렬하여 표시합니다.
         * @param Label UI 항목의 표시 이름입니다.
         * @param Value 표시·비교하거나 적용할 값입니다.
         * @param Bound 변수가 연결된 속성인지 여부입니다.
         */
        void DrawProperty(const char* Label, const char* Value, bool Bound = false) const;

        /**
         * @brief 노드를 끝내고 헤더 배경을 그립니다.
         */
        void End();

        /**
         * @brief 노드 내용의 너비를 반환합니다.
         * @return 픽셀 단위 너비입니다.
         */
        float GetWidth() const;

        /**
         * @brief 포즈 핀의 기본 색입니다.
         * @return 흰색에 가까운 색입니다.
         */
        static ImVec4 PoseColor();

        /**
         * @brief Float 값 핀에 사용할 색상을 반환합니다.
         * @return Float 핀 표시 색상입니다.
         */
        static ImVec4 FloatColor();

        /**
         * @brief 변수 이름·현재 값과 출력 핀을 가진 Float Getter 노드를 표시합니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @param Pin 노드 편집기 출력 핀 ID입니다.
         * @param Name 조회하거나 표시할 이름입니다.
         * @param Value 표시·비교하거나 적용할 값입니다.
         * @param Connected 핀에 연결이 있는지 여부입니다.
         */
        static void DrawFloatGetter(ax::NodeEditor::NodeId Node, ax::NodeEditor::PinId Pin, const char* Name, const char* Value, bool Connected);

        /**
         * @brief 현재 노드 에디터에 블루프린트 비슷한 스타일을 적용합니다. SetCurrentEditor 뒤, Begin 전에 호출합니다.
         */
        static void ApplyStyle();

    private:

        /**
         * @brief 핀 아이콘 자리를 차지하고, 핀 위치와 링크 시작점을 아이콘으로 지정한 뒤 아이콘을 그립니다.
         * @param[in] Connected 연결되어 있으면 아이콘을 채웁니다.
         * @param[in] Color 아이콘 색입니다.
         */
        void PinIcon(bool Connected, const ImVec4& Color);

    private:

        /**
         * @brief 그리고 있는 노드입니다.
         */
        ax::NodeEditor::NodeId mId{};

        /**
         * @brief 노드 내용의 너비입니다.
         */
        float mWidth{};

        /**
         * @brief 속성 이름 열의 계산된 픽셀 너비입니다.
         */
        float mLabelWidth{};

        /**
         * @brief 속성 값 열의 계산된 픽셀 너비입니다.
         */
        float mValueWidth{};

        /**
         * @brief 노드 내용이 시작하는 캔버스 X 위치입니다.
         */
        float mStartX{};

        /**
         * @brief 헤더 아래 끝의 캔버스 Y 위치입니다.
         */
        float mHeaderBottom{};

        /**
         * @brief 헤더 색입니다.
         */
        ImVec4 mHeaderColor{};
    };

}
