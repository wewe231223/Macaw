/**
 * @file FBVH8Traversal.h
 * @brief 준비된 광선의 경계 검사와 리프 방문자를 사용하는 공용 8분기 BVH 순회를 정의한다.
 */
#pragma once
#include "Core/Spatial/FBVH8.h"

namespace BVH8 {
    /**
     * @brief 교차하는 경계를 가까운 순서로 방문하며 최근접 거리 상한으로 가지를 제거한다.
     * @tparam TPreparedRay FRayData로 생성되고 Intersect()로 자식 교차 마스크와 진입 거리를 계산하는 타입.
     * @tparam HasParallel 평행 축 처리를 사용하는지 지정하는 컴파일 시점 값.
     * @tparam TVisitLeaf 리프 인덱스, 묶음 크기 및 float 거리 참조를 받아 충돌 여부를 반환할 호출 대상.
     * @param Nodes 참조 인덱스로 접근하는 내부 노드 배열.
     * @param RootReference 시작할 인코딩된 노드 또는 리프 참조.
     * @param Ray 준비할 광선의 축별 데이터.
     * @param[in,out] ClosestDistance 광선 매개변수 상한. 리프 방문자가 더 가까운 충돌 값으로 갱신한다.
     * @param VisitLeaf 실제 리프 교차를 검사하는 방문자.
     * @return 방문자가 하나라도 충돌을 보고하면 true.
     * @details 64개 항목의 스택을 먼저 사용하고 넘치는 항목은 동적 배열에 보관한다. 방문자의 false 반환은 순회를 중단시키지 않는다.
     * @pre HasParallel이 false이면 Ray의 모든 축 평행 상태가 false여야 한다.
     */
    template <class TPreparedRay, bool HasParallel, class TVisitLeaf>
    bool Traverse(const FNode* Nodes, Uint32 RootReference, const FRayData& Ray, float& ClosestDistance, TVisitLeaf VisitLeaf) {
        const TPreparedRay Prepared{Ray};

        /**
         * @brief 순회 스택에 저장할 참조와 경계 진입 거리.
         */
        struct FEntry {
            /**
             * @brief 방문할 내부 노드 또는 리프의 인코딩된 참조.
             */
            Uint32 Reference;
            /**
             * @brief 해당 경계에 진입하는 광선 매개변수. 현재 상한을 넘으면 방문을 생략한다.
             */
            float Distance;
        };

        FEntry Stack[64];
        Uint32 StackSize = 0;
        TArray<FEntry> Overflow;

        const auto Push = [&](FEntry Entry) {
            if (StackSize < std::size(Stack))
                Stack[StackSize++] = Entry;
            else
                Overflow.push_back(Entry);
        };

        FEntry Entry{RootReference, 0.0f};
        bool Hit = false;

        for (;;) {
            if (Entry.Distance <= ClosestDistance) {
                if (Entry.Reference & BVH8::LeafBit) {
                    Hit |= VisitLeaf(Entry.Reference & BVH8::IndexMask, BVH8::GetCount(Entry.Reference), ClosestDistance);
                } else {
                    const auto& Node = Nodes[Entry.Reference & BVH8::IndexMask];

                    _mm_prefetch(reinterpret_cast<const char*>(Node.Children), _MM_HINT_T0);

                    float Distances[8];
                    Uint32 Mask = Prepared.template Intersect<HasParallel>(Node, ClosestDistance, Distances, BVH8::GetCount(Entry.Reference));

                    if (Mask != 0 && (Mask & (Mask - 1)) == 0) {
                        const Uint32 Lane = std::countr_zero(Mask);

                        Entry = {Node.Children[Lane], Distances[Lane]};
                        continue;
                    }

                    const Uint32 Remaining = Mask & (Mask - 1);

                    if (Remaining != 0 && (Remaining & (Remaining - 1)) == 0) {
                        const Uint32 A = std::countr_zero(Mask), B = std::countr_zero(Remaining);
                        const FEntry First{Node.Children[A], Distances[A]}, Second{Node.Children[B], Distances[B]};

                        if (First.Distance <= Second.Distance) {
                            Push(Second);
                            Entry = First;
                        } else {
                            Push(First);
                            Entry = Second;
                        }

                        continue;
                    }

                    FEntry Children[8];
                    Uint32 Count = 0;

                    while (Mask != 0) {
                        const Uint32 Lane = std::countr_zero(Mask);

                        Mask &= Mask - 1;

                        const FEntry Child{Node.Children[Lane], Distances[Lane]};
                        Uint32 Position = Count++;

                        while (Position > 0 && Child.Distance < Children[Position - 1].Distance) {
                            Children[Position] = Children[Position - 1];
                            --Position;
                        }

                        Children[Position] = Child;
                    }

                    if (Count != 0) {
                        for (Uint32 i = Count; i > 1; --i)
                            Push(Children[i - 1]);

                        Entry = Children[0];
                        continue;
                    }
                }
            }

            if (!Overflow.empty()) {
                Entry = Overflow.back();
                Overflow.pop_back();
            } else if (StackSize != 0)
                Entry = Stack[--StackSize];
            else
                break;
        }

        return Hit;
    }
}
