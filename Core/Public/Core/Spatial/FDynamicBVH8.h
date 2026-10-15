/**
 * @file FDynamicBVH8.h
 * @brief 리프의 삽입, 삭제 및 경계 갱신을 지원하는 동적 8분기 BVH를 정의한다.
 */
/**
 * @var float FDynamicBVH8::FBounds::Max[3]
 * @brief X, Y, Z 순서의 경계 최댓값입니다.
 */
/**
 * @var Uint32 FDynamicBVH8::FParent::Lane
 * @brief 부모 노드 안에서 현재 참조가 저장된 자식 레인입니다.
 */
/**
 * @var double FDynamicBVH8::FCandidate::CombinedArea
 * @brief 후보 경계와 새 리프를 병합한 상자의 표면적입니다.
 */
/**
 * @var double FDynamicBVH8::FCandidate::Lower
 * @brief 후보 하위 트리의 삽입 위치 탐색에 사용하는 비용 하한입니다.
 */
/**
 * @var TArray<FDynamicBVH8::FParent> FDynamicBVH8::mLeafParents
 * @brief 리프 인덱스별 부모 노드와 자식 레인 연결을 저장한 배열입니다.
 */
#pragma once
#include "Core/Spatial/FBVH8.h"
#include <utility>

/**
 * @brief 부모 연결과 경계 비용을 관리하면서 정적 BVH에 동적 리프 변경을 지원한다.
 * @details 삽입 위치의 경계 확장 비용을 탐색하고 변경된 조상의 경계와 높이를 다시 맞춘다. 유리한 자식 교환으로 경계 비용을 줄인다.
 * @note 동적 삽입과 갱신은 개수 1인 리프 참조를 생성한다.
 */
class FDynamicBVH8 : public FBVH8 {
public:
    /**
     * @brief 기본 BVH와 부모, 리프 연결, 경계, 높이 및 노드 재사용 정보를 모두 비운다.
     */
    void Clear();

    /**
     * @brief 기본 BVH를 구축한 뒤 동적 변경에 필요한 부모와 경계 정보를 초기화한다.
     * @tparam TArgs FBVH8::Build()에 전달할 인수 타입들.
     * @param Args 기본 구축 함수에 완벽 전달할 인수들.
     * @details 기존 동적 정보를 비우고 기본 구축을 실행한 뒤 부모 연결, 자식 수, 경계 및 높이를 구성한다.
     */
    template <class... TArgs>
    void Build(TArgs&&... Args) {
        Clear();
        FBVH8::Build(std::forward<TArgs>(Args)...);
        InitializeParents();
    }

    /**
     * @brief 리프 하나를 비용에 따라 선택한 위치에 삽입한다.
     * @param Leaf 아직 등록되지 않은 IndexMask 이하의 리프 인덱스.
     * @param Box 리프의 축 정렬 경계. 각 축의 반크기는 비음수이고 변환한 최솟값과 최댓값이 유한해야 한다.
     * @details 빈 트리에는 루트를 생성한다. 그 외에는 기존 노드에 자식을 추가하거나 후보와 새 리프를 묶을 부모를 생성한 뒤 조상을 갱신한다.
     * @details 중복 리프 인덱스, 잘못된 경계 또는 참조 용량 초과는 치명적 오류로 보고한다.
     */
    void Insert(Uint32 Leaf, const DirectX::BoundingBox& Box);
    /**
     * @brief 리프를 제거하고 불필요한 내부 노드를 정리한다.
     * @param Leaf 제거할 리프 인덱스. 등록되지 않았으면 아무 작업도 하지 않는다.
     * @details 빈 슬롯은 마지막 자식으로 메우고 자식 하나만 남은 내부 노드는 가능한 경우 부모 연결에서 축약한다. 마지막 리프가 제거되면 트리를 비운다.
     */
    void Remove(Uint32 Leaf);
    /**
     * @brief 리프 경계가 바뀌면 제거 후 새 경계로 다시 삽입한다.
     * @param Leaf 갱신할 리프 인덱스. 등록되지 않았으면 새로 삽입한다.
     * @param Box 새 축 정렬 경계. Insert()와 같은 유한 값 및 비음수 반크기 조건을 사용한다.
     * @details 기존 최솟값과 최댓값이 모두 같으면 변경하지 않는다.
     */
    void Update(Uint32 Leaf, const DirectX::BoundingBox& Box);

private:
    /**
     * @brief 부모 연결이 없음을 나타내는 인덱스 값.
     */
    static constexpr Uint32 Invalid = UINT32_MAX;

