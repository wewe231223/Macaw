/**
 * @file FSceneInterface.h
 * @brief 월드가 렌더 씬을 식별하고 변경 배치를 전달하는 인터페이스를 정의합니다.
 */
#pragma once

#include "RenderCore/FSceneHandle.h"

struct FSceneUpdateBatch;

/**
 * @brief 렌더 씬의 핸들 조회, 변경 적용과 해제 요청을 제공하는 인터페이스입니다.
 */
class FSceneInterface {
public:
    /**
     * @brief 파생 씬 인터페이스를 기반 포인터로 해제할 수 있게 합니다.
     */
    virtual ~FSceneInterface();

public:
    /**
     * @brief 현재 렌더 씬의 식별 핸들을 반환합니다.
     * @return 갱신 배치의 대상 확인에 사용할 씬 ID와 세대입니다.
     */
    virtual FSceneHandle GetHandle() const = 0;
    /**
     * @brief 대상 씬에 프리미티브·광원 변경 배치를 전달합니다.
     * @param[in,out] Updates 현재 핸들과 일치하는 변경 배치입니다. 적용 과정에서 생성 프록시의 소유권이 이동할 수 있습니다.
     * @return 구현이 배치 적용을 수락하면 true입니다. 씬 핸들이 맞지 않거나 적용할 수 없으면 false입니다.
     * @note 배치 적용 수락이 각 개별 변경의 성공을 보장하는지는 구현에 따릅니다.
     */
    virtual bool ApplyUpdates(FSceneUpdateBatch& Updates) = 0;
    /**
     * @brief 렌더러에 씬과 관련 리소스의 해제를 요청합니다.
     * @note 호출 후 인터페이스와 핸들의 사용 가능 여부는 소유 구현의 수명 정책을 따릅니다.
     */
    virtual void Release() = 0;
};
