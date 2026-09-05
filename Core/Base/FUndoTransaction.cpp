#include "PCH.h"
#include "FUndoTransaction.h"

#include "IUndoRecord.h"

FUndoTransaction::~FUndoTransaction() = default;

void FUndoTransaction::Undo()
{
    for (auto CurrentRecord = Records.rbegin(), EndRecord = Records.rend(); CurrentRecord != EndRecord; ++CurrentRecord)
    {
        (*CurrentRecord)->ApplyUndo();
    }
}

void FUndoTransaction::Redo()
{
    for (auto CurrentRecord = Records.begin(), EndRecord = Records.end(); CurrentRecord != EndRecord; ++CurrentRecord)
    {
        (*CurrentRecord)->ApplyRedo();
    }
}