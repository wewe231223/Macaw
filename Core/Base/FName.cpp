#include "pch.h"
#include "FName.h"

FNameEntryId::FNameEntryId()
    : mValue(0) {
}

Uint32 FNameEntryId::ToUnstableInt() const {
    return mValue;
}

#include "Core/Hash/city.h"

static constexpr Uint32 FNameMaxBlockBits{13};
static constexpr Uint32 FNameBlockOffsetBits{16};
static constexpr Uint32 FNameMaxBlocks{1 << FNameMaxBlockBits};
static constexpr Uint32 FNameBlockOffsets{1 << FNameBlockOffsetBits};

static constexpr Uint32 EntryIdBits{FNameMaxBlockBits + FNameBlockOffsetBits};
static constexpr Uint32 EntryIdMask{(1 << EntryIdBits) - 1};
static constexpr Uint32 ProbeHashShift{EntryIdBits};
static constexpr Uint32 ProbeHashMask{~EntryIdMask};

// Unpacked FNameEntryId to Block and Offset
struct FNameEntryHandle {
    Uint32 mBlock{0};
    Uint32 mOffset{0};

    FNameEntryHandle(Uint32 InBlock, Uint32 InOffset)
        : mBlock(InBlock),
          mOffset(InOffset) {
    }

    FNameEntryHandle(FNameEntryId Id)
        : mBlock(Id.ToUnstableInt() >> FNameBlockOffsetBits),
          mOffset(Id.ToUnstableInt() & (FNameBlockOffsets - 1)) {
    }

    operator FNameEntryId() const {
        return FNameEntryId::FromUnstableInt(mBlock << FNameBlockOffsetBits | mOffset);
    }

    explicit operator bool() const {
        return mBlock | mOffset;
    }
};

// FNameSlot
// Hash and Id
struct FNameSlot {
    FNameSlot() {
    }

    FNameSlot(FNameEntryId Value, Uint32 ProbeHash)
        : mIdAndHash(Value.ToUnstableInt() | ProbeHash) {
    }

    FNameEntryId GetId() const {
        return FNameEntryId::FromUnstableInt(mIdAndHash & EntryIdMask);
    }

    Uint32 GetProbeHash() const {
        return mIdAndHash & ProbeHashMask;
    }

    bool Used() const {
        return mIdAndHash != 0;
    }

private:
    Uint32 mIdAndHash{0};
};

// FNameEntryAllocator
// Allocate memory to FNameEntry
class FNameEntryAllocator {
public:
    enum { Stride = alignof(FNameEntry) };

    enum { BlockSizeBytes = Stride * FNameBlockOffsets };

    FNameEntryAllocator() {
        mBlocks[0] = new Uint8[BlockSizeBytes]();
        mCurrentByteCursor = Stride;
    }

    ~FNameEntryAllocator() {
        for (Int32 Index{static_cast<Int32>(mCurrentBlock)}; Index >= 0; --Index) {
            delete[] mBlocks[Index];
        }
    }

    FNameEntryHandle Allocate(Uint32 Bytes) {
        Uint32 Step{(Bytes + Stride - 1) & ~(Stride - 1)};

        if (mCurrentByteCursor + Step > BlockSizeBytes) {
            AllocateNewBlock();
        }

        Uint32 ByteOffset{mCurrentByteCursor};
        mCurrentByteCursor += Step;

        return FNameEntryHandle(mCurrentBlock, ByteOffset / Stride);
    }

    FNameEntry& Resolve(FNameEntryHandle Handle) const {
        return *reinterpret_cast<FNameEntry*>(mBlocks[Handle.mBlock] + Stride * Handle.mOffset);
    }

    void AllocateNewBlock() {
        ++mCurrentBlock;
        mCurrentByteCursor = 0;

        if (mBlocks[mCurrentBlock] == nullptr) {
            mBlocks[mCurrentBlock] = new Uint8[BlockSizeBytes]();
        }
    }

private:
    Uint32 mCurrentBlock{0};
    Uint32 mCurrentByteCursor{0};
    Uint8* mBlocks[FNameMaxBlocks]{};
};

// FNameHash
// Hash(Uint32) and ProbeHash(Uint32)
struct FNameHash {
    Uint32 mHash{};
    Uint32 mProbeHash{};

    static Uint64 GenerateHash(const char* Str, std::size_t Len) {
        return CityHash64(Str, Len);
    }

    FNameHash(const char* Str, Int32 Len)
        : FNameHash(GenerateHash(Str, Len), Len) {
    }

    FNameHash()
        : mHash(0),
          mProbeHash(0) {
    }

