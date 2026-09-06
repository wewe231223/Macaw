#include "PCH.h"
#include "FUndoTransaction.h"

#include "IUndoRecord.h"

FUndoTransaction::~FUndoTransaction() = default;

void FUndoTransaction::Undo(IUndoContext& Context)
{
    for (auto CurrentRecord = Records.rbegin(), EndRecord = Records.rend(); CurrentRecord != EndRecord; ++CurrentRecord)
    {
        (*CurrentRecord)->ApplyUndo(Context);
    }
}

void FUndoTransaction::Redo(IUndoContext& Context)
{
    for (auto CurrentRecord = Records.begin(), EndRecord = Records.end(); CurrentRecord != EndRecord; ++CurrentRecord)
    {
        (*CurrentRecord)->ApplyRedo(Context);
    }
}