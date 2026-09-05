#pragma once
#include "../../Common.h"
#include "IUndoRecord.h" 
#include "FGuid.h"    

class FRecordObjectState: public IUndoRecord
{
public:
    FRecordObjectState(const FGuid& InGuid,
        const std::vector<uint8_t>& InBefore,
        const std::vector<uint8_t>& InAfter)
        : TargetGuid(InGuid), BeforeData(InBefore), AfterData(InAfter) {
    }

    virtual void ApplyUndo() override;
    virtual void ApplyRedo() override;

private:
    FGuid TargetGuid;

    // TODO: SERIALIZE 방법에 따라 타입이 달라질수있음
    std::vector<uint8_t> BeforeData;
    std::vector<uint8_t> AfterData;
};