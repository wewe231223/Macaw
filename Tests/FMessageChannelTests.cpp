#include "../doctest/doctest.h"

#include <array>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../Channel/FMessageChannel.h"

namespace {
    struct FNumberMessage {
        explicit FNumberMessage(int InValue) noexcept : Value(InValue) {}

        int Value{};

        JG_DECLARE_ROOT_TYPEINFO(FNumberMessage)
    };

    struct FUnhandledMessage {
        JG_DECLARE_ROOT_TYPEINFO(FUnhandledMessage)
    };

    struct FTriggerMessage {
        JG_DECLARE_ROOT_TYPEINFO(FTriggerMessage)
    };

    struct FSecondaryMessage {
        explicit FSecondaryMessage(int InValue) noexcept : Value(InValue) {}

        int Value{};

        JG_DECLARE_ROOT_TYPEINFO(FSecondaryMessage)
    };

    struct FLargeTrackedMessage {
        FLargeTrackedMessage(int InValue, int& InDestructionCount) noexcept
            : Value(InValue), DestructionCount(&InDestructionCount) {
        }

        FLargeTrackedMessage(const FLargeTrackedMessage&) = delete;
        FLargeTrackedMessage& operator=(const FLargeTrackedMessage&) = delete;

        FLargeTrackedMessage(FLargeTrackedMessage&& Other) noexcept
            : Value(Other.Value), DestructionCount(std::exchange(Other.DestructionCount, nullptr)) {
        }

        FLargeTrackedMessage& operator=(FLargeTrackedMessage&&) = delete;

        ~FLargeTrackedMessage() noexcept {
            if (DestructionCount != nullptr) {
                ++(*DestructionCount);
            }
        }

        int Value{};
        int* DestructionCount{};
        std::array<std::byte, FMessage::InlineSize + 1> Padding{};

        JG_DECLARE_ROOT_TYPEINFO(FLargeTrackedMessage)
    };
}

