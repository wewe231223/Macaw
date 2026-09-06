#include "PCH.h"
#include "FRecordObjectState.h"
#include "IUndoContext.h"

void FRecordObjectState::ApplyUndo(IUndoContext& Context)
{
    Context.NotifyObjectChanged(TargetGuid, BeforeData);
}

void FRecordObjectState::ApplyRedo(IUndoContext& Context)
{
    Context.NotifyObjectChanged(TargetGuid, AfterData);
}