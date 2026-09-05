#pragma once

class IUndoRecord
{
public:
    virtual ~IUndoRecord() = default;

    virtual void ApplyUndo() = 0;

    virtual void ApplyRedo() = 0;
};