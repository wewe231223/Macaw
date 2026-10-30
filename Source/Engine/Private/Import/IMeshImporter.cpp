/**
 * @file IMeshImporter.cpp
 * @brief 원본 파일에서 메시 데이터와 재질 참조를 가져오는 임포터의 공통 인터페이스입니다.
 * @ingroup Engine
 */

#include "Engine/Import/IMeshImporter.h"
#include "Engine/Import/FAssimpMeshImporter.h"

namespace Engine {

    Engine::IMeshImporter::~IMeshImporter() = default;

    const Engine::IMeshImporter& Engine::IMeshImporter::GetDefault() {
        static const Engine::FAssimpMeshImporter Importer{};
        return Importer;
    }

}
