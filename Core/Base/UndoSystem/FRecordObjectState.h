#pragma once
#include "IUndoRecord.h" 
#include "../FGuid.h"    

class FRecordObjectState: public IUndoRecord
{
public:
    FRecordObjectState(const FGuid& InGuid,
        const std::vector<uint8>& InBefore,
        const std::vector<uint8>& InAfter)
        : TargetGuid(InGuid), BeforeData(InBefore), AfterData(InAfter) {
    }

    virtual void ApplyUndo(IUndoContext& Context) override;
    virtual void ApplyRedo(IUndoContext& Context) override;

private:
    FGuid TargetGuid;

    std::vector<uint8> BeforeData;
    std::vector<uint8> AfterData;
};