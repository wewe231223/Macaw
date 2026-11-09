/**
 * @file FName.cpp
 * @brief 대소문자 구분 없는 공유 이름 식별자입니다.
 * @ingroup Core
 */

#include "Core/FName.h"

#include <mutex>
#include <unordered_map>

namespace Core {

    /**
     * @brief 입력 이름을 ASCII 대소문자 구분 없이 공유 이름 풀에 등록합니다.
     */
    Core::FName::FName(std::string_view Name) {
        if (Name.empty()) {
            return;
        }

        static std::mutex Mutex{};
        static std::unordered_map<Core::FString, std::shared_ptr<const Core::FString>> Names{};
        Core::FString Key{Name};

        for (char& Character : Key) {
            if (Character >= 'A' && Character <= 'Z') {
                Character = static_cast<char>(Character - 'A' + 'a');
            }
        }

        const std::lock_guard Lock{Mutex};
        const auto Existing{Names.find(Key)};

        if (Existing != Names.end()) {
            mValue = Existing->second;
        } else {
            mValue = std::make_shared<const Core::FString>(Name);

            Names.emplace(std::move(Key), mValue);
        }
    }

    /**
     * @brief 공유 이름의 동일성으로 대소문자를 구분하지 않고 비교합니다.
     */
    bool Core::FName::operator==(const Core::FName& Other) const {
        return mValue == Other.mValue;
    }

    /**
     * @brief 두 공유 이름 식별자가 서로 다른지 비교합니다.
     */
    bool Core::FName::operator!=(const Core::FName& Other) const {
        return !(*this == Other);
    }

    /**
     * @brief 공유 이름이 없는 기본 식별자인지 검사합니다.
     */
    bool Core::FName::IsNone() const {
        return mValue == nullptr;
    }

    /**
     * @brief 이 이름이 처음 등록되었을 때의 표기 문자열을 반환합니다.
     */
    const Core::FString& Core::FName::ToString() const {
        static const Core::FString Empty{};

        return mValue != nullptr ? *mValue : Empty;
    }

    /**
     * @brief 동등한 이름에 같은 값을 부여하는 프로세스 내부 해시를 반환합니다.
     */
    std::size_t Core::FName::GetHash() const {
        return std::hash<const Core::FString*>{}(mValue.get());
    }

    /**
     * @brief FName 동등 비교와 일치하는 해시를 반환합니다.
     */
    std::size_t Core::FNameHash::operator()(const Core::FName& Name) const {
        return Name.GetHash();
    }

}
