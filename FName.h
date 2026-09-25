#pragma once
#include "STL.h"
#include "Common.h"
#include <string_view>

// Max size of name, including the null terminator
enum {
    NameSize = 1024
};

struct FNameEntryId {
    FNameEntryId();

    Uint32 ToUnstableInt() const;

    static FNameEntryId FromUnstableInt(Uint32 UnstableInt);

    // operator
    bool operator==(const FNameEntryId& Rhs) const;

    bool operator!=(const FNameEntryId& Rhs) const;

private:
    Uint32 mValue{};
};

struct FNameEntryHeader {
    Uint16 mBIsWide : 1 {0};
    Uint16 mLen : 15 {0};
};

struct FNameEntry {
public:
    FNameEntry(const FNameEntry&) = delete;
    FNameEntry(FNameEntry&&) = delete;
    FNameEntry& operator=(const FNameEntry&) = delete;
    FNameEntry& operator=(FNameEntry&&) = delete;

    bool IsWide() const;

    Int32 GetNameLength() const;

    FNameEntryId GetComparisonId() const;

    void SetComparisonId(FNameEntryId NewId);

    const char* GetName() const;

private:
    FNameEntryHeader mHeader{};
    FNameEntryId mComparisonId{};
    Uint8 mNameData[0]{};
};

class FName {
public:
    constexpr FName() = default;
    FName(std::string_view Str);
    FName(const char* PStr);
    FName(FString Str);
    FName(std::string_view BaseName, Int32 InNumber);

    Int32 Compare(const FName& Rhs) const;
    bool operator==(const FName& Rhs) const;
    bool operator<(const FName& Rhs) const;

    FString ToString() const;

    FNameEntryId GetDisplayId() const;

    FNameEntryId GetComparisonId() const;

    Int32 GetNumber() const;

private:
    FNameEntryId mDisplayId{};
    FNameEntryId mComparisonId{};
    Int32 mNumber{0};
};

void SplitNameAndNumber(std::string_view InString, std::string_view& OutString, Int32& OutNumber);
