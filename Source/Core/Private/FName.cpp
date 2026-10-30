/**
 * @file FName.cpp
 * @brief 대소문자 구분 없는 공유 이름 식별자입니다.
 * @ingroup Core
 */

#include "Core/FName.h"

#include <mutex>
#include <unordered_map>

FName::FName(std::string_view Name) {
    if (Name.empty()) {
        return;
    }

    static std::mutex Mutex{};
    static std::unordered_map<FString, std::shared_ptr<const FString>> Names{};
    FString Key{Name};

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
        mValue = std::make_shared<const FString>(Name);
        Names.emplace(std::move(Key), mValue);
    }
}

bool FName::operator==(const FName& Other) const {
    return mValue == Other.mValue;
}

bool FName::operator!=(const FName& Other) const {
    return !(*this == Other);
}

bool FName::IsNone() const {
    return mValue == nullptr;
}

const FString& FName::ToString() const {
    static const FString Empty{};
    return mValue != nullptr ? *mValue : Empty;
}

std::size_t FName::GetHash() const {
    return std::hash<const FString*>{}(mValue.get());
}

std::size_t FNameHash::operator()(const FName& Name) const {
    return Name.GetHash();
}
