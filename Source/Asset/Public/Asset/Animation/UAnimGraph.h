/**
 * @file UAnimGraph.h
 * @brief 에디터에서 노드를 배치하고 연결해 만드는 애니메이션 그래프 에셋입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "Core/FRevision.h"
#include "Asset/Animation/FAnimStateMachineData.h"
#include "CoreUObject/UObject.h"

#include "UAnimGraph.generated.h"

namespace Asset {

    /**
     * @brief 노드 파라미터 하나의 이름과 문자열 값입니다.
     * @details 값의 해석(실수·참거짓·에셋 경로)은 노드 종류가 정합니다. 이름이 "X.Binding"처럼 ".Binding"으로 끝나면 그 파라미터를 묶을 그래프 변수 이름입니다.
     */
    struct FAnimGraphParameter {
    public:

        /**
         * @brief 파라미터 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 문자열로 저장한 값입니다.
         */
        Core::FString mValue{};

        /**
         * @brief 파라미터에 연결한 Getter ID이며 연결이 없으면 -1입니다.
         */
        Core::Int32 mGetterId{-1};
    };

    /**
     * @brief 그래프에 배치한 노드 하나입니다.
     */
    struct FAnimGraphNode {
    public:

        /**
         * @brief 파라미터 값을 찾습니다.
         * @param[in] Name 파라미터 이름입니다.
         * @return 없으면 nullptr입니다.
         */
        const Core::FString* FindParameter(const Core::FString& Name) const;

        /**
         * @brief 파라미터 값을 설정합니다. 값이 비어 있으면 파라미터를 지웁니다.
         * @param[in] Name 파라미터 이름입니다.
         * @param[in] Value 새 값입니다.
         * @param GetterId 연결할 변수 Getter ID이며 -1은 상수 파라미터입니다.
         */
        void SetParameter(const Core::FString& Name, const Core::FString& Value, Core::Int32 GetterId = -1);

    public:

        /**
         * @brief 그래프 안에서 고유한 0 이상의 식별자입니다. 링크가 이 값으로 노드를 가리킵니다.
         */
        Core::Int32 mId{};

        /**
         * @brief 노드 종류 이름입니다. 예를 들어 "SequencePlayer"입니다. 엔진의 노드 종류 목록이 해석합니다.
         */
        Core::FString mType{};

        /**
         * @brief 표시 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 노드 에디터 캔버스의 X 위치입니다.
         */
        float mX{};

        /**
         * @brief 노드 에디터 캔버스의 Y 위치입니다.
         */
        float mY{};

        /**
         * @brief 파라미터 목록입니다.
         */
        Core::TArray<Asset::FAnimGraphParameter> mParameters{};
    };

    /**
     * @brief 한 노드의 출력 포즈를 다른 노드의 포즈 입력 핀에 연결합니다.
     */
    struct FAnimGraphLink {
    public:

        /**
         * @brief 포즈를 내보내는 노드의 식별자입니다.
         */
        Core::Int32 mFromNode{};

        /**
         * @brief 포즈를 받는 노드의 식별자이며, UAnimGraph::OutputNodeId이면 그래프의 최종 포즈입니다.
         */
        Core::Int32 mToNode{};

        /**
         * @brief 받는 노드의 포즈 입력 핀 순서입니다.
         */
        Core::Index_t mToPin{};
    };

    /**
     * @brief 그래프 변수의 값이 어디서 오는지 나타냅니다.
     */
    enum class EAnimGraphVariableSource : Core::Uint8 {

        /**
         * @brief 기본값으로 시작하고 코드나 디버거가 바꿉니다.
         */
        Manual,

        /**
         * @brief 매 갱신마다 소유 컴포넌트의 수평 속력으로 채웁니다.
         */
        OwnerSpeed
    };

    /**
     * @brief 노드 파라미터를 묶을 수 있는 실수 변수입니다.
     */
    enum class EAnimGraphVariableType : Core::Uint8 {

        /**
         * @brief 부동소수점 변수입니다.
         */
        Float,

        /**
         * @brief 논리값 변수입니다.
         */
        Bool
    };

    /**
     * @brief 그래프 변수의 이름, 형식, 기본값과 외부 값 공급 방식을 정의합니다.
     * @ingroup Asset
     */
    struct FAnimGraphVariable {
    public:

        /**
         * @brief 변수 이름입니다. 그래프 안에서 고유합니다.
         */
        Core::FString mName{};

        /**
         * @brief 시작 값입니다.
         */
        float mDefault{};

        /**
         * @brief 값의 출처입니다.
         */
        Asset::EAnimGraphVariableSource mSource{Asset::EAnimGraphVariableSource::Manual};

        /**
         * @brief 그래프 변수의 Float 또는 Bool 형식입니다.
         */
        Asset::EAnimGraphVariableType mType{Asset::EAnimGraphVariableType::Float};

        /**
         * @brief Bool 변수의 기본값입니다.
         */
        bool mBoolDefault{};
    };

    /**
     * @brief 그래프 변수 이름과 값을 읽는 노드의 식별자·에디터 위치입니다.
     * @ingroup Asset
     */
    struct FAnimGraphVariableGetter {
    public:

        /**
         * @brief 소속 데이터 안에서 항목을 식별하는 ID입니다.
         */
        Core::Int32 mId{};

        /**
         * @brief 읽을 그래프 변수 이름입니다.
         */
        Core::FString mVariable{};

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
     * @brief 노드·링크·변수로 이루어진 애니메이션 그래프 에셋입니다.
     * @details 언리얼 애니메이션 블루프린트의 AnimGraph에 대응하는 데이터만 담습니다. 노드 종류의 의미는 엔진의 FAnimGraphNodeRegistry가 정하고,
     * UAnimGraphInstance가 이 데이터로 실행할 노드를 만듭니다. 머티리얼처럼 에디터가 캐시된 객체를 직접 고치는 에셋입니다.
     *
     * Output 노드는 저장하지 않는 고정 노드이며 식별자는 OutputNodeId입니다. 포즈 입력 핀 하나에는 링크가 하나만 연결되고, 순환 연결은 허용하지 않습니다.
     */
    UCLASS()
    class UAnimGraph final : public CoreUObject::UObject {
        GENERATED_BODY()

    public:

        /**
         * @brief 최종 포즈를 받는 Output 노드의 식별자입니다.
         */
        static constexpr Core::Int32 OutputNodeId{-1};

        /**
         * @brief 한 그래프가 가질 수 있는 최대 노드 수입니다.
         */
        static constexpr Core::Uint32 MaxNodes{256};

    public:

        /**
         * @brief UAnimGraph의 초기 상태와 의존성을 구성합니다.
         */
        UAnimGraph() = default;

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UAnimGraph() override = default;

    public:

        /**
         * @brief 그래프 노드 목록을 읽기 전용으로 제공합니다.
         * @return 그래프가 소유한 노드 배열의 참조입니다.
         */
        const Core::TArray<Asset::FAnimGraphNode>& GetNodes() const;

        /**
         * @brief 포즈 노드 사이의 연결 목록을 읽기 전용으로 제공합니다.
         * @return 그래프가 소유한 연결 배열의 참조입니다.
         */
        const Core::TArray<Asset::FAnimGraphLink>& GetLinks() const;

        /**
         * @brief 그래프 변수 정의 목록을 반환합니다.
         * @return 그래프 소유 배열의 읽기 전용 참조입니다.
         */
        const Core::TArray<Asset::FAnimGraphVariable>& GetVariables() const;

        /**
         * @brief 식별자로 노드를 찾습니다.
         * @param[in] Id 노드 식별자입니다.
         * @return 없으면 nullptr입니다.
         */
        Asset::FAnimGraphNode* FindNode(Core::Int32 Id);

        /**
         * @brief 노드 ID에 대응하는 항목을 찾습니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 내부 노드의 비소유 포인터이며 없으면 nullptr입니다.
         */
        const Asset::FAnimGraphNode* FindNode(Core::Int32 Id) const;

        /**
         * @brief 노드를 추가합니다.
         * @param[in] Type 노드 종류 이름입니다.
         * @param[in] Name 표시 이름입니다.
         * @param[in] X 캔버스 X 위치입니다.
         * @param[in] Y 캔버스 Y 위치입니다.
         * @return 새 노드의 식별자이며, 노드 수가 한도에 도달했으면 -2입니다.
         */
        Core::Int32 AddNode(const Core::FString& Type, const Core::FString& Name, float X, float Y);

        /**
         * @brief 노드와 그 노드에 이어진 링크를 지웁니다.
         * @param[in] Id 노드 식별자입니다.
         * @return 지웠으면 true입니다.
         */
        bool RemoveNode(Core::Int32 Id);

        /**
         * @brief 한 노드의 출력을 다른 노드의 입력 핀에 연결합니다. 핀에 이미 링크가 있으면 바꿉니다.
         * @param[in] FromNode 출력 노드입니다.
         * @param[in] ToNode 입력 노드이거나 OutputNodeId입니다.
         * @param[in] ToPin 입력 핀 순서입니다.
         * @return 노드가 있고 순환이 생기지 않으면 true입니다.
         */
        bool Connect(Core::Int32 FromNode, Core::Int32 ToNode, Core::Index_t ToPin);

        /**
         * @brief 입력 핀의 링크를 지웁니다.
         * @param[in] ToNode 입력 노드이거나 OutputNodeId입니다.
         * @param[in] ToPin 입력 핀 순서입니다.
         * @return 지웠으면 true입니다.
         */
        bool Disconnect(Core::Int32 ToNode, Core::Index_t ToPin);

        /**
         * @brief 연결하면 순환이 생기는지 검사합니다.
         * @param[in] FromNode 출력 노드입니다.
         * @param[in] ToNode 입력 노드입니다.
         * @return 순환이 생기면 true입니다.
         */
        bool WouldCreateCycle(Core::Int32 FromNode, Core::Int32 ToNode) const;

        /**
         * @brief 입력 핀에 연결된 노드를 찾습니다.
         * @param[in] ToNode 입력 노드이거나 OutputNodeId입니다.
         * @param[in] ToPin 입력 핀 순서입니다.
         * @return 연결된 노드 식별자이며 없으면 OutputNodeId입니다.
         */
        Core::Int32 FindInput(Core::Int32 ToNode, Core::Index_t ToPin) const;

        /**
         * @brief 노드의 위치를 바꿉니다.
         * @param[in] Id 노드 식별자이거나 OutputNodeId입니다.
         * @param[in] X 캔버스 X 위치입니다.
         * @param[in] Y 캔버스 Y 위치입니다.
         */
        void SetNodePosition(Core::Int32 Id, float X, float Y);

        /**
         * @brief Output 노드의 X 위치를 반환합니다.
         * @return 캔버스 X 위치입니다.
         */
        float GetOutputX() const;

        /**
         * @brief Output 노드의 Y 위치를 반환합니다.
         * @return 캔버스 Y 위치입니다.
         */
        float GetOutputY() const;

        /**
         * @brief 변수를 추가합니다.
         * @param[in] Variable 추가할 변수이며 이름이 비어 있지 않고 겹치지 않아야 합니다.
         * @return 추가했으면 true입니다.
         */
        bool AddVariable(const Asset::FAnimGraphVariable& Variable);

        /**
         * @brief 변수를 바꿉니다. 이름을 바꾸면 그 변수를 묶은 파라미터도 새 이름으로 바꿉니다.
         * @param[in] Index 변수 순서입니다.
         * @param[in] Variable 새 값이며 이름이 비어 있지 않고 다른 변수와 겹치지 않아야 합니다.
         * @return 적용했으면 true입니다.
         */
        bool SetVariable(Core::Index_t Index, const Asset::FAnimGraphVariable& Variable);

        /**
         * @brief 변수를 지우고, 그 변수를 묶은 파라미터의 바인딩을 해제합니다.
         * @param[in] Index 변수 순서입니다.
         * @return 지웠으면 true입니다.
         */
        bool RemoveVariable(Core::Index_t Index);

        /**
         * @brief 값 연결에 사용하는 변수 Getter 목록을 반환합니다.
         * @return 그래프 소유 배열의 읽기 전용 참조입니다.
         */
        const Core::TArray<Asset::FAnimGraphVariableGetter>& GetVariableGetters() const;

        /**
         * @brief ID로 변수 Getter를 찾습니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 그래프 소유의 Getter이며 없으면 nullptr입니다.
         */
        const Asset::FAnimGraphVariableGetter* FindVariableGetter(Core::Int32 Id) const;

        /**
         * @brief 노드 파라미터의 연결 정보로 변수 Getter를 찾습니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @param Parameter 조회하거나 연결할 파라미터 이름 또는 인덱스입니다.
         * @return 원본 그래프 소유의 Getter이며 연결이 없거나 유효하지 않으면 nullptr입니다.
         */
        const Asset::FAnimGraphVariableGetter* FindBoundGetter(const Asset::FAnimGraphNode& Node, const Core::FString& Parameter) const;

        /**
         * @brief 존재하는 그래프 변수를 읽는 Getter를 지정 위치에 추가합니다.
         * @param Variable 참조할 그래프 변수 이름입니다.
         * @param X 에디터 배치의 X 좌표입니다.
         * @param Y 에디터 배치의 Y 좌표입니다.
         * @return Getter ID이며 변수·위치·용량이 유효하지 않으면 -1입니다.
         */
        Core::Int32 AddVariableGetter(const Core::FString& Variable, float X, float Y);

        /**
         * @brief Getter의 에디터 표시 위치를 변경합니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @param X 에디터 배치의 X 좌표입니다.
         * @param Y 에디터 배치의 Y 좌표입니다.
         * @return Getter와 좌표가 유효하면 true입니다.
         */
        bool SetVariableGetterPosition(Core::Int32 Id, float X, float Y);

        /**
         * @brief Getter와 이를 참조하는 파라미터 연결을 제거합니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 지정한 Getter를 제거했으면 true입니다.
         */
        bool RemoveVariableGetter(Core::Int32 Id);

        /**
         * @brief 다른 그래프의 내용으로 바꿉니다.
         * @param[in] Source 복사할 그래프입니다.
         */
        void CopyFrom(const Asset::UAnimGraph& Source);

        /**
         * @brief 원본 변경 감지에 사용하는 리비전을 반환합니다.
         * @return 현재 그래프 리비전입니다.
         */
        Core::FRevision GetRevision() const;

        /**
         * @brief 상태 머신 노드 ID의 중첩 데이터를 찾습니다.
         * @param NodeId 원본 그래프 노드 ID입니다.
         * @return 그래프가 소유하는 데이터이며 없으면 nullptr입니다.
         */
        Asset::FAnimStateMachineData* FindStateMachine(Core::Int32 NodeId);

        /**
         * @brief 상태 머신 노드 ID의 중첩 데이터를 찾습니다.
         * @param NodeId 원본 그래프 노드 ID입니다.
         * @return 그래프가 소유하는 데이터이며 없으면 nullptr입니다.
         */
        const Asset::FAnimStateMachineData* FindStateMachine(Core::Int32 NodeId) const;

        /**
         * @brief 상위 그래프의 변수 정의를 중첩 그래프와 일치시킵니다.
         * @param Variables 상위 그래프와 공유할 변수 정의 목록입니다.
         */
        void SynchronizeVariables(const Core::TArray<Asset::FAnimGraphVariable>& Variables);

        /**
         * @brief 노드·링크·변수를 읽거나 저장합니다.
         * @param[in,out] Archive 저장하거나 읽을 아카이브입니다.
         */
        void Serialize(Core::FArchive& Archive) override;

    private:

        /**
         * @brief 직렬화된 파라미터 연결에 대응하는 Getter를 해석합니다.
         * @param Binding 변수 Getter를 참조하는 파라미터 연결 정보입니다.
         * @return 그래프 소유의 Getter이며 해석할 수 없으면 nullptr입니다.
         */
        const Asset::FAnimGraphVariableGetter* ResolveGetter(const Asset::FAnimGraphParameter& Binding) const;

        /**
         * @brief 변수 Getter의 ID·변수 이름·에디터 위치를 직렬화합니다.
         * @param Archive 저장·로드 방향과 오류 상태를 제공하는 아카이브입니다.
         */
        void SerializeVariableGetters(Core::FArchive& Archive);

        /**
         * @brief 상태 머신별 상태·전이와 중첩 포즈 그래프를 직렬화합니다.
         * @param Archive 저장·로드 방향과 오류 상태를 제공하는 아카이브입니다.
         */
        void SerializeStateMachines(Core::FArchive& Archive);

        /**
         * @brief 중첩 상태 그래프와 전이 조건의 변수 참조를 갱신합니다.
         * @param OldName 변경 전 변수 이름입니다.
         * @param NewName 변경 후 변수 이름입니다.
         */
        void UpdateChildVariables(const Core::FString& OldName, const Core::FString& NewName);

    private:

        /**
         * @brief 내용 변경을 감지하는 리비전입니다.
         */
        Core::FRevision mRevision{};

        /**
         * @brief 상태 머신 노드 ID별 중첩 상태 데이터입니다.
         */
        Core::TMap<Core::Int32, Asset::FAnimStateMachineData> mStateMachines{};

        /**
         * @brief 배치한 노드입니다.
         */
        Core::TArray<Asset::FAnimGraphNode> mNodes{};

        /**
         * @brief 포즈 연결입니다.
         */
        Core::TArray<Asset::FAnimGraphLink> mLinks{};

        /**
         * @brief 그래프 변수입니다.
         */
        Core::TArray<Asset::FAnimGraphVariable> mVariables{};

        /**
         * @brief 그래프 변수를 읽는 Getter 노드 목록입니다.
         */
        Core::TArray<Asset::FAnimGraphVariableGetter> mVariableGetters{};

        /**
         * @brief 다음 변수 Getter에 부여할 ID입니다.
         */
        Core::Int32 mNextGetterId{};

        /**
         * @brief 다음에 추가할 노드의 식별자입니다.
         */
        Core::Int32 mNextNodeId{};

        /**
         * @brief Output 노드의 캔버스 X 위치입니다.
         */
        float mOutputX{400.0f};

        /**
         * @brief Output 노드의 캔버스 Y 위치입니다.
         */
        float mOutputY{0.0f};
    };

}
