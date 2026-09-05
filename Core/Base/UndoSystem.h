#pragma once
#include "../../Common.h" 

class UObject; 

namespace UndoSystem
{
    void BeginTransaction(const FString& TransactionName);
    void EndTransaction();

    void Modify(UObject* TargetObject);

    void Undo();
    void Redo();
}