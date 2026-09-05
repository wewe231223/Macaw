#include "../doctest/doctest.h"

#include <memory>
#include <string>

#include "../Channel/FStateChannel.h"

TEST_SUITE("TStateChannel") {
    TEST_CASE("a default channel has no value and no unread change") {
        TStateChannel<int> Channel;
        auto Reader = Channel.GetReader();
        auto Writer = Channel.GetWriter();

        CHECK_FALSE(Reader.HasValue());
        CHECK_FALSE(Writer.HasValue());
        CHECK_FALSE(Reader.HasChanged());
        CHECK_EQ(Reader.Peek(), nullptr);
        CHECK_EQ(Reader.Read(), nullptr);

        const auto Result = Reader.ReadIfChanged();
        CHECK_FALSE(Result.Changed);
        CHECK_EQ(Result.Value, nullptr);
    }

    TEST_CASE("readers observe writes independently") {
        TStateChannel<int> Channel;
        auto FirstReader = Channel.GetReader();
        auto SecondReader = Channel.GetReader();
        auto Writer = Channel.GetWriter();

        Writer.Write(42);

        CHECK(FirstReader.HasChanged());
        CHECK(SecondReader.HasChanged());

        const auto FirstResult = FirstReader.ReadIfChanged();
        REQUIRE(FirstResult.Value != nullptr);
        CHECK(FirstResult.Changed);
        CHECK_EQ(*FirstResult.Value, 42);
        CHECK_FALSE(FirstReader.HasChanged());
        CHECK(SecondReader.HasChanged());

        const int* SecondValue = SecondReader.Read();
        REQUIRE(SecondValue != nullptr);
        CHECK_EQ(*SecondValue, 42);
        CHECK_FALSE(SecondReader.HasChanged());
    }

    TEST_CASE("emplace and modify update the state") {
        TStateChannel<std::string> Channel;
        auto Reader = Channel.GetReader();
        auto Writer = Channel.GetWriter();

        const std::string& Value = Writer.Emplace("Macaw");
        CHECK_EQ(Value, "Macaw");
        CHECK(Reader.HasChanged());

        CHECK(Writer.Modify([](std::string& State) {
            State += " Channel";
        }));

        const std::string* ModifiedValue = Reader.Read();
        REQUIRE(ModifiedValue != nullptr);
        CHECK_EQ(*ModifiedValue, "Macaw Channel");
        CHECK_FALSE(Reader.HasChanged());
    }

    TEST_CASE("modify fails without a value and clear is observable") {
        TStateChannel<int> Channel{ 7 };
        auto Reader = Channel.GetReader();
        auto Writer = Channel.GetWriter();

        REQUIRE(Reader.Read() != nullptr);
        CHECK_FALSE(Reader.HasChanged());

        Writer.Clear();

        CHECK_FALSE(Writer.HasValue());
        CHECK(Reader.HasChanged());

        const auto ClearedResult = Reader.ReadIfChanged();
        CHECK(ClearedResult.Changed);
        CHECK_EQ(ClearedResult.Value, nullptr);
        CHECK_FALSE(Reader.HasChanged());
        CHECK_FALSE(Writer.Modify([](int& State) {
            ++State;
        }));
    }

    TEST_CASE("an initial value is reported as an unread change") {
        TStateChannel<std::string> Channel{ std::in_place, 3, 'x' };
        auto Reader = Channel.GetReader();

        CHECK(Reader.HasValue());
        CHECK(Reader.HasChanged());
        REQUIRE(Reader.Peek() != nullptr);
        CHECK_EQ(*Reader.Peek(), "xxx");
        CHECK(Reader.HasChanged());

        const auto Result = Reader.ReadIfChanged();

        REQUIRE(Result.Value != nullptr);
        CHECK(Result.Changed);
        CHECK_EQ(*Result.Value, "xxx");
        CHECK_FALSE(Reader.HasChanged());
    }

    TEST_CASE("peek does not acknowledge a change") {
        TStateChannel<int> Channel;
        auto Reader = Channel.GetReader();
        auto Writer = Channel.GetWriter();

        Writer.Write(5);

        REQUIRE(Reader.Peek() != nullptr);
        CHECK_EQ(*Reader.Peek(), 5);
        CHECK(Reader.HasChanged());

        REQUIRE(Reader.Read() != nullptr);
        CHECK_FALSE(Reader.HasChanged());
    }

    TEST_CASE("writing the same value still creates a new version") {
        TStateChannel<int> Channel{ 10 };
        auto Reader = Channel.GetReader();
        auto Writer = Channel.GetWriter();

        REQUIRE(Reader.Read() != nullptr);
        CHECK_FALSE(Reader.HasChanged());

        Writer.Write(10);

        CHECK(Reader.HasChanged());
        const auto Result = Reader.ReadIfChanged();
        REQUIRE(Result.Value != nullptr);
        CHECK_EQ(*Result.Value, 10);
        CHECK(Result.Changed);
    }

    TEST_CASE("clearing an already empty channel does not create a change") {
        TStateChannel<int> Channel{ 1 };
        auto Reader = Channel.GetReader();
        auto Writer = Channel.GetWriter();

        REQUIRE(Reader.Read() != nullptr);
        Writer.Clear();
        CHECK(Reader.HasChanged());
        CHECK_EQ(Reader.Read(), nullptr);
        CHECK_FALSE(Reader.HasChanged());

        Writer.Clear();

        CHECK_FALSE(Reader.HasChanged());
        CHECK_FALSE(Writer.HasValue());
    }

    TEST_CASE("copied readers retain their own read position") {
        TStateChannel<int> Channel{ 3 };
        auto OriginalReader = Channel.GetReader();
        auto Writer = Channel.GetWriter();

        REQUIRE(OriginalReader.Read() != nullptr);
        auto CopiedReader = OriginalReader;

        CHECK_FALSE(OriginalReader.HasChanged());
        CHECK_FALSE(CopiedReader.HasChanged());

        Writer.Write(4);

        CHECK(OriginalReader.HasChanged());
        CHECK(CopiedReader.HasChanged());
        REQUIRE(OriginalReader.Read() != nullptr);
        CHECK_FALSE(OriginalReader.HasChanged());
        CHECK(CopiedReader.HasChanged());
    }

    TEST_CASE("move-only state can be written and modified") {
        TStateChannel<std::unique_ptr<int>> Channel;
        auto Reader = Channel.GetReader();
        auto Writer = Channel.GetWriter();

        Writer.Write(std::make_unique<int>(12));

        const std::unique_ptr<int>* WrittenValue = Reader.Read();
        REQUIRE(WrittenValue != nullptr);
        REQUIRE(*WrittenValue != nullptr);
        CHECK_EQ(**WrittenValue, 12);

        CHECK(Writer.Modify([](std::unique_ptr<int>& State) {
            *State = 24;
        }));
        CHECK(Reader.HasChanged());

        const std::unique_ptr<int>* ModifiedValue = Reader.Read();
        REQUIRE(ModifiedValue != nullptr);
        REQUIRE(*ModifiedValue != nullptr);
        CHECK_EQ(**ModifiedValue, 24);
    }
}
