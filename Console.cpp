#include "pch.h"
#include "Console.h"

namespace
{
    class FConsoleMessageStorage
    {
    public:
        explicit FConsoleMessageStorage(size_t InCapacity)
            : Capacity(InCapacity)
        {
            Messages.resize(Capacity);
        }

        void Push(FString Message)
        {
            /*if (Messages.size() >= Capacity)
            {
                Messages.pop_front();
            }

            Messages.push_back(std::move(Message));*/

            if (Count < Capacity)
            {
                size_t Index = (Front + Count) % Capacity;
                Messages[Index] = std::move(Message);
                ++Count;
            }
            else
            {
                Messages[Front] = std::move(Message);
                Front = (Front + 1) % Capacity;
            }
        }

        void Clear()
        {
            // Messages.clear();
            Front = 0;
            Count = 0;
        }

        size_t GetMessageCount() const
        {
            // return Messages.size();
            return Count;
        }

        const FString& GetMessage(size_t Index) const
        {
            size_t CircleIndex = (Front + Index) % Capacity;
            return Messages[CircleIndex];
        }

    private:
        size_t Capacity;
        size_t Front = 0;
        size_t Count = 0;
        std::vector<FString> Messages;
    };

    struct FConsoleState
    {
        std::vector<FConsoleMessageStorage> OutputStorages;

        FConsoleState()
        {
            OutputStorages.emplace_back(1000); // STDOut
            OutputStorages.emplace_back(1000); // STDError
        }
    };

    FConsoleState& GetConsoleState()
    {
        static FConsoleState State;
        return State;
    }

    FConsoleMessageStorage* ResolveStorage(FConsoleOutputHandle Handle)
    {
        FConsoleState& State = GetConsoleState();

        if (!Handle.IsValid())
        {
            return nullptr;
        }

        if (Handle.Index >= State.OutputStorages.size())
        {
            return nullptr;
        }

        return &State.OutputStorages[Handle.Index];
    }
}

namespace Console
{
    void Print(FConsoleOutputHandle Handle, FString Message)
    {
        FConsoleMessageStorage* Storage = ResolveStorage(Handle);

        if (Storage == nullptr)
        {
            return;
        }

        Storage->Push(std::move(Message));
    }

    size_t GetMessageCount(FConsoleOutputHandle Handle)
    {
        const FConsoleMessageStorage* Storage = ResolveStorage(Handle);

        return Storage ? Storage->GetMessageCount() : 0;
    }

    const FString& GetMessageAt(
        FConsoleOutputHandle Handle,
        size_t Index)
    {
        return ResolveStorage(Handle)->GetMessage(Index);
    }
}
