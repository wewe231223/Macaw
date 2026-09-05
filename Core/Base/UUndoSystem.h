#pragma once
#include "../../Common.h" 

class UObject; 

namespace UUndoSystem
{
    void BeginTransaction(const FString& TransactionName);
    void EndTransaction();

    void Modify(UObject* TargetObject);

    void Undo();
    void Redo();
}