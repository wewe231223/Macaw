#include "PCH.h"
#include "UUndoSystem.h"

#include <optional>

#include "FUndoTransaction.h"
#include "FRecordObjectState.h" 
#include "UObject.h"

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
        std::array<std::optional<FUndoTransaction>, Capacity> Buffer;
    };

    struct FUndoSystemState 
    {
        FUndoHistoryStack<64> History;

        std::unique_ptr<FUndoTransaction> CurrentTransaction = nullptr;
        std::vector<std::function<void(FUndoTransaction&)>> PendingFinalizers;
        std::unordered_set<UObject*> ModifiedObjectsThisTransaction;
    };

    static FUndoSystemState& GetState()
    {
        static FUndoSystemState State;
        return State;
    }
}

namespace UUndoSystem
{
    void BeginTransaction(const FString& TransactionName)
    {
        FUndoSystemState& State = GetState();
        if (State.CurrentTransaction != nullptr) 
            return;

        State.CurrentTransaction = std::make_unique<FUndoTransaction>(TransactionName);
        State.ModifiedObjectsThisTransaction.clear();
    }

    void Modify(UObject* TargetObject)
    {
        FUndoSystemState& State = GetState();
        if (!State.CurrentTransaction || !TargetObject) return;

        if (State.ModifiedObjectsThisTransaction.contains(TargetObject))
            return;
        State.ModifiedObjectsThisTransaction.insert(TargetObject);

        std::vector<uint8_t> BeforeData;
        // TODO
        // Serialize가 구현되면, 현재 target object의 값을 serialize해서 이를 before data 에 저장해야함
        // 예시: TargetObject->Serialize(BeforeData);

        // 트랜잭션이 끝난 뒤의 상태를 저장하는 람다
        State.PendingFinalizers.push_back(
            [TargetObject, BeforeData](FUndoTransaction& Transaction)
            {
                std::vector<uint8_t> AfterData;
                // TODO
                // Serialize가 구현되면, 현재 target object의 값을 serialize해서 이를 이번에는 after data 에 저장해야함
                // TargetObject->Serialize(AfterData);

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

        for (const auto& Finalizer : State.PendingFinalizers)
        {
            Finalizer(*State.CurrentTransaction);
        }

        // 트랜잭션이 끝날 때, 그동안 큐에 쌓여있던 Record들을 모두 저장함
        State.History.Push(std::move(*State.CurrentTransaction));

        State.CurrentTransaction.reset();
        State.PendingFinalizers.clear();
    }

    void Undo()
    {
        if (FUndoTransaction* Transactions = GetState().History.Undo())
        {
            Transactions->Undo();
        }
    }

    void Redo()
    {
        if (FUndoTransaction* Transactions = GetState().History.Redo())
        {
            Transactions->Redo();
        }
    }
}