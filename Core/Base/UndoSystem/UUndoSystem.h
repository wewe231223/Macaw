#pragma once
#include "../../../Common.h" 
#include "../../Channel/FMessageChannel.h"

class UObject;
namespace UUndoSystem
{
    // =================================================================
    // Message Sender 관리 API
    // =================================================================
    void InitializeSenderToWorldChannel(FMessageChannel::FSender&& SenderToWorldChannel);

    // =================================================================
    // Undo/Redo API
    // =================================================================
    void BeginTransaction(const FString& TransactionName);
    void EndTransaction();

    void Modify(UObject* TargetObject);

    void Undo();
    void Redo();
}