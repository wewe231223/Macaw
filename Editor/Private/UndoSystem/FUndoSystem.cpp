#include "pch.h"
#include "Editor/UndoSystem/FUndoSystem.h"
#include "Editor/UndoSystem/IUndoContext.h"
#include "Editor/UndoSystem/FUndoTransaction.h"
#include "Editor/UndoSystem/FUndoRecords.h"
#include "Editor/UndoSystem/FUndoMessages.h"
#include "Serialization/FArchiveMemory.h"
#include "Core/Channel/FMessageChannel.h"
#include "Core/Channel/FMessage.h"

namespace {
    class FUndoHistoryStack {
    public:
        FUndoHistoryStack() = default;

    public:
        void Push(FUndoTransaction&& NewTransaction);
        FUndoTransaction* Undo();
        FUndoTransaction* Redo();
        void Clear();

    private:
        Uint32 mHead{};
        Uint32 mSize{};
        Uint32 mCurrent{};
        TFixedArray<std::optional<FUndoTransaction>, 64> mBuffer{};
    };

    void FUndoHistoryStack::Push(FUndoTransaction&& NewTransaction) {
        while (mSize > mCurrent) {
            --mSize;
            mBuffer[(mHead + mSize) % mBuffer.size()].reset();
        }

        if (mSize == mBuffer.size()) {
            mHead = (mHead + 1) % mBuffer.size();
            --mSize;
        }

        const Uint32 WriteIndex{(mHead + mSize) % static_cast<Uint32>(mBuffer.size())};

        mBuffer[WriteIndex].emplace(std::move(NewTransaction));
        ++mSize;
        mCurrent = mSize;
    }

    FUndoTransaction* FUndoHistoryStack::Undo() {
        if (mCurrent == 0) {
            return nullptr;
        }

        --mCurrent;

        const Uint32 TargetIndex{(mHead + mCurrent) % static_cast<Uint32>(mBuffer.size())};

        return &mBuffer[TargetIndex].value();
    }

    FUndoTransaction* FUndoHistoryStack::Redo() {
        if (mCurrent >= mSize) {
            return nullptr;
        }

        const Uint32 TargetIndex{(mHead + mCurrent) % static_cast<Uint32>(mBuffer.size())};

        ++mCurrent;

        return &mBuffer[TargetIndex].value();
    }

    void FUndoHistoryStack::Clear() {
        for (std::optional<FUndoTransaction>& Transaction : mBuffer) {
            Transaction.reset();
        }

        mHead = 0;
        mSize = 0;
        mCurrent = 0;
    }

    struct FUndoSystemState {
        FUndoHistoryStack mHistory{};
        std::unique_ptr<FUndoTransaction> mCurrentTransaction{nullptr};
        TArray<std::function<void(FUndoTransaction&)>> mPendingFinalizers{};
        TSet<UObject*> mModifiedObjectsThisTransaction{};
        std::optional<FMessageChannel::FSender> mMessageSender{};
    };

    static FUndoSystemState& GetState() {
        static FUndoSystemState State{};

        return State;
    }

    class FUndoContextImpl : public IUndoContext {
    public:
        void NotifyObjectChanged(const FGuid& Guid, const TArray<Uint8>& Data) override;
        void NotifyObjectSpawned(const FGuid& Guid, const TArray<Uint8>& Data, FName TypeName) override;
        void NotifyObjectDeleted(const FGuid& Guid) override;
    };

    void FUndoContextImpl::NotifyObjectChanged(const FGuid& Guid, const TArray<Uint8>& Data) {
        FUndoSystemState& State{GetState()};

        if (State.mMessageSender.has_value()) {
            State.mMessageSender->TryEmplace<FMessageUndoObjectStateChanged>(Guid, TArray<Uint8>{Data});
        }
    }

    void FUndoContextImpl::NotifyObjectSpawned(const FGuid& Guid, const TArray<Uint8>& Data, FName TypeName) {
        FUndoSystemState& State{GetState()};

        if (State.mMessageSender.has_value()) {
            State.mMessageSender->TryEmplace<FMessageUndoObjectSpawned>(Guid, TArray<Uint8>{Data}, std::move(TypeName));
        }
    }

