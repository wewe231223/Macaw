/**
 * @file TObjectIterator.h
 * @brief 등록된 UObject를 리플렉션 타입별로 순회하는 반복자와 범위를 제공합니다.
 */

#pragma once
#include "CoreUObject/Reflection/UClass.h"

#include "Core/CoreMinimal.h"
#include "CoreUObject/UObject.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/UObjectSystem.h"
#include "Core/Base/ErrorHandler.h"

#include <concepts>
#include <cstddef>

/// <summary>
///
/// </summary>
namespace UObjectSystem {
    // UObject에서 파생된 타입만 사용가능
    /**
     * @brief 빈 슬롯을 건너뛰며 TObject와 그 파생 객체를 순회합니다.
     * @tparam TObject UObject 파생 리플렉션 타입입니다.
     * @note 끝 인덱스를 저장하며 객체 수명을 소유하지 않습니다. 순회 중 추가된 객체는 저장된 범위 안에 있을 때만 탐색할 수 있습니다.
     */
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    class TObjectIterator {
    public:
        // 순회 범위 (start, end - 1까지)
        /**
         * @brief [Start, End) 슬롯 범위에서 첫 타입 호환 객체를 찾습니다.
         * @param Start 탐색을 시작할 슬롯입니다.
         * @param End 탐색에서 제외할 끝 슬롯입니다.
         * @pre Start는 End 이하이어야 합니다.
         */
        TObjectIterator(Uint32 Start, Uint32 End);

        // 레퍼런스 리턴
        /**
         * @brief 현재 핸들이 가리키는 타입 호환 객체를 반환합니다.
         * @pre 반복자가 끝에 있지 않고 현재 객체가 살아 있어야 합니다. 위반하면 Critical 오류를 보고합니다.
         * @return 현재 객체의 참조입니다.
         */
        TObject& operator*() const;

        /**
         * @brief 현재 객체의 포인터를 반환합니다.
         * @pre 역참조 가능한 반복자이어야 합니다.
         * @return 현재 객체 포인터입니다.
         */
        TObject* operator->() const;

        /**
         * @brief 현재 슬롯 뒤에서 다음 타입 호환 객체를 찾습니다.
         * @return 갱신된 반복자의 참조입니다. 끝에 도달하면 끝 인덱스를 유지합니다.
         */
        TObjectIterator& operator++();

        /**
         * @brief 현재 슬롯 앞에서 이전 타입 호환 객체를 찾습니다.
         * @return 갱신된 반복자의 참조입니다.
         * @note 인덱스 0까지 역방향으로 검색하며 이전 객체가 없으면 Critical 오류를 보고합니다. 생성 시 Start를 하한으로 저장하지 않습니다.
         */
        TObjectIterator& operator--();

        /**
         * @brief 현재 인덱스가 끝 인덱스보다 작은지 검사합니다.
         * @return 끝에 도달하지 않았으면 true입니다.
         * @note 현재 객체의 생존 여부를 다시 검사하지 않습니다.
         */
        operator bool() const;

        // 후순위
        /**
         * @brief 현재 위치에서 Index개의 타입 호환 객체를 전진한 위치를 역참조합니다.
         * @param Index 현재 객체를 0으로 하는 상대 순회 위치입니다.
         * @pre 전진한 위치가 역참조 가능해야 합니다.
         * @return 해당 객체의 참조입니다. 현재 반복자는 변경하지 않습니다.
         */
        TObject& operator[](std::size_t Index);

        /**
         * @brief 반복자의 복사본을 전진해 상대 위치의 객체를 반환합니다.
         * @param Index 현재 객체를 0으로 하는 상대 순회 위치입니다.
         * @pre 전진한 위치가 역참조 가능해야 합니다.
         * @return 해당 객체의 참조입니다.
         */
        TObject& operator[](std::size_t Index) const;

        /**
         * @brief 현재 슬롯과 끝 인덱스가 같은지 비교합니다.
         * @param Other 비교할 반복자입니다.
         * @return 두 인덱스가 모두 같으면 true입니다. 핸들 세대는 비교하지 않습니다.
         */
        bool operator==(const TObjectIterator& Other) const;

        /**
         * @brief 반복자의 현재 슬롯 또는 끝 인덱스가 다른지 비교합니다.
         * @param Other 비교할 반복자입니다.
         * @return 같지 않으면 true입니다.
         */
        bool operator!=(const TObjectIterator& Other) const;

    private:
        /**
         * @brief 현재 슬롯부터 타입 호환 객체를 탐색합니다.
         * @return 객체를 찾으면 true입니다. 못 찾으면 끝 위치로 이동하고 false를 반환합니다.
         */
        bool AdvanceToNextValidObject();

        /**
         * @brief 현재 슬롯부터 비어 있지 않은 등록 슬롯을 찾습니다.
         * @return 핸들을 찾으면 true입니다. 끝이면 핸들을 비우고 false를 반환합니다.
         */
        bool Advance();

        /**
         * @brief 현재 슬롯에서 찾은 객체 핸들입니다.
         */
        FObjectHandle mHandle{};

        /**
         * @brief 현재 객체 슬롯 또는 끝 위치입니다.
         */
        Uint32 mCurrentIndex{};

        /**
         * @brief 순회에서 제외할 끝 슬롯입니다.
         */
        Uint32 mEndIndex{};
    };

