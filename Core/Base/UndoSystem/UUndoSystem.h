#pragma once
#include "../../../Common.h" 

class UObject;
class FMessageChannel;
namespace UUndoSystem
{
    // =================================================================
    // Message Sender 관리 API
    // =================================================================
    void InitializeSenderToWorldChannel(FMessageChannel& WorldChannel);

    // =================================================================
    // Undo/Redo API
    // =================================================================
    void BeginTransaction(const FString& TransactionName);
    void EndTransaction();

    void Modify(UObject* TargetObject);

    void Undo();
    void Redo();
}