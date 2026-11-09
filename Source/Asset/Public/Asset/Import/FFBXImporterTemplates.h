/**
 * @file FFBXImporterTemplates.h
 * @brief FFBXImporter의 템플릿 멤버 함수 구현입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Import/FFBXImporter.h"

namespace Asset {

    /**
     * @brief 레이어 요소의 매핑·참조 모드에 따라 다각형 정점의 값을 읽습니다.
     */
    template <typename TValue>
    bool Asset::FFBXImporter::ReadElement(const fbxsdk::FbxLayerElementTemplate<TValue>* Element, int ControlPoint, int Polygon, int PolygonVertex, TValue& Value) const {
        if (Element == nullptr) {
            return true;
        }

        int Index{};

        // 매핑 모드는 값이 무엇을 단위로 저장되었는지를 나타내므로, 현재 꼭짓점에 맞는 인덱스를 고릅니다.
        // eAllSame은 모든 꼭짓점이 같은 값을 쓰므로 인덱스 0을 그대로 둡니다.
        switch (Element->GetMappingMode()) {
        case fbxsdk::FbxLayerElement::eByControlPoint:
            Index = ControlPoint;

            break;
        case fbxsdk::FbxLayerElement::eByPolygonVertex:
            Index = PolygonVertex;

            break;
        case fbxsdk::FbxLayerElement::eByPolygon:
            Index = Polygon;

            break;
        case fbxsdk::FbxLayerElement::eAllSame:
            break;
        default:
            return false;
        }

        // 참조 모드는 위 인덱스의 해석 방법입니다. eDirect는 값 배열의 위치이고, 인덱스 모드는 인덱스 배열을 한 번 더 거쳐 값 배열의 위치를 얻습니다.
        if (Element->GetReferenceMode() == fbxsdk::FbxLayerElement::eIndexToDirect || Element->GetReferenceMode() == fbxsdk::FbxLayerElement::eIndex) {
            if (Index < 0 || Index >= Element->GetIndexArray().GetCount()) {
                return false;
            }

            Index = Element->GetIndexArray().GetAt(Index);
        } else if (Element->GetReferenceMode() != fbxsdk::FbxLayerElement::eDirect) {
            return false;
        }

        if (Index < 0 || Index >= Element->GetDirectArray().GetCount()) {
            return false;
        }

        Value = Element->GetDirectArray().GetAt(Index);

        return true;
    }

}
