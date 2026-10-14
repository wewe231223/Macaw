/**
 * @file FRevisionCursor.h
 * @brief 외부 리비전의 기록, 비교 및 무효화를 위한 커서를 정의한다.
 */
#pragma once

#include "Core/CoreMinimal.h"
#include "Core/Common.h"

/**
 * @brief 마지막으로 기록한 리비전과 그 유효 상태를 보관한다.
 * @details 외부에서 제공한 리비전을 Commit()으로 기록하고 IsCurrent()로 일치 여부를 확인한다. 리비전을 자체적으로 증가시키지는 않는다.
 */
class FRevisionCursor {
public:
    /** @brief 리비전이 0이고 아직 유효한 기록이 없는 커서를 생성한다. */
    FRevisionCursor() = default;

public:
    /**
     * @brief 저장된 리비전 값을 조회한다.
     * @return 저장된 리비전. 기본 생성 또는 Invalidate() 직후에는 0.
     * @note 0도 Commit()으로 기록할 수 있는 값이므로 반환값만으로 유효 상태를 판별할 수 없다.
     */
    Uint64 GetRevision() const;
    /**
     * @brief 유효한 기록이 지정한 리비전과 일치하는지 확인한다.
     * @param Revision 비교할 외부 리비전.
     * @return 커서가 유효하고 저장된 값이 Revision과 같으면 true, 그 외에는 false.
     */
    bool IsCurrent(Uint64 Revision) const;

    /**
     * @brief 리비전을 기록하고 커서를 유효 상태로 전환한다.
     * @param Revision 기록할 외부 리비전. 0을 포함한 모든 Uint64 값을 사용할 수 있다.
     * @details 기존 리비전을 전달한 값으로 덮어쓴다.
     */
    void Commit(Uint64 Revision);
    /**
     * @brief 커서를 무효화하고 저장된 리비전을 0으로 초기화한다.
     * @details 다시 Commit()하기 전까지 IsCurrent()는 모든 입력에 false를 반환한다.
     */
    void Invalidate();

private:
    /** @brief 마지막으로 기록한 리비전 값. 기본 생성 및 무효화 시에는 0이다. */
    Uint64 mRevision{};
    /** @brief 유효한 리비전 기록이 있는지 나타내는 상태. Commit() 시 true, Invalidate() 시 false이다. */
    bool mValid{};
};
