#include "Core/Channel/FMessageChannel.h"
#include "Core/Asset/IAssetResolver.h"

#include <iostream>
#include <stdexcept>

struct FFirstMessage {
    JG_DECLARE_CHANNEL_MESSAGE(FFirstMessage);

    explicit FFirstMessage(int Value) noexcept;

    int mValue{};
};

struct FSecondMessage {
    JG_DECLARE_CHANNEL_MESSAGE(FSecondMessage);

    explicit FSecondMessage(int Value) noexcept;

    int mValue{};
};

FFirstMessage::FFirstMessage(int Value) noexcept
	: mValue{Value} {
}

FSecondMessage::FSecondMessage(int Value) noexcept
	: mValue{Value} {
}

const FMessageTypeInfo& FFirstMessage::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FFirstMessage"};
    return Information;
}
const FMessageTypeInfo& FSecondMessage::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FSecondMessage"};
    return Information;
}

static void Require(bool Condition, const char* Message) {
    if (!Condition) {
        throw std::runtime_error{Message};
    }
}

int main() {
    try {
        FMessageChannel Channel{8};
        int Total{};
        Require(Channel.TryBind<FFirstMessage>([&Channel, &Total](const FFirstMessage& Message) {
            Total += Message.mValue;
            Channel.GetSender().TryEmplace<FSecondMessage>(Message.mValue * 2);
        }), "first handler registration failed");
        Require(Channel.TryBind<FSecondMessage>([&Total](const FSecondMessage& Message) {
            Total += Message.mValue;
        }), "second handler registration failed");
        Require(Channel.GetSender().TryEmplace<FFirstMessage>(7), "message enqueue failed");
        Channel.Dispatch();
        Require(Total == 7 && !Channel.IsEmpty(), "messages enqueued during dispatch did not remain deferred");
        Channel.Dispatch();
        Require(Total == 21 && Channel.IsEmpty(), "typed message dispatch failed");
        Require(!FFirstMessage::StaticTypeInfo().IsExactlyA(&FSecondMessage::StaticTypeInfo()), "message types were conflated");
        const FGuid Guid{FGuid::NewGuid()};
        FGuid Parsed{};
        Require(Parsed.Parse(Guid.ToString()) && Guid == Parsed, "core GUID round trip failed");
        std::cout << "Core boundary tests passed without CoreUObject or Editor.\n";
        return 0;
    } catch (const std::exception& Error) {
        std::cerr << Error.what() << '\n';
        return 1;
    }
}
