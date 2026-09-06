#pragma once
#include "../../../STL.h" 

class IUndoContext;
class IUndoRecord;
class FUndoTransaction
{
public:
    explicit FUndoTransaction(const FString& InName)
        : TransactionName(InName) {}

    ~FUndoTransaction();

    // move 만 사용
    FUndoTransaction(FUndoTransaction&&) noexcept = default;
    FUndoTransaction& operator=(FUndoTransaction&&) noexcept = default;

    FUndoTransaction(const FUndoTransaction&) = delete;
    FUndoTransaction& operator=(const FUndoTransaction&) = delete;

    void AddRecord(std::unique_ptr<IUndoRecord> Record)
    {
        Records.push_back(std::move(Record));
    }

    void Undo(IUndoContext& Context);
    void Redo(IUndoContext& Context);

private:
    FString TransactionName;
    std::vector<std::unique_ptr<IUndoRecord>> Records;
};