    FNameHash(Uint64 InHash, Int32 Len) {
        Uint32 Hi{static_cast<Uint32>(InHash >> 32)};
        Uint32 Lo{static_cast<Uint32>(InHash & 0xFFFFFFFF)};

        mHash = Lo;
        mProbeHash = Hi & ProbeHashMask;
    }
};

// FNameValue
// Name(string_view), Hash(FNameHash)
struct FNameValue {
    FNameValue(std::string_view InName)
        : mName(InName) {
    }

    FNameValue(std::string_view InName, FNameHash InHash)
        : mName(InName),
          mHash(InHash) {
    }

    std::string_view mName{};
    FNameHash mHash{};
};

// FNameComparisonValue
// Only for using lower(Not display)
struct FNameComparisonValue : public FNameValue {
    FNameComparisonValue(std::string_view InName)
        : FNameValue(InName) {
        // Stack buffer
        char LowerBuffer[NameSize]{};

        std::size_t Len{std::min(InName.length(), std::size_t(NameSize - 1))};

        for (std::size_t I{0}; I < Len; ++I) {
            LowerBuffer[I] = static_cast<char>(std::tolower(InName[I]));
        }

        mHash = FNameHash(LowerBuffer, static_cast<Int32>(Len));
    }
};

// FNameDisplayValue
struct FNameDisplayValue : public FNameValue {
    FNameDisplayValue(std::string_view InName)
        : FNameValue(InName) {
        mHash = FNameHash(InName.data(), static_cast<Int32>(InName.length()));
    }
};

// FNamePool
// HashBuckets, Entries
class FNamePool {
public:
    static FNamePool& Get() {
        static FNamePool Instance{};
        return Instance;
    }

    FNameEntryId Find(std::string_view NameString) const {
        if (NameString.empty()) {
            return FNameEntryId();
        }

        if (NameString.length() >= NameSize) {
            assert(false && "FName string too long! FName is only meant for identifiers (<= 1023 chars).");

            NameString = NameString.substr(0, NameSize - 1);
        }

        // Display
        FNameDisplayValue DisplayValue{NameString};
        FNameEntryId Existing{FNamePool::FindValue(mDisplayHashBuckets, DisplayValue, true)};
        if (Existing.ToUnstableInt() != 0) {
            return Existing;
        }

        // Comparison
        FNameComparisonValue ComparisonValue{NameString};

        return FNamePool::FindValue(mComparisonHashBuckets, ComparisonValue, false);
    }

    FNameEntryId Store(std::string_view NameString) {
        if (NameString.empty()) {
            return FNameEntryId();
        }

        if (NameString.length() >= NameSize) {
            assert(false && "FName string too long! FName is only meant for identifiers (<= 1023 chars).");

            NameString = NameString.substr(0, NameSize - 1);
        }

        FNameDisplayValue DisplayValue{NameString};
        FNameEntryId Existing{FNamePool::FindValue(mDisplayHashBuckets, DisplayValue, true)};
        if (Existing.ToUnstableInt() != 0) {
            return Existing;
        }

        bool BAdded{false};
        FNameComparisonValue ComparisonValue{NameString};
        FNameEntryId ComparisonId{StoreComparisonValue(ComparisonValue, BAdded)};

        return StoreDisplayValue(DisplayValue, ComparisonId, BAdded);
    }

    const FNameEntry& Resolve(FNameEntryId Id) const {
        return mEntries.Resolve(Id);
    }

private:
    FNamePool() {
        Initialize(1 << 20);
    }

    void Initialize(Uint32 InitialCapacity) {
        mComparisonHashBuckets.assign(InitialCapacity, FNameSlot());
        mDisplayHashBuckets.assign(InitialCapacity, FNameSlot());
    }

    FNameEntryId FindValue(const TArray<FNameSlot>& Buckets, const FNameValue& InValue, bool BIsCaseSensitive) const {
        Uint32 CapacityMask{static_cast<Uint32>(Buckets.size() - 1)};
        Uint32 SlotIndex{InValue.mHash.mHash & CapacityMask};

        while (Buckets[SlotIndex].Used()) {
            if (Buckets[SlotIndex].GetProbeHash() == InValue.mHash.mProbeHash) {
                FNameEntryId ExistingId{Buckets[SlotIndex].GetId()};
                const FNameEntry& Entry{Resolve(ExistingId)};

                const char* ExistingStr{Entry.GetName()};
                if (Entry.GetNameLength() == InValue.mName.length()) {
                    bool BIsMatch{true};
                    if (BIsCaseSensitive) {
                        BIsMatch = (std::memcmp(ExistingStr, InValue.mName.data(), InValue.mName.length()) == 0);
                    } else {
                        BIsMatch = (_strnicmp(ExistingStr, InValue.mName.data(), InValue.mName.length()) == 0);
                    }

                    if (BIsMatch) {
                        return ExistingId;
                    }
                }
            }

            SlotIndex = (SlotIndex + 1) & CapacityMask;
        }

        return FNameEntryId();
    }