TEST_SUITE("FMessageChannel") {
    TEST_CASE("bound messages are dispatched in FIFO order") {
        FMessageChannel Channel;
        std::vector<int> ReceivedValues;

        CHECK(Channel.TryBind<FNumberMessage>([&ReceivedValues](const FNumberMessage& Message) {
            ReceivedValues.push_back(Message.Value);
        }));

        CHECK(Channel.TryEmplace<FNumberMessage>(10));
        CHECK(Channel.TryPush(FNumberMessage{ 20 }));
        CHECK_EQ(Channel.Size(), 2);

        const FMessageDispatchResult Result = Channel.Dispatch();

        CHECK_EQ(Result.DispatchedCount, 2);
        CHECK_EQ(Result.UnhandledCount, 0);
        CHECK_FALSE(Result.Deferred);
        CHECK(Channel.IsEmpty());
        CHECK_EQ(ReceivedValues, std::vector<int>{ 10, 20 });
    }

    TEST_CASE("capacity is enforced for the channel and its sender") {
        FMessageChannel Channel{ 2 };
        FMessageChannel::FSender Sender = Channel.GetSender();

        CHECK_EQ(Channel.GetCapacity(), 2);
        CHECK(Sender.TryEmplace<FNumberMessage>(1));
        CHECK(Sender.TryPush(FNumberMessage{ 2 }));
        CHECK(Channel.IsFull());
        CHECK_FALSE(Sender.TryEmplace<FNumberMessage>(3));
        CHECK_EQ(Channel.Size(), 2);

        Channel.Clear();

        CHECK(Channel.IsEmpty());
        CHECK_FALSE(Channel.IsFull());
    }

    TEST_CASE("unbound messages are reported as unhandled") {
        FMessageChannel Channel;

        CHECK(Channel.TryEmplace<FUnhandledMessage>());

        const FMessageDispatchResult Result = Channel.Dispatch();

        CHECK_EQ(Result.DispatchedCount, 0);
        CHECK_EQ(Result.UnhandledCount, 1);
        CHECK(Channel.IsEmpty());
    }

    TEST_CASE("only one handler can be bound for each message type") {
        FMessageChannel Channel;

        CHECK(Channel.TryBind<FNumberMessage>([](const FNumberMessage&) {}));
        CHECK_FALSE(Channel.TryBind<FNumberMessage>([](const FNumberMessage&) {}));
    }

    TEST_CASE("messages produced by a handler wait for the next dispatch") {
        FMessageChannel Channel;
        std::vector<int> ReceivedValues;

        REQUIRE(Channel.TryBind<FNumberMessage>([&Channel, &ReceivedValues](const FNumberMessage& Message) {
            ReceivedValues.push_back(Message.Value);
            if (Message.Value == 1) {
                CHECK(Channel.TryEmplace<FNumberMessage>(2));
            }
        }));
        REQUIRE(Channel.TryEmplace<FNumberMessage>(1));

        const FMessageDispatchResult FirstResult = Channel.Dispatch();

        CHECK_EQ(FirstResult.DispatchedCount, 1);
        CHECK_EQ(Channel.Size(), 1);
        CHECK_EQ(ReceivedValues, std::vector<int>{ 1 });

        const FMessageDispatchResult SecondResult = Channel.Dispatch();

        CHECK_EQ(SecondResult.DispatchedCount, 1);
        CHECK(Channel.IsEmpty());
        CHECK_EQ(ReceivedValues, std::vector<int>{ 1, 2 });
    }

    TEST_CASE("a nested dispatch is deferred and completed by the outer dispatch") {
        FMessageChannel Channel;
        FMessageDispatchResult NestedResult;
        int ReceivedValue = 0;

        REQUIRE(Channel.TryBind<FTriggerMessage>([&Channel, &NestedResult](const FTriggerMessage&) {
            CHECK(Channel.TryEmplace<FNumberMessage>(77));
            NestedResult = Channel.Dispatch();
        }));
        REQUIRE(Channel.TryBind<FNumberMessage>([&ReceivedValue](const FNumberMessage& Message) {
            ReceivedValue = Message.Value;
        }));
        REQUIRE(Channel.TryEmplace<FTriggerMessage>());

        const FMessageDispatchResult OuterResult = Channel.Dispatch();

        CHECK(NestedResult.Deferred);
        CHECK_EQ(NestedResult.DispatchedCount, 0);
        CHECK_EQ(OuterResult.DispatchedCount, 2);
        CHECK_FALSE(OuterResult.Deferred);
        CHECK_EQ(ReceivedValue, 77);
        CHECK(Channel.IsEmpty());
    }

    TEST_CASE("a handler bound during dispatch becomes active afterwards") {
        FMessageChannel Channel;
        int ReceivedValue = 0;
        bool BindSucceeded = false;

        REQUIRE(Channel.TryBind<FTriggerMessage>([&Channel, &ReceivedValue, &BindSucceeded](const FTriggerMessage&) {
            BindSucceeded = Channel.TryBind<FSecondaryMessage>([&ReceivedValue](const FSecondaryMessage& Message) {
                ReceivedValue = Message.Value;
            });
        }));
        REQUIRE(Channel.TryEmplace<FTriggerMessage>());
        REQUIRE(Channel.TryEmplace<FSecondaryMessage>(10));

        const FMessageDispatchResult FirstResult = Channel.Dispatch();

        CHECK(BindSucceeded);
        CHECK_EQ(FirstResult.DispatchedCount, 1);
        CHECK_EQ(FirstResult.UnhandledCount, 1);
        CHECK_EQ(ReceivedValue, 0);

        REQUIRE(Channel.TryEmplace<FSecondaryMessage>(20));
        const FMessageDispatchResult SecondResult = Channel.Dispatch();

        CHECK_EQ(SecondResult.DispatchedCount, 1);
        CHECK_EQ(SecondResult.UnhandledCount, 0);
        CHECK_EQ(ReceivedValue, 20);
    }

    TEST_CASE("dispatch state is restored after a handler throws") {
        FMessageChannel Channel;
        int InvocationCount = 0;

        REQUIRE(Channel.TryBind<FNumberMessage>([&InvocationCount](const FNumberMessage& Message) {
            ++InvocationCount;
            if (Message.Value == 1) {
                throw std::runtime_error("expected test exception");
            }
        }));
        REQUIRE(Channel.TryEmplace<FNumberMessage>(1));
        REQUIRE(Channel.TryEmplace<FNumberMessage>(2));

        CHECK_THROWS_AS(Channel.Dispatch(), std::runtime_error);
        CHECK_EQ(InvocationCount, 1);
        CHECK_EQ(Channel.Size(), 1);

        const FMessageDispatchResult RecoveryResult = Channel.Dispatch();

        CHECK_EQ(RecoveryResult.DispatchedCount, 1);
        CHECK_EQ(InvocationCount, 2);
        CHECK(Channel.IsEmpty());
    }

    TEST_CASE("a heap-stored message is delivered and destroyed exactly once") {
        FMessageChannel Channel;
        int DestructionCount = 0;
        int ReceivedValue = 0;

        REQUIRE(sizeof(FLargeTrackedMessage) > FMessage::InlineSize);
        REQUIRE(Channel.TryBind<FLargeTrackedMessage>([&ReceivedValue](const FLargeTrackedMessage& Message) {
            ReceivedValue = Message.Value;
        }));
        REQUIRE(Channel.TryEmplace<FLargeTrackedMessage>(91, DestructionCount));

        CHECK_EQ(DestructionCount, 0);

        const FMessageDispatchResult Result = Channel.Dispatch();

        CHECK_EQ(Result.DispatchedCount, 1);
        CHECK_EQ(ReceivedValue, 91);
        CHECK_EQ(DestructionCount, 1);
    }
}
