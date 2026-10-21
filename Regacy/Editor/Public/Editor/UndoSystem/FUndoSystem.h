/**
 * @file FUndoSystem.h
 * @brief 전역 에셋 해석기를 사용하는 편집 작업의 실행 취소 기록을 정의합니다.
 */
#pragma once
#include "Core/Channel/FMessageChannel.h"
#include "CoreUObject/UObject.h"

enum class EUndoType {
    StateChange,
    Spawn,
    Destroy,

    End
};

namespace FUndoSystem {
    // =================================================================
    // Message Sender
    // =================================================================
    void InitializeSenderToWorldChannel(FMessageChannel::FSender&& SenderToWorldChannel);

    // =================================================================
    // Undo/Redo API
    // =================================================================
    void Reset();
    void BeginTransaction(const FString& TransactionName);
    /**
     * @brief 전역 에셋 서비스를 사용해 대상 객체의 상태를 현재 트랜잭션에 기록합니다.
     * @param TargetObject 기록할 객체입니다.
     * @param UndoType 상태 변경, 생성 또는 삭제 기록 종류입니다.
     * @details 직렬화 아카이브는 각 스냅샷 생성 시 현재 에셋 해석기를 조회합니다.
     */
    void RecordObject(UObject* TargetObject, EUndoType UndoType);
    void EndTransaction();

    void Undo();
    void Redo();
}