    FNameEntryId StoreValue(TArray<FNameSlot>& Buckets, const FNameValue& InValue, bool BIsCaseSensitive) {
        FNameEntryId ExistingId{FNamePool::FindValue(Buckets, InValue, BIsCaseSensitive)};
        if (ExistingId.ToUnstableInt() != 0) {
            return ExistingId;
        }

        // Write Memory
        Uint32 NeededByte{static_cast<Uint32>(sizeof(FNameEntryHeader) + InValue.mName.length() + 1)}; // 1 : null terminator
        FNameEntryHandle NewHandle{mEntries.Allocate(NeededByte)};

        // Set header
        FNameEntry& NewEntry{mEntries.Resolve(NewHandle)};
        Uint16* HeaderPtr{reinterpret_cast<Uint16*>(&NewEntry)};
        *HeaderPtr = static_cast<Uint16>(InValue.mName.length()) << 1;
        // Set string
        char* DataPtr{const_cast<char*>(NewEntry.GetName())};
        std::memcpy(DataPtr, InValue.mName.data(), InValue.mName.length());
        DataPtr[InValue.mName.length()] = '\0'; // null terminator

        Uint32 CapacityMask{static_cast<Uint32>(Buckets.size() - 1)};
        Uint32 SlotIndex{InValue.mHash.mHash & CapacityMask};
        Uint32 Probes{0};
        const Uint32 MaxProbes{static_cast<Uint32>(Buckets.size())};

        while (Buckets[SlotIndex].Used()) {
            if (++Probes >= MaxProbes) {
                assert(false && "FNamePool out of memory!");
                std::abort();
            }

            SlotIndex = (SlotIndex + 1) & CapacityMask;
        }

        Buckets[SlotIndex] = FNameSlot(NewHandle, InValue.mHash.mProbeHash);

        return NewHandle;
    }

    FNameEntryId StoreComparisonValue(const FNameValue& InValue, bool& BOutAdded) {
        FNameEntryId ExistingId{FNamePool::FindValue(mComparisonHashBuckets, InValue, false)};
        if (ExistingId.ToUnstableInt() != 0) {
            return ExistingId;
        }

        BOutAdded = true;

        // Write Memory
        Uint32 NeededByte{static_cast<Uint32>(sizeof(FNameEntry) + InValue.mName.length() + 1)}; // 1 : null terminator
        FNameEntryHandle NewHandle{mEntries.Allocate(NeededByte)};

        // Set header
        FNameEntry& NewEntry{mEntries.Resolve(NewHandle)};
        Uint16* HeaderPtr{reinterpret_cast<Uint16*>(&NewEntry)};
        *HeaderPtr = static_cast<Uint16>(InValue.mName.length()) << 1;
        // Set string
        char* DataPtr{const_cast<char*>(NewEntry.GetName())};
        std::memcpy(DataPtr, InValue.mName.data(), InValue.mName.length());
        DataPtr[InValue.mName.length()] = '\0'; // null terminator

        NewEntry.SetComparisonId(NewHandle);

        InsertSlot(mComparisonHashBuckets, InValue, NewHandle);

        return NewHandle;
    }

    FNameEntryId StoreDisplayValue(const FNameValue& InValue, FNameEntryId InComparisonId, bool BWasAdded) {
        if (BWasAdded) {
            InsertSlot(mDisplayHashBuckets, InValue, InComparisonId);
            return InComparisonId;
        }

        // Write Memory
        Uint32 NeededByte{static_cast<Uint32>(sizeof(FNameEntry) + InValue.mName.length() + 1)}; // 1 : null terminator
        FNameEntryHandle NewHandle{mEntries.Allocate(NeededByte)};

        // Set header
        FNameEntry& NewEntry{mEntries.Resolve(NewHandle)};
        Uint16* HeaderPtr{reinterpret_cast<Uint16*>(&NewEntry)};
        *HeaderPtr = static_cast<Uint16>(InValue.mName.length()) << 1;
        // Set string
        char* DataPtr{const_cast<char*>(NewEntry.GetName())};
        std::memcpy(DataPtr, InValue.mName.data(), InValue.mName.length());
        DataPtr[InValue.mName.length()] = '\0'; // null terminator

        NewEntry.SetComparisonId(InComparisonId);

        InsertSlot(mDisplayHashBuckets, InValue, NewHandle);

        return NewHandle;
    }

