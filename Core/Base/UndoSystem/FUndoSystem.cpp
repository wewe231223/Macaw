#include "PCH.h"
#include "FUndoSystem.h"

#include <optional>

#include "IUndoContext.h"
#include "FUndoTransaction.h"
#include "FRecordObjectState.h" 
#include "../../../Serialize/FArchiveMemory.h"
#include "../../Channel/FMessageChannel.h"
#include "../../Channel/FMessage.h"
#include "FMessageUndo.h"

class UObject;

namespace
{
    template<uint32 Capacity = 64>
    class FUndoHistoryStack
    {
    public:
        FUndoHistoryStack() : Head(0), Size(0), Current(0) {}

        void Push(FUndoTransaction&& NewTransaction)
        {
            Size = Current; 

            if (Size == Capacity)
            {
                Head = (Head + 1) % Capacity;
                --Size;
            }

            uint32 WriteIndex = (Head + Size) % Capacity;
            Buffer[WriteIndex].emplace(std::move(NewTransaction));

            ++Size;
            Current = Size;
        }

        bool CanUndo() const { return Current > 0; }

        FUndoTransaction* Undo()
        {
            if (!CanUndo()) 
                return nullptr;
            --Current;
            uint32 TargetIndex = (Head + Current) % Capacity;
            return &Buffer[TargetIndex].value();
        }

        bool CanRedo() const { return Current < Size; }

        FUndoTransaction* Redo()
        {
            if (!CanRedo()) 
                return nullptr;
            uint32 TargetIndex = (Head + Current) % Capacity;
            ++Current;
            return &Buffer[TargetIndex].value();
        }

        void Clear()
        {
            Head = 0;
            Size = 0;
            Current = 0;
        }

    private:
        uint32 Head;
        uint32 Size;
        uint32 Current;
        TFixedArray<std::optional<FUndoTransaction>, Capacity> Buffer;
    };

    struct FUndoSystemState 
    {
        FUndoHistoryStack<64> History;

        std::unique_ptr<FUndoTransaction> CurrentTransaction = nullptr;
        TArray<std::function<void(FUndoTransaction&)>> PendingFinalizers;
        TSet<UObject*> ModifiedObjectsThisTransaction;

        std::optional<FMessageChannel::FSender> MessageSender;
    };

    static FUndoSystemState& GetState()
    {
        static FUndoSystemState State;
        return State;
    }


    template<typename TMessage> requires CMessageType<std::remove_cvref_t<TMessage>>
    bool SendMessageToWorldChannel(TMessage&& Message)
    {
        FUndoSystemState& State = GetState();
        if (State.MessageSender.has_value())
        {
            return State.MessageSender->TryPush(std::forward<TMessage>(Message));
        }
        return false;
    }
    template<CMessageType TMessage, typename... Args> requires CMessageConstructible<TMessage, Args...>
    bool EmplaceMessageToWorldChannel(Args&&... Arguments)
    {
        FUndoSystemState& State = GetState();
        if (State.MessageSender.has_value())
        {
            return State.MessageSender->TryEmplace<TMessage>(std::forward<Args>(Arguments)...);
        }
        return false;
    }


    class FUndoContextImpl : public IUndoContext
    {
    public:
        virtual void NotifyObjectChanged(const FGuid& Guid, const TArray<uint8>& Data) override
        {
            EmplaceMessageToWorldChannel<FMessageObjectStateChanged>(Guid, TArray<uint8>(Data));
        }

        virtual void NotifyObjectDeleted(const FGuid& Guid) override
        {
            // 나중에 FObjectDeletedMessage 메시지가 생기면 여기서 쏴주면 됨
            // EmplaceMessageToWorldChannel<FObjectDeletedMessage>(Guid, ...);
        }

        virtual void NotifyObjectSpawned(const FGuid& Guid) override
        {
            // 나중에 FObjectSpawnedMessage 메시지가 생기면 여기서 쏴주면 됨
            // EmplaceMessageToWorldChannel<FObjectSpawnedMessage>(Guid, ...);
        }
    };


}

namespace FUndoSystem
{
    // =================================================================
    // Message Sender 관리 API
    // =================================================================
    void InitializeSenderToWorldChannel(FMessageChannel::FSender&& SenderToWorldChannel)
    {
        FUndoSystemState& State = GetState();
        State.MessageSender.emplace(std::move(SenderToWorldChannel));
    }

    // =================================================================
    // Undo/Redo API
    // =================================================================
    void BeginTransaction(const FString& TransactionName)
    {
        FUndoSystemState& State = GetState();
        if (State.CurrentTransaction != nullptr) 
            return;

        State.CurrentTransaction = std::make_unique<FUndoTransaction>(TransactionName);
    }

    void Modify(UObject* TargetObject)
    {
        FUndoSystemState& State = GetState();
        if (!State.CurrentTransaction || !TargetObject)
            return;

        if (State.ModifiedObjectsThisTransaction.contains(TargetObject))
            return;

        State.ModifiedObjectsThisTransaction.insert(TargetObject);

        TArray<uint8> BeforeData;
        FArchiveMemory MemoryArchiveBefore(BeforeData);
        TargetObject->Save(MemoryArchiveBefore); 

        State.PendingFinalizers.push_back(
            [TargetObject, BeforeData = std::move(BeforeData)](FUndoTransaction& Transaction)
            {
                TArray<uint8> AfterData;
                FArchiveMemory MemoryArchiveAfter(AfterData);
                TargetObject->Save(MemoryArchiveAfter);

                auto Record = std::make_unique<FRecordObjectState>(
                    TargetObject->GetGuid(), BeforeData, AfterData);

                Transaction.AddRecord(std::move(Record));
            }
        );
    }

    void EndTransaction()
    {
        FUndoSystemState& State = GetState();
        if (!State.CurrentTransaction)
            return;
        if (State.PendingFinalizers.size() == 0)
        {
            State.CurrentTransaction.reset();
            return;
        }

        for (const auto& Finalizer : State.PendingFinalizers)
        {
            Finalizer(*State.CurrentTransaction);
        }

        State.History.Push(std::move(*State.CurrentTransaction));

        State.CurrentTransaction.reset();
        State.PendingFinalizers.clear();
        State.ModifiedObjectsThisTransaction.clear();
    }

    void Undo()
    {
        if (FUndoTransaction* Transactions = GetState().History.Undo())
        {
            FUndoContextImpl UndoContext;
            Transactions->Undo(UndoContext);
        }
    }

    void Redo()
    {
        if (FUndoTransaction* Transactions = GetState().History.Redo())
        {
            FUndoContextImpl RedoContext;
            Transactions->Redo(RedoContext);
        }
    }
}