    /**
     * @brief [0, End) 슬롯의 타입 호환 객체를 제공하는 순회 범위입니다.
     * @tparam TObject UObject 파생 리플렉션 타입입니다.
     */
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    class TObjectRange {
    public:
        /**
         * @brief 순회할 끝 슬롯을 저장합니다.
         * @param End 범위에서 제외할 끝 인덱스입니다.
         */
        TObjectRange(Uint32 End);

        /**
         * @brief 인덱스 0부터 첫 타입 호환 객체를 찾는 반복자를 반환합니다.
         * @return 시작 반복자입니다.
         */
        TObjectIterator<TObject> begin() const;

        /**
         * @brief 저장된 끝 인덱스를 가리키는 반복자를 반환합니다.
         * @return 끝 반복자입니다.
         */
        TObjectIterator<TObject> end() const;

    private:
        /**
         * @brief 범위를 만들 때 저장한 끝 슬롯입니다.
         */
        Uint32 mEnd{};
    };

    /**
     * @brief 현재 저장소 슬롯 수를 끝 인덱스로 하는 타입별 범위를 만듭니다.
     * @tparam TObject UObject 파생 리플렉션 타입입니다.
     * @return 현재 슬롯 범위입니다. 객체의 소유권을 갖는 스냅샷은 아닙니다.
     */
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    TObjectRange<TObject> Objects() {
        return TObjectRange<TObject>(UObjectSystem::GetItemCount());
    }

}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    TObjectIterator<TObject>::TObjectIterator(Uint32 Start, Uint32 End)
		: mCurrentIndex(Start),
		  mEndIndex(End) {
        if (Start > End) {
            ErrorHandler::Report("Invalid start!", "Start > End", ErrorHandler::EErrorLevel::Critical);
        }

        AdvanceToNextValidObject();
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    TObject& TObjectIterator<TObject>::operator*() const {
        UObject* Object{UObjectSystem::Resolve(mHandle)};

        if (mCurrentIndex == mEndIndex || Object == nullptr ||
            !Object->GetClass()->IsChildOf(TObject::StaticClass())) {
            ErrorHandler::Report("Refer Error!", "Does not refer to a live object", ErrorHandler::EErrorLevel::Critical);
        }

        return *static_cast<TObject*>(Object);
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    TObject* TObjectIterator<TObject>::operator->() const {
        return &operator*();
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    TObjectIterator<TObject>& TObjectIterator<TObject>::operator++() {
        if (mCurrentIndex < mEndIndex) {
            ++mCurrentIndex;
            AdvanceToNextValidObject();
        }

        return *this;
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    TObjectIterator<TObject>& TObjectIterator<TObject>::operator--() {
        for (Uint32 Index{mCurrentIndex}; Index > 0;) {
            --Index;

            FObjectHandle Handle{};

            if (UObjectSystem::TryGet(Index, Handle)) {
                UObject* Object{UObjectSystem::Resolve(Handle)};

                // 같은 타입인지
                if (Object != nullptr && Object->GetClass()->IsChildOf(TObject::StaticClass())) {
                    mCurrentIndex = Index;
                    mHandle = Handle;
                    return *this;
                }
            }
        }

        ErrorHandler::Report("Invalid decrement!", "No previous object", ErrorHandler::EErrorLevel::Critical);

        return *this;
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    TObjectIterator<TObject>::operator bool() const {
        return mCurrentIndex < mEndIndex;
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    TObject& TObjectIterator<TObject>::operator[](std::size_t Index) {
        TObjectIterator It{*this};

        for (std::size_t I{0}; I < Index; ++I) {
            ++It;
        }

        return *It;
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    TObject& TObjectIterator<TObject>::operator[](std::size_t Index) const {
        TObjectIterator It{*this};

        for (std::size_t I{0}; I < Index; ++I)
            ++It;

        return *It;
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    bool TObjectIterator<TObject>::operator==(const TObjectIterator& Other) const {
        return mCurrentIndex == Other.mCurrentIndex && mEndIndex == Other.mEndIndex;
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    bool TObjectIterator<TObject>::operator!=(const TObjectIterator& Other) const {
        return !(*this == Other);
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    bool TObjectIterator<TObject>::AdvanceToNextValidObject() {
        while (Advance()) {
            UObject* Object{UObjectSystem::Resolve(mHandle)};

            // 타입검사(완전히 같은 타입)
            if (Object != nullptr && Object->GetClass()->IsChildOf(TObject::StaticClass()))
                return true;

            ++mCurrentIndex; // 핵심: 타입이 다르거나 빈 슬롯이면 다음 인덱스로
        }

        mCurrentIndex = mEndIndex;

        return false;
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    bool TObjectIterator<TObject>::Advance() {
        while (mCurrentIndex < mEndIndex) {
            if (UObjectSystem::TryGet(mCurrentIndex, mHandle))
                return true; // 이 슬롯은 타입 검사를 해볼 수 있음

            ++mCurrentIndex; // 빈 슬롯이면 계속 탐색
        }

        mHandle = {}; // 초기화

        return false; // 범위의 끝
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    TObjectRange<TObject>::TObjectRange(Uint32 End)
		: mEnd(End) {
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    TObjectIterator<TObject> TObjectRange<TObject>::begin() const {
        return {0, mEnd};
    }
}

namespace UObjectSystem {
    template <typename TObject>
        requires std::derived_from<TObject, UObject>
    TObjectIterator<TObject> TObjectRange<TObject>::end() const {
        return {mEnd, mEnd};
    }
}
