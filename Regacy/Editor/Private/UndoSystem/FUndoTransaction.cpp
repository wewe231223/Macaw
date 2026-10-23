#include "pch.h"
#include "Editor/UndoSystem/FUndoTransaction.h"
#include "Editor/UndoSystem/IUndoRecord.h"
#include <ranges>

FUndoTransaction::~FUndoTransaction() = default;

void FUndoTransaction::Undo(IUndoContext* Context) {
    for (const auto& Record : std::views::reverse(mRecords)) {
        Record->ApplyUndo(Context);
    }
}

void FUndoTransaction::Redo(IUndoContext* Context) {
    for (const auto& Record : mRecords) {
        Record->ApplyRedo(Context);
    }
}

FUndoTransaction::FUndoTransaction(const FString& InName)
	: mTransactionName{InName} {
}

const FString& FUndoTransaction::GetTransactionName() const {
    return mTransactionName;
}

void FUndoTransaction::AddRecord(std::unique_ptr<IUndoRecord> Record) {
    mRecords.push_back(std::move(Record));
}
