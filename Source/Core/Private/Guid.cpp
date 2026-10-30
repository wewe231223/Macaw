/**
 * @file Guid.cpp
 * @brief GUID 생성, 파싱과 문자열 변환을 제공합니다.
 * @ingroup Core
 */

#include "Core/Guid.h"
#include "Core/Base/ErrorHandler.h"

#include <Windows.h>
#include <combaseapi.h>
#include <algorithm>
#include <bit>

namespace Core {

    /**
     * @brief 새로운 GUID를 생성합니다.
     */
    Core::FGuid::FGuid() {
        GUID Native{};
        Core::ErrorHandler::ReportHRESULT(CoCreateGuid(&Native), "UUID", "UUID creation failed", Core::ErrorHandler::EErrorLevel::Critical);

        mBytes = std::bit_cast<std::array<Core::Uint8, 16>>(Native);
    }

    /**
     * @brief GUID 문자열을 검증하여 결과에 기록합니다.
     */
    bool Core::FGuid::Parse(const Core::FString& Text, Core::FGuid& Result) {
        if (Text.size() != 36 || Text[8] != '-' || Text[13] != '-' || Text[18] != '-' || Text[23] != '-') {
            return false;
        }

        std::array<Core::Uint8, 16> Bytes{};
        std::size_t ByteIndex{};
        Core::Int32 High{-1};

        for (char Character : Text) {
            if (Character == '-') {
                continue;
            }

            const Core::Int32 Digit{Character >= '0' && Character <= '9' ? Character - '0' : Character >= 'a' && Character <= 'f' ? Character - 'a' + 10 : Character >= 'A' && Character <= 'F' ? Character - 'A' + 10 : -1};

            if (Digit < 0) {
                return false;
            }

            if (High < 0) {
                High = Digit;
            } else {
                Bytes[ByteIndex++] = static_cast<Core::Uint8>(High * 16 + Digit);
                High = -1;
            }
        }

        Result.mBytes = Bytes;

        return Result.IsValid();
    }

    /**
     * @brief GUID를 정규 문자열 표현으로 변환합니다.
     */
    Core::FString Core::FGuid::ToString() const {
        constexpr char Digits[]{"0123456789ABCDEF"};
        Core::FString Result{};
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

    /**
     * @brief GUID가 모두 0인 무효 값이 아닌지 검사합니다.
     */
    bool Core::FGuid::IsValid() const {
        return std::any_of(mBytes.begin(), mBytes.end(), [](Core::Uint8 Byte) {
            return Byte != 0;
        });
    }

    /**
     * @brief 16바이트 GUID 값이 모두 같은지 검사합니다.
     */
    bool Core::FGuid::operator==(const Core::FGuid& Other) const {
        return mBytes == Other.mBytes;
    }

}
