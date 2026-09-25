#include "pch.h"
#include "FGuid.h"

#include <format>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <combaseapi.h>

#include <string>

FGuid FGuid::NewGuid() {
    FGuid NewGuid{0, 0, 0, 0};

    GUID WinGuid{};
    if (CoCreateGuid(&WinGuid) == S_OK) {
        std::memcpy(&NewGuid, &WinGuid, sizeof(FGuid));
    }

    return NewGuid;
}

FString FGuid::ToString() const {
    const std::string Formatted{std::format("{:08X}-{:04X}-{:04X}-{:04X}-{:04X}{:08X}", mA, mB >> 16, mB & 0xFFFF, mC >> 16, mC & 0xFFFF, mD)};

    return FString{Formatted.begin(), Formatted.end()};
}

bool FGuid::Parse(const FString& GuidString) {
    Uint32 ParsedValues[4]{0, 0, 0, 0};
    Int32 ChunkIndex{0};
    Int32 CharCount{0};

    for (auto Ch : GuidString) {
        if (Ch == '-')
            continue;

        Uint32 HexValue{0};
        if (Ch >= '0' && Ch <= '9') {
            HexValue = Ch - '0';
        } else if (Ch >= 'a' && Ch <= 'f') {
            HexValue = Ch - 'a' + 10;
        } else if (Ch >= 'A' && Ch <= 'F') {
            HexValue = Ch - 'A' + 10;
        } else
            return false; // 유효하지 않은 문자(16진수가 아닌 문자)

        // 기존 값에 16(<< 4)을 곱하고 새로운 16진수 값을 더함
        ParsedValues[ChunkIndex] = (ParsedValues[ChunkIndex] << 4) | HexValue;
        CharCount++;

        // 8글자(32비트)를 다 채웠으면 다음 변수(A->B->C->D)로 이동
        if (CharCount == 8) {
            ChunkIndex++;
            CharCount = 0;

            // 32개의 16진수(8글자 * 4)를 모두 찾았으면 파싱 종료
            if (ChunkIndex == 4) {
                break;
            }
        }
    }

    // 끝까지 파싱했는데 32글자(128비트)를 모두 채우지 못했다면 실패
    if (ChunkIndex != 4)
        return false;

    mA = ParsedValues[0];
    mB = ParsedValues[1];
    mC = ParsedValues[2];
    mD = ParsedValues[3];

    return true;
}

bool FGuid::IsValid() const {
    return (mA | mB | mC | mD) != 0;
}

std::size_t FGuid::GetHash() const noexcept {
    std::size_t Hash{std::hash<Uint32>{}(mA)};
    Hash ^= std::hash<Uint32>{}(mB) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
    Hash ^= std::hash<Uint32>{}(mC) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
    Hash ^= std::hash<Uint32>{}(mD) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
    return Hash;
}

bool FGuid::operator==(const FGuid& Other) const {
    return (mA == Other.mA) && (mB == Other.mB) && (mC == Other.mC) && (mD == Other.mD);
}

bool FGuid::operator!=(const FGuid& Other) const {
    return !(*this == Other);
}

std::size_t std::hash<FGuid>::operator()(const FGuid& Guid) const noexcept {
    return Guid.GetHash();
}