    void FUndoContextImpl::NotifyObjectDeleted(const FGuid& Guid) {
        FUndoSystemState& State{GetState()};

        if (State.mMessageSender.has_value()) {
            State.mMessageSender->TryEmplace<FMessageUndoObjectDestroyed>(Guid);
        }
    }
}

namespace FUndoSystem {

    // =================================================================
    // Message Sender 관리 API
    // =================================================================
    void InitializeSenderToWorldChannel(FMessageChannel::FSender&& SenderToWorldChannel) {
        FUndoSystemState& State{GetState()};

        State.mMessageSender.emplace(std::move(SenderToWorldChannel));
    }

    // =================================================================
    // Undo/Redo API
    // =================================================================
    void Reset() {
        FUndoSystemState& State{GetState()};

        State.mCurrentTransaction.reset();
        State.mPendingFinalizers.clear();
        State.mModifiedObjectsThisTransaction.clear();
        State.mHistory.Clear();
    }

    void BeginTransaction(const FString& TransactionName) {
        FUndoSystemState& State{GetState()};

        if (State.mCurrentTransaction != nullptr)
            return;

        State.mCurrentTransaction = std::make_unique<FUndoTransaction>(TransactionName);
    }

    void RecordObject(UObject* TargetObject, EUndoType UndoType, const IAssetRegistry* AssetRegistry) {
        FUndoSystemState& State{GetState()};

        if (!State.mCurrentTransaction || !TargetObject)
            return;

        if (UndoType == EUndoType::StateChange) {
            if (State.mModifiedObjectsThisTransaction.contains(TargetObject))
                return;

            State.mModifiedObjectsThisTransaction.insert(TargetObject);
        }

        TArray<Uint8> CurrentData{};
        FArchiveMemory MemoryArchive{CurrentData};

        MemoryArchive.SetAssetResolver(AssetRegistry);
        TargetObject->Save(MemoryArchive);

        if (UndoType == EUndoType::StateChange) {
            State.mPendingFinalizers.push_back(
                [TargetObject, CurrentData = std::move(CurrentData)](FUndoTransaction& Transaction) {
                TArray<Uint8> AfterData{};
                FArchiveMemory MemoryArchiveAfter{AfterData};

                TargetObject->Save(MemoryArchiveAfter);

                auto Record{std::make_unique<FRecordObjectState>(TargetObject->GetGuid(), CurrentData, AfterData)};

                Transaction.AddRecord(std::move(Record));
            });
        } else {
            std::unique_ptr<IUndoRecord> Record{nullptr};

            switch (UndoType) {
                case EUndoType::Spawn:
                    Record = std::make_unique<FRecordObjectSpawned>(TargetObject->GetGuid(), std::move(CurrentData), TargetObject->GetTypeInfo()->mTypeName);
                    break;

                case EUndoType::Destroy:
                    Record = std::make_unique<FRecordObjectDestroyed>(TargetObject->GetGuid(), std::move(CurrentData), TargetObject->GetTypeInfo()->mTypeName);
                    break;
            }

            if (Record)
                State.mCurrentTransaction->AddRecord(std::move(Record));
        }
    }

    void EndTransaction() {
        FUndoSystemState& State{GetState()};

        if (!State.mCurrentTransaction)
            return;

        for (const auto& Finalizer : State.mPendingFinalizers) {
            Finalizer(*State.mCurrentTransaction);
        }

        State.mHistory.Push(std::move(*State.mCurrentTransaction));

        State.mCurrentTransaction.reset();
        State.mPendingFinalizers.clear();
        State.mModifiedObjectsThisTransaction.clear();
    }

    void Undo() {
        if (FUndoTransaction * Transactions{GetState().mHistory.Undo()}) {
            FUndoContextImpl UndoContext{};

            Transactions->Undo(&UndoContext);
        }
    }

    void Redo() {
        if (FUndoTransaction * Transactions{GetState().mHistory.Redo()}) {
            FUndoContextImpl RedoContext{};

            Transactions->Redo(&RedoContext);
        }
    }
}
