/**
 * @file FWorldCloner.h
 * @brief 메모리 직렬화와 객체 GUID 재매핑으로 독립적인 월드를 복제합니다.
 */
#pragma once

#include "World/EWorldType.h"
#include <memory>

class UWorld;

/**
 * @brief 원본의 에셋 레지스트리를 공유하면서 액터와 컴포넌트 객체를 새 월드에 복제합니다.
 */
class FWorldCloner final {
public:
    /**
     * @brief 원본 월드를 메모리에 저장하고 새 GUID를 적용해 새 월드에 로드합니다.
     * @param SourceWorld 초기화되어 있고 바쁘지 않으며 에셋 레지스트리가 있는 원본입니다.
     * @param WorldType 새 월드의 타입입니다.
     * @return 복제 월드의 단독 소유권입니다. 저장, GUID 재매핑, 로드 또는 남은 데이터 검사 실패 시 nullptr입니다.
     * @note 월드·레벨·액터·컴포넌트 참조를 새 객체로 재매핑합니다. 원본 저장 과정에서는 지연 액터 삭제를 처리할 수 있습니다.
     */
    static std::unique_ptr<UWorld> Clone(UWorld& SourceWorld, EWorldType WorldType = EWorldType::Game);
};
