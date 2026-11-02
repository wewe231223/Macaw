/**
 * @file FFBXImporterTemplates.h
 * @brief FFBXImporter의 템플릿 멤버 함수 구현입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Import/FFBXImporter.h"

namespace Asset {

    template <typename TValue>
    bool Asset::FFBXImporter::ReadElement(const fbxsdk::FbxLayerElementTemplate<TValue>* Element, int ControlPoint, int Polygon, int PolygonVertex, TValue& Value) const {
        if (Element == nullptr) {
            return true;
        }

        int Index{};

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
