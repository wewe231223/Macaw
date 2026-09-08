#pragma once
#include "../../Channel/FMessageChannel.h"

enum class EUndoType
{
    StateChange,
    Spawn,
    Destroy,

    End
};

class UObject;
namespace FUndoSystem
{
    // =================================================================
    // Message Sender
    // =================================================================
    void InitializeSenderToWorldChannel(FMessageChannel::FSender&& SenderToWorldChannel);

    // =================================================================
    // Undo/Redo API
    // =================================================================
    void BeginTransaction(const FString& TransactionName);
    void RecordObject(UObject* TargetObject, EUndoType UndoType, FAssetRegistry* AssetRegistry);
    void EndTransaction();

    void Undo();
    void Redo();
}