    /**
     * @brief 동적 트리 비용 계산과 자식 갱신에 사용할 축별 최솟값 및 최댓값 경계.
     */
    struct FBounds {
        /**
         * @brief X, Y, Z 순서의 경계 최솟값과 최댓값.
         */
        float Min[3]{}, Max[3]{};

        /**
         * @brief 경계 상자의 실제 표면적을 계산한다.
         * @return 축별 길이를 X, Y, Z로 할 때 2 * (X * Y + Y * Z + Z * X).
         */
        double Area() const;
        /**
         * @brief 두 경계의 축별 최솟값과 최댓값을 결합한다.
         * @param A 첫 번째 경계.
         * @param B 두 번째 경계.
         * @return 두 경계를 포함하는 경계.
         */
        static FBounds Merge(const FBounds& A, const FBounds& B);
        /**
         * @brief 중심과 반크기로 표현된 상자를 최솟값 및 최댓값 경계로 변환한다.
         * @param Box 변환할 축 정렬 경계 상자.
         * @return 중심에서 반크기를 빼고 더한 경계.
         * @details 음수 반크기 또는 유한하지 않은 경계 성분은 치명적 오류로 보고한다.
         */
        static FBounds FromBox(const DirectX::BoundingBox& Box);
        /**
         * @brief 두 경계의 모든 최솟값과 최댓값을 정확히 비교한다.
         */
        bool operator==(const FBounds&) const = default;
    };

    /**
     * @brief 노드 또는 리프가 들어 있는 부모 노드와 자식 레인을 보관한다.
     */
    struct FParent {
        /**
         * @brief Node는 부모 인덱스이고 Lane은 부모의 자식 슬롯이다. Node가 Invalid이면 부모가 없다.
         */
        Uint32 Node = Invalid, Lane = 0;
    };

    /**
     * @brief 새 리프를 삽입할 위치의 참조, 경계, 비용 하한 및 깊이를 보관한다.
     */
    struct FCandidate {
        /**
         * @brief 삽입 후보인 내부 노드 또는 리프의 인코딩된 참조.
         */
        Uint32 Ref;
        /**
         * @brief 후보를 가리키는 부모 연결.
         */
        FParent Parent;
        /**
         * @brief 현재 후보 하위 트리의 경계.
         */
        FBounds Bounds;
        /**
         * @brief Inherited는 누적 경계 확장 비용, CombinedArea는 새 리프와 병합한 표면적, Lower는 탐색에 사용할 비용 하한이다.
         */
        double Inherited, CombinedArea, Lower;
        /**
         * @brief 루트에서 후보까지의 깊이.
         */
        Uint32 Depth;
    };

    /**
     * @brief 선택한 삽입 후보와 비용 및 자식 추가 방식을 보관한다.
     */
    struct FInsertion {
        /**
         * @brief 선택한 삽입 위치의 정보.
         */
        FCandidate Candidate;
        /**
         * @brief 선택한 구성의 삽입 비용.
         */
        double Cost;
        /**
         * @brief true이면 후보 내부 노드의 빈 레인에 추가하고 false이면 후보와 리프를 묶는 새 부모를 만든다.
         */
        bool Append;
    };