    void InsertSlot(TArray<FNameSlot>& Buckets, const FNameValue& InValue, FNameEntryId InEntryId) {
        Uint32 CapacityMask{static_cast<Uint32>(Buckets.size() - 1)};
        Uint32 SlotIndex{InValue.mHash.mHash & CapacityMask};
        Uint32 Probes{0};
        const Uint32 MaxProbes{static_cast<Uint32>(Buckets.size())};

        while (Buckets[SlotIndex].Used()) {
            if (++Probes >= MaxProbes) {
                assert(false && "FNamePool out of memory!");
                std::abort();
            }

            SlotIndex = (SlotIndex + 1) & CapacityMask;
        }

        Buckets[SlotIndex] = FNameSlot(InEntryId, InValue.mHash.mProbeHash);
    }

    FNameEntryAllocator mEntries{};

    TArray<FNameSlot> mComparisonHashBuckets{};
    TArray<FNameSlot> mDisplayHashBuckets{};
};

FName::FName(std::string_view Str) {
    if (Str.length() > 0) {
        std::string_view BaseStr{};
        SplitNameAndNumber(Str, BaseStr, mNumber);

        mDisplayId = FNamePool::Get().Store(BaseStr);
        mComparisonId = FNamePool::Get().Resolve(mDisplayId).GetComparisonId();
    }
}

FName::FName(const char* PStr)
    : FName(PStr ? FName{std::string_view(PStr)} : FName{}) {
}

FName::FName(FString Str)
    : FName(std::string_view(Str)) {
}

FName::FName(std::string_view BaseName, Int32 InNumber) {
    if (!BaseName.empty()) {
        mNumber = (InNumber >= 0) ? (InNumber + 1) : 0;
        mDisplayId = FNamePool::Get().Store(BaseName);
        mComparisonId = FNamePool::Get().Resolve(mDisplayId).GetComparisonId();
    }
}

Int32 FName::Compare(const FName& Rhs) const {
    return this->mComparisonId.ToUnstableInt() - Rhs.mComparisonId.ToUnstableInt();
}

bool FName::operator==(const FName& Rhs) const {
    return this->mComparisonId == Rhs.mComparisonId && this->mNumber == Rhs.mNumber;
}

bool FName::operator<(const FName& Rhs) const {
    return this->mComparisonId.ToUnstableInt() < Rhs.mComparisonId.ToUnstableInt();
}

FString FName::ToString() const {
    if (mComparisonId.ToUnstableInt() == 0) {
        return FString{""};
    }

    const FNameEntry& Entry{FNamePool::Get().Resolve(mDisplayId)};

    FString Result{FString{Entry.GetName(), static_cast<std::size_t>(Entry.GetNameLength())}};

    if (mNumber > 0) {
        Result += "_" + std::to_string(mNumber - 1);
    }

    return Result;
}

FNameEntryId FNameEntryId::FromUnstableInt(Uint32 UnstableInt) {
    FNameEntryId Id{};
    Id.mValue = UnstableInt;
    return Id;
}

bool FNameEntryId::operator==(const FNameEntryId& Rhs) const {
    return this->mValue == Rhs.mValue;
}

bool FNameEntryId::operator!=(const FNameEntryId& Rhs) const {
    return this->mValue != Rhs.mValue;
}

bool FNameEntry::IsWide() const {
    return mHeader.mBIsWide;
}

Int32 FNameEntry::GetNameLength() const {
    return mHeader.mLen;
}

FNameEntryId FNameEntry::GetComparisonId() const {
    return mComparisonId;
}

void FNameEntry::SetComparisonId(FNameEntryId NewId) {
    mComparisonId = NewId;
}

const char* FNameEntry::GetName() const {
    return (char*)mNameData;
}

FNameEntryId FName::GetDisplayId() const {
    return mDisplayId;
}

FNameEntryId FName::GetComparisonId() const {
    return mComparisonId;
}

Int32 FName::GetNumber() const {
    return mNumber;
}

void SplitNameAndNumber(std::string_view InString, std::string_view& OutString, Int32& OutNumber) {
    if (InString.empty()) {
        return;
    }

    OutString = InString;
    OutNumber = 0;

    const std::size_t Sep{InString.rfind('_')};
    if (Sep == std::string_view::npos || Sep == 0 || Sep + 1 == InString.length()) {
        return;
    }

    Int32 Num{0};
    for (std::size_t I{Sep + 1}; I < InString.length(); ++I) {
        if (!std::isdigit(static_cast<unsigned char>(InString[I]))) {
            return;
        }

        Num = Num * 10 + (InString[I] - '0');
    }

    OutString = InString.substr(0, Sep);
    OutNumber = Num + 1;
}
