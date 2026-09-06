#pragma once

class IUndoContext
{
public:
    virtual ~IUndoContext() = default;

    virtual void NotifyObjectChanged(const FGuid& Guid, const std::vector<uint8>& Data) = 0;
    virtual void NotifyObjectDeleted(const FGuid& Guid) = 0;
    virtual void NotifyObjectSpawned(const FGuid& Guid) = 0;
};