    /**
     * @brief 내부 노드별 부모 연결과 리프 인덱스별 부모 연결을 보관하는 배열.
     */
    TArray<FParent> mParents, mLeafParents;
    /**
     * @brief 내부 노드의 전체 자식 경계를 병합한 캐시.
     */
    TArray<FBounds> mBounds;
    /**
     * @brief 내부 노드별 유효한 자식 수.
     */
    TArray<Uint8> mCounts;
    /**
     * @brief 내부 노드별 하위 트리 높이.
     */
    TArray<Uint32> mHeights;
    /**
     * @brief 제거한 내부 노드 중 재사용할 수 있는 인덱스 목록.
     */
    TArray<Uint32> mFreeNodes;
    /**
     * @brief 비용 하한과 깊이로 우선순위를 정하는 삽입 탐색용 힙 저장소.
     */
    TArray<FCandidate> mCandidates;

    /**
     * @brief 경계 확장 비용과 높이를 비교하여 삽입 위치를 선택한다.
     * @param LeafBounds 삽입할 리프의 경계.
     * @return 기존 노드에 추가할지 또는 새 부모를 만들지와 그 비용을 포함한 후보.
     * @pre 트리가 비어 있지 않고 동적 메타데이터가 초기화되어 있어야 한다.
     */
    FInsertion FindInsertion(const FBounds& LeafBounds);
    /**
     * @brief 기본 BVH를 순회하여 부모 연결, 자식 수, 전체 경계와 높이를 초기화한다.
     */
    void InitializeParents();
    /**
     * @brief 비어 있는 내부 노드 저장 공간을 확보한다.
     * @return 재사용하거나 배열 끝에 새로 추가한 노드 인덱스.
     * @details 반환할 노드와 해당 메타데이터를 초기화한다. 참조 인덱스 용량을 넘으면 치명적 오류를 보고한다.
     */
    Uint32 AllocateNode();
    /**
     * @brief 내부 노드와 메타데이터를 초기화하고 재사용 목록에 넣는다.
     * @param Node 트리 연결에서 제거한 노드 인덱스.
     */
    void FreeNode(Uint32 Node);

    /**
     * @brief 노드 인덱스와 현재 자식 수로 내부 노드 참조를 구성한다.
     * @param Node 참조를 만들 내부 노드 인덱스.
     * @return 리프 비트가 없는 인코딩된 참조.
     */
    Uint32 Reference(Uint32 Node) const {
        return BVH8::MakeReference(Node, mCounts[Node]);
    }

    /**
     * @brief 내부 노드의 자식 레인 경계를 읽는다.
     * @param Node 내부 노드 인덱스.
     * @param Lane 경계를 읽을 자식 레인.
     * @return 노드의 축별 배열에서 읽은 경계.
     */
    FBounds ReadBounds(Uint32 Node, Uint32 Lane) const;
    /**
     * @brief 자식 참조와 경계를 쓰고 해당 자식의 부모 연결을 갱신한다.
     * @param Node 부모 노드 인덱스.
     * @param Lane 기록할 자식 레인.
     * @param Child 기록할 인코딩된 자식 참조.
     * @param Bounds 자식의 경계.
     */
    void WriteChild(Uint32 Node, Uint32 Lane, Uint32 Child, const FBounds& Bounds);
    /**
     * @brief 노드의 경계와 높이를 다시 계산하고 부모에 반영한다.
     * @param Node 유효한 자식이 있는 내부 노드 인덱스.
     * @details 부모가 없으면 현재 자식 수를 포함한 루트 참조를 갱신한다.
     */
    void Refresh(Uint32 Node);
    /**
     * @brief 현재 노드부터 루트까지 경계를 갱신하고 비용을 줄이는 교환을 적용한다.
     * @param Node 갱신을 시작할 내부 노드 인덱스.
     */
    void RefitAncestors(Uint32 Node);
    /**
     * @brief 자식 내부 노드의 손자와 그 형제를 교환하여 경계 비용을 줄인다.
     * @param Node 교환 후보를 검사할 내부 노드 인덱스.
     * @return 양의 표면적 감소를 주는 교환을 적용하면 true, 후보가 없으면 false.
     */
    bool Rotate(Uint32 Node);
};
