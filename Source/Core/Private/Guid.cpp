#include "Core/Guid.h"
#include "Core/Base/ErrorHandler.h"

#include <Windows.h>
#include <combaseapi.h>
#include <algorithm>
#include <bit>

FGuid FGuid::NewGuid() {
    GUID Native{};
    ErrorHandler::ReportHRESULT(CoCreateGuid(&Native), "UUID", "UUID creation failed", ErrorHandler::EErrorLevel::Critical);

    return std::bit_cast<FGuid>(Native);
}

bool FGuid::Parse(const FString& Text, FGuid& Result) {
    if (Text.size() != 36 || Text[8] != '-' || Text[13] != '-' || Text[18] != '-' || Text[23] != '-') {
        return false;
    }

    FGuid Parsed{};
    std::size_t ByteIndex{};
    Int32 High{-1};

    for (char Character : Text) {
        if (Character == '-') {
            continue;
        }

        const Int32 Digit{Character >= '0' && Character <= '9' ? Character - '0' : Character >= 'a' && Character <= 'f' ? Character - 'a' + 10 : Character >= 'A' && Character <= 'F' ? Character - 'A' + 10 : -1};

        if (Digit < 0) {
            return false;
        }

        if (High < 0) {
            High = Digit;
        } else {
            Parsed.mBytes[ByteIndex++] = static_cast<Uint8>(High * 16 + Digit);
            High = -1;
        }
    }

    Result = Parsed;

    return Result.IsValid();
}

FString FGuid::ToString() const {
    constexpr char Digits[]{"0123456789ABCDEF"};
    FString Result{};
    Result.reserve(36);

    for (std::size_t Index{}; Index < mBytes.size(); ++Index) {
        if (Index == 4 || Index == 6 || Index == 8 || Index == 10) {
            Result.push_back('-');
        }

        Result.push_back(Digits[mBytes[Index] >> 4]);
        Result.push_back(Digits[mBytes[Index] & 15]);
    }

    return Result;
}

bool FGuid::IsValid() const {
    return std::any_of(mBytes.begin(), mBytes.end(), [](Uint8 Byte) {
        return Byte != 0;
    });
}

bool FGuid::operator==(const FGuid& Other) const {
    return mBytes == Other.mBytes;
}
