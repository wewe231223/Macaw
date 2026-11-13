/**
 * @file FAnimStateMachineData.h
 * @brief 상태별 포즈 그래프와 전이 조건을 보관하고 직렬화하는 상태 머신 에셋 데이터입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"

#include <memory>

namespace Core {
    class FArchive;
}

namespace Asset {

    class UAnimGraph;

    /**
     * @brief 전이 조건 그래프에서 사용할 상수·변수·비교·논리·재생 정보 노드 종류입니다.
     * @ingroup Asset
     */
    enum class EAnimRuleNodeType : Core::Uint8 {

        /**
         * @brief Float 상수를 출력합니다.
         */
        Float,

        /**
         * @brief Bool 상수를 출력합니다.
         */
        Bool,

        /**
         * @brief Float 그래프 변수를 읽습니다.
         */
        GetFloat,

        /**
         * @brief Bool 그래프 변수를 읽습니다.
         */
        GetBool,

        /**
         * @brief 첫 Float 입력이 두 번째보다 큰지 비교합니다.
         */
        Greater,

        /**
         * @brief 첫 Float 입력이 두 번째 이상인지 비교합니다.
         */
        GreaterEqual,

        /**
         * @brief 첫 Float 입력이 두 번째보다 작은지 비교합니다.
         */
        Less,

        /**
         * @brief 첫 Float 입력이 두 번째 이하인지 비교합니다.
         */
        LessEqual,

        /**
         * @brief 두 Float 입력이 같은지 비교합니다.
         */
        Equal,

        /**
         * @brief 두 Float 입력이 다른지 비교합니다.
         */
        NotEqual,

        /**
         * @brief 두 Bool 입력의 논리곱입니다.
         */
        And,

        /**
         * @brief 두 Bool 입력의 논리합입니다.
         */
        Or,

        /**
         * @brief Bool 입력을 반전합니다.
         */
        Not,

        /**
         * @brief 현재 상태의 경과 시간(초)을 읽습니다.
         */
        StateTime,

        /**
         * @brief 지정한 에셋 플레이어의 남은 재생 시간(초)을 읽습니다.
         */
        TimeRemaining,

        /**
         * @brief 지정한 에셋 플레이어의 재생 종료 여부를 읽습니다.
         */
        Finished,

        /**
         * @brief 유효한 조건 노드 종류 수이며 실제 노드로 사용하지 않습니다.
         */
        Count
    };

    /**
     * @brief 전이 조건 노드의 연산 종류, 입력 연결, 상수 값과 에디터 위치입니다.
     * @ingroup Asset
     */
    struct FAnimRuleNode {
    public:

        /**
         * @brief 소속 데이터 안에서 항목을 식별하는 ID입니다.
         */
        Core::Int32 mId{};

        /**
         * @brief 전이 조건 노드의 연산 종류입니다.
         */
        Asset::EAnimRuleNodeType mType{};

        /**
         * @brief 첫 번째 입력의 원본 노드 ID이며 미연결 시 -1입니다.
         */
        Core::Int32 mInputA{-1};

        /**
         * @brief 두 번째 입력의 원본 노드 ID이며 미연결 시 -1입니다.
         */
        Core::Int32 mInputB{-1};

        /**
         * @brief 변수 조회 또는 상태 플레이어 지정에 사용하는 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief Float 상수 노드의 값입니다.
         */
        float mValue{};

        /**
         * @brief Bool 상수 또는 런타임 변수 값입니다.
         */
        bool mBoolValue{};

        /**
         * @brief 노드의 에디터 X 좌표입니다.
         */
        float mX{};

        /**
         * @brief 노드의 에디터 Y 좌표입니다.
         */
        float mY{};
    };

    /**
     * @brief 형식이 일치하는 값 노드를 연결하여 Bool 전이 조건을 구성하는 그래프입니다.
     * @ingroup Asset
     */
    class FAnimTransitionGraph {
    public:

        /**
         * @brief 그래프 노드 목록을 읽기 전용으로 제공합니다.
         * @return 그래프가 소유한 노드 배열의 참조입니다.
         */
        const Core::TArray<Asset::FAnimRuleNode>& GetNodes() const;

        /**
         * @brief 노드 ID에 대응하는 항목을 찾습니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 내부 노드의 비소유 포인터이며 없으면 nullptr입니다.
         */
        Asset::FAnimRuleNode* FindNode(Core::Int32 Id);

        /**
         * @brief 노드 ID에 대응하는 항목을 찾습니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 내부 노드의 비소유 포인터이며 없으면 nullptr입니다.
         */
        const Asset::FAnimRuleNode* FindNode(Core::Int32 Id) const;

        /**
         * @brief 지정한 종류와 에디터 위치로 전이 조건 노드를 추가합니다.
         * @param Type 추가할 전이 조건 연산 종류입니다.
         * @param X 에디터 배치의 X 좌표입니다.
         * @param Y 에디터 배치의 Y 좌표입니다.
         * @return 새 노드 ID이며 종류·위치·용량이 유효하지 않으면 -1입니다.
         */
        Core::Int32 AddNode(Asset::EAnimRuleNodeType Type, float X, float Y);

        /**
         * @brief 노드를 제거하고 해당 노드를 참조하는 입력과 결과 지정을 해제합니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 해당 노드를 제거했으면 true입니다.
         */
        bool RemoveNode(Core::Int32 Id);

        /**
         * @brief 자료형과 순환 여부를 검증한 뒤 원본 노드를 대상 입력 핀에 연결합니다.
         * @param From 연결을 시작할 원본 항목 ID입니다.
         * @param To 연결할 대상 항목 ID입니다.
         * @param Pin 대상 입력 핀 번호이며 첫 입력은 0입니다.
         * @return 연결 가능한 노드·핀 조합이면 true입니다.
         */
        bool Connect(Core::Int32 From, Core::Int32 To, Core::Int32 Pin);

        /**
         * @brief 대상 노드의 지정 입력 연결을 해제합니다.
         * @param To 연결할 대상 항목 ID입니다.
         * @param Pin 대상 입력 핀 번호이며 첫 입력은 0입니다.
         * @return 실제로 연결을 해제했으면 true입니다.
         */
        bool Disconnect(Core::Int32 To, Core::Int32 Pin);

        /**
         * @brief Bool 노드를 전이 조건 출력으로 지정하거나 -1로 출력을 해제합니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 출력으로 지정할 수 있는 ID이면 true입니다.
         */
        bool SetResult(Core::Int32 Id);

        /**
         * @brief 전이 조건의 최종 출력 노드를 반환합니다.
         * @return Bool 결과 노드 ID이며 미지정 시 -1입니다.
         */
        Core::Int32 GetResult() const;

        /**
         * @brief 노드 ID, 자료형, 입력 연결과 순환 참조의 유효성을 검사합니다.
         * @return 모든 노드와 연결이 유효하면 true입니다.
         */
        bool Validate() const;

        /**
         * @brief 변수 Getter가 참조하는 이름을 일괄 변경합니다.
         * @param OldName 변경 전 변수 이름입니다.
         * @param NewName 변경 후 변수 이름입니다.
         */
        void RenameVariable(const Core::FString& OldName, const Core::FString& NewName);

        /**
         * @brief 아카이브 방향에 따라 데이터를 저장하거나 읽고 로드한 연결을 검증합니다.
         * @param Archive 저장·로드 방향과 오류 상태를 제공하는 아카이브입니다.
         */
        void Serialize(Core::FArchive& Archive);

        /**
         * @brief 노드의 출력 자료형이 Bool인지 확인합니다.
         * @param Type 검사할 전이 조건 노드 종류입니다.
         * @return Bool 결과를 만드는 종류이면 true입니다.
         */
        static bool IsBool(Asset::EAnimRuleNodeType Type);

        /**
         * @brief 노드 종류에 필요한 입력 핀 수를 반환합니다.
         * @param Type 검사할 전이 조건 노드 종류입니다.
         * @return 단항 연산은 1, 이항 연산은 2, 입력 없는 노드는 0입니다.
         */
        static Core::Int32 GetInputCount(Asset::EAnimRuleNodeType Type);

        /**
         * @brief 노드 입력이 Bool 값을 요구하는지 확인합니다.
         * @param Type 검사할 전이 조건 노드 종류입니다.
         * @return And·Or·Not 노드이면 true입니다.
         */
        static bool IsBoolInput(Asset::EAnimRuleNodeType Type);

        /**
         * @brief 전이 조건 노드 종류의 표시 이름을 반환합니다.
         * @param Type 표시 이름을 조회할 전이 조건 연산 종류입니다.
         * @return 정적 문자열이며 알 수 없는 종류는 Unknown입니다.
         */
        static const char* GetTitle(Asset::EAnimRuleNodeType Type);

    private:

        /**
         * @brief 입력 연결을 따라 지정한 노드에 도달하는지 검사합니다.
         * @param From 연결을 시작할 원본 항목 ID입니다.
         * @param To 연결할 대상 항목 ID입니다.
         * @return From에서 입력을 추적하여 To에 도달하면 true입니다.
         */
        bool Reaches(Core::Int32 From, Core::Int32 To) const;

    private:

        /**
         * @brief 그래프가 보관하는 노드 목록입니다.
         */
        Core::TArray<Asset::FAnimRuleNode> mNodes{};

        /**
         * @brief 다음 노드에 부여할 ID입니다.
         */
        Core::Int32 mNextId{};

        /**
         * @brief 최종 Bool 조건을 제공하는 노드 ID이며 미지정 시 -1입니다.
         */
        Core::Int32 mResult{-1};
    };

    /**
     * @brief 상태의 식별자, 이름, 재진입 정책과 소유한 포즈 그래프입니다.
     * @ingroup Asset
     */
    struct FAnimStateData {
    public:

        /**
         * @brief 소속 데이터 안에서 항목을 식별하는 ID입니다.
         */
        Core::Int32 mId{};

        /**
         * @brief 상태 머신 안에서 중복되지 않는 상태 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 노드의 에디터 X 좌표입니다.
         */
        float mX{};

        /**
         * @brief 노드의 에디터 Y 좌표입니다.
         */
        float mY{};

        /**
         * @brief 상태에 다시 진입할 때 내부 포즈 그래프를 초기화할지 여부입니다.
         */
        bool mResetOnEntry{true};

        /**
         * @brief 이 상태의 포즈를 생성하는 공유 소유 그래프입니다.
         */
        std::shared_ptr<Asset::UAnimGraph> mGraph{};
    };

    /**
     * @brief 출발·도착 상태, 블렌드 시간, 우선순위와 전이 조건입니다.
     * @ingroup Asset
     */
    struct FAnimTransitionData {
    public:

        /**
         * @brief 소속 데이터 안에서 항목을 식별하는 ID입니다.
         */
        Core::Int32 mId{};

        /**
         * @brief 전이가 시작되는 상태 ID입니다.
         */
        Core::Int32 mFromState{};

        /**
         * @brief 전이가 도착하는 상태 ID입니다.
         */
        Core::Int32 mToState{};

        /**
         * @brief 상태 전이 블렌드 시간(초)입니다.
         */
        float mDuration{0.2f};

        /**
         * @brief 동시에 성립한 전이 사이에서 사용할 우선순위 값입니다.
         */
        Core::Int32 mPriority{};

        /**
         * @brief 전이 가능 여부를 계산하는 Bool 조건 그래프입니다.
         */
        Asset::FAnimTransitionGraph mRule{};
    };

    /**
     * @brief 상태별 포즈 그래프와 전이 조건을 보관하고 직렬화하는 상태 머신 에셋 데이터입니다.
     * @ingroup Asset
     */
    class FAnimStateMachineData {
    public:

        /**
         * @brief 단일 상태 머신에서 허용하는 최대 상태 수입니다.
         */
        static constexpr Core::Uint32 MaxStates{254};

        /**
         * @brief 상태 목록을 읽기 전용으로 제공합니다.
         * @return 객체가 소유한 상태 배열의 참조입니다.
         */
        const Core::TArray<Asset::FAnimStateData>& GetStates() const;

        /**
         * @brief 전이 목록을 읽기 전용으로 제공합니다.
         * @return 객체가 소유한 전이 배열의 참조입니다.
         */
        const Core::TArray<Asset::FAnimTransitionData>& GetTransitions() const;

        /**
         * @brief 상태 ID에 대응하는 항목을 찾습니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 내부 상태의 비소유 포인터이며 없으면 nullptr입니다.
         */
        Asset::FAnimStateData* FindState(Core::Int32 Id);

        /**
         * @brief 상태 ID에 대응하는 항목을 찾습니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 내부 상태의 비소유 포인터이며 없으면 nullptr입니다.
         */
        const Asset::FAnimStateData* FindState(Core::Int32 Id) const;

        /**
         * @brief 전이 ID에 대응하는 항목을 찾습니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 내부 전이의 비소유 포인터이며 없으면 nullptr입니다.
         */
        Asset::FAnimTransitionData* FindTransition(Core::Int32 Id);

        /**
         * @brief 전이 ID에 대응하는 항목을 찾습니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 내부 전이의 비소유 포인터이며 없으면 nullptr입니다.
         */
        const Asset::FAnimTransitionData* FindTransition(Core::Int32 Id) const;

        /**
         * @brief 이름이 중복되지 않는 상태와 독립 포즈 그래프를 만들고 소유 그래프의 변수를 복사합니다.
         * @param Name 조회하거나 표시할 이름입니다.
         * @param X 에디터 배치의 X 좌표입니다.
         * @param Y 에디터 배치의 Y 좌표입니다.
         * @param Owner 새 상태에 복사할 변수 정의를 가진 소유 그래프입니다.
         * @return 추가한 상태 ID이며 이름·위치·용량이 유효하지 않으면 -1입니다.
         */
        Core::Int32 AddState(const Core::FString& Name, float X, float Y, const Asset::UAnimGraph& Owner);

        /**
         * @brief 상태와 연결된 전이를 제거하고 필요하면 진입 상태를 다시 지정합니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 해당 상태를 제거했으면 true입니다.
         */
        bool RemoveState(Core::Int32 Id);

        /**
         * @brief 존재하는 두 상태 사이에 기본 블렌드 설정으로 전이를 추가합니다.
         * @param From 연결을 시작할 원본 항목 ID입니다.
         * @param To 연결할 대상 항목 ID입니다.
         * @return 추가한 전이 ID이며 상태·용량이 유효하지 않으면 -1입니다.
         */
        Core::Int32 AddTransition(Core::Int32 From, Core::Int32 To);

        /**
         * @brief 지정한 ID의 전이를 제거합니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 해당 전이를 제거했으면 true입니다.
         */
        bool RemoveTransition(Core::Int32 Id);

        /**
         * @brief 존재하는 상태를 초기 진입 상태로 지정합니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 상태 ID가 유효하면 true입니다.
         */
        bool SetEntryState(Core::Int32 Id);

        /**
         * @brief 초기 진입 상태 ID를 반환합니다.
         * @return 진입 상태 ID이며 상태가 없으면 -1입니다.
         */
        Core::Int32 GetEntryState() const;

        /**
         * @brief 원본의 데이터를 복사하고 중첩 포즈 그래프를 독립 복제합니다.
         * @param Source 복사하거나 읽을 원본 데이터입니다.
         */
        void CopyFrom(const Asset::FAnimStateMachineData& Source);

        /**
         * @brief 아카이브 방향에 따라 데이터를 저장하거나 읽고 로드한 연결을 검증합니다.
         * @param Archive 저장·로드 방향과 오류 상태를 제공하는 아카이브입니다.
         */
        void Serialize(Core::FArchive& Archive);

    private:

        /**
         * @brief 상태 목록입니다.
         */
        Core::TArray<Asset::FAnimStateData> mStates{};

        /**
         * @brief 상태 사이의 전이 목록입니다.
         */
        Core::TArray<Asset::FAnimTransitionData> mTransitions{};

        /**
         * @brief 초기 진입 상태 ID이며 상태가 없으면 -1입니다.
         */
        Core::Int32 mEntryState{-1};

        /**
         * @brief 다음 상태에 부여할 단조 증가 ID입니다.
         */
        Core::Int32 mNextStateId{};

        /**
         * @brief 다음 전이에 부여할 단조 증가 ID입니다.
         */
        Core::Int32 mNextTransitionId{};
    };

}
