/**
 * @file FName.cpp
 * @brief 대소문자 구분 없는 공유 이름 식별자입니다.
 * @ingroup Core
 */

#include "Core/FName.h"

#include <mutex>
#include <unordered_map>

namespace Core {

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

    bool Core::FName::operator==(const Core::FName& Other) const {
        return mValue == Other.mValue;
    }

    bool Core::FName::operator!=(const Core::FName& Other) const {
        return !(*this == Other);
    }

    bool Core::FName::IsNone() const {
        return mValue == nullptr;
    }

    const Core::FString& Core::FName::ToString() const {
        static const Core::FString Empty{};
        return mValue != nullptr ? *mValue : Empty;
    }

    std::size_t Core::FName::GetHash() const {
        return std::hash<const Core::FString*>{}(mValue.get());
    }

    std::size_t Core::FNameHash::operator()(const Core::FName& Name) const {
        return Name.GetHash();
    }

}
