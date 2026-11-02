/**
 * @file TRevisionCache.h
 * @brief 입력 스탬프가 같을 때만 결과를 재사용하는 범용 캐시와 축출 정책을 정의합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Stats.h"
#include "Core/Types.h"

#include <optional>
#include <utility>

namespace Core {

    /**
     * @brief 오래 쓰지 않은 항목도 축출하지 않는 정책입니다.
     */
    struct FNoEviction {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FNoEviction);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FNoEviction);

        /**
         * @brief 항목을 축출할지 판단합니다.
         * @param[in] LastUsed 항목을 마지막으로 조회하거나 기록한 프레임입니다.
         * @param[in] Frame 현재 프레임입니다.
         * @return 항상 false입니다.
         */
        bool ShouldEvict(Core::Uint64 LastUsed, Core::Uint64 Frame) const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();
    };

    /**
     * @brief 지정한 프레임 수 동안 조회하지 않은 항목을 축출하는 정책입니다.
     */
    struct FFrameAgeEviction {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FFrameAgeEviction);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FFrameAgeEviction);

        /**
         * @brief 마지막 사용 이후 허용 프레임 수를 넘겼는지 판단합니다.
         * @param[in] LastUsed 항목을 마지막으로 조회하거나 기록한 프레임입니다.
         * @param[in] Frame 현재 프레임입니다.
         * @return 허용 프레임 수를 넘겼으면 true입니다.
         */
        bool ShouldEvict(Core::Uint64 LastUsed, Core::Uint64 Frame) const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 조회하지 않아도 유지할 최대 프레임 수입니다.
         */
        Core::Uint64 mMaxAge{};
        ReflectProperty(mMaxAge, "MaxAge", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    /**
     * @brief 모든 TRevisionCache에 적용하는 디버그 설정입니다.
     */
    struct FRevisionCacheSettings {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FRevisionCacheSettings);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FRevisionCacheSettings);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief true이면 모든 캐시 조회가 미스로 처리되어 매번 결과를 다시 만듭니다.
         * @details 리비전 갱신 누락으로 캐시가 옛 결과를 돌려주는지 비교할 때 사용합니다.
         */
        static inline bool sBypass{};
    };

    /**
     * @brief TRevisionCache 한 키의 스탬프와 성공 결과 또는 실패 원인입니다.
     * @tparam TStamp 결과가 의존하는 입력 묶음입니다.
     * @tparam TValue 보관할 결과입니다.
     */
    template <typename TStamp, typename TValue>
    struct TRevisionCacheEntry {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(TRevisionCacheEntry);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectTemplate(TRevisionCacheEntry);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 결과를 만들 때 사용한 입력 스탬프입니다.
         */
        TStamp mStamp{};
        ReflectProperty(mStamp, "Stamp", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 성공한 결과이며, 실패 항목은 비어 있습니다.
         */
        std::optional<TValue> mValue{};
        ReflectProperty(mValue, "Value", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 실패 항목의 원인입니다.
         */
        Core::FString mFailure{};
        ReflectProperty(mFailure, "Failure", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 항목을 마지막으로 조회하거나 기록한 캐시 프레임입니다.
         */
        Core::Uint64 mLastUsed{};
        ReflectProperty(mLastUsed, "LastUsed", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    /**
     * @brief 키별로 결과와 그 결과를 만들 때 본 입력 스탬프를 보관하고, 스탬프가 같을 때만 결과를 재사용하는 캐시입니다.
     * @details 데이터 원본은 리비전만 올리고, 소비자는 Find로 조회한 뒤 결과를 만드는 데 성공했을 때만 Commit합니다.
     * 실패도 스탬프와 함께 기록하므로 입력이 바뀔 때까지 같은 실패를 반복하지 않습니다.
     * Find가 반환한 주소는 다음 Commit·BeginFrame·Clear 전까지 유효합니다.
     * @tparam TKey 항목을 구분하는 키이며 operator==가 필요합니다.
     * @tparam TStamp 결과가 의존하는 입력 묶음이며 operator==로 전체 값을 비교합니다.
     * @tparam TValue 보관할 결과이며, 결과가 GPU 자원 안에 있으면 std::monostate를 사용합니다.
     * @tparam THash TKey의 해시 함수 객체입니다.
     * @tparam TEviction ShouldEvict(LastUsed, Frame)를 제공하는 축출 정책입니다.
     */
    template <typename TKey, typename TStamp, typename TValue, typename THash, typename TEviction>
    class TRevisionCache {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(TRevisionCache);

    private:
        /**
         * @brief 한 키의 스탬프와 성공 결과 또는 실패 원인입니다.
         */
        using FEntry = Core::TRevisionCacheEntry<TStamp, TValue>;

    public:
        /**
         * @brief 통계 이름과 축출 정책으로 빈 캐시를 만듭니다.
         * @param[in] StatName 적중·미스·실패 통계 항목 이름의 접두어입니다.
         * @param[in] Eviction 오래된 항목을 축출할 정책입니다.
         */
        TRevisionCache(const Core::FString& StatName, TEviction Eviction);

        /**
         * @brief 통계 항목을 소유하므로 복사를 금지합니다.
         */
        TRevisionCache(const TRevisionCache&) = delete;

        /**
         * @brief 통계 항목을 소유하므로 대입을 금지합니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        TRevisionCache& operator=(const TRevisionCache&) = delete;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectTemplate(TRevisionCache);

        /**
         * @brief 같은 스탬프로 성공한 결과를 조회합니다.
         * @param[in] Key 조회할 키입니다.
         * @param[in] Stamp 현재 입력 스탬프입니다.
         * @return 같은 스탬프의 성공 결과이면 그 주소, 없거나 스탬프가 다르거나 우회 중이면 nullptr입니다.
         */
        TValue* Find(const TKey& Key, const TStamp& Stamp);

        /**
         * @brief 같은 스탬프로 실패한 기록을 조회합니다.
         * @param[in] Key 조회할 키입니다.
         * @param[in] Stamp 현재 입력 스탬프입니다.
         * @return 같은 스탬프로 실패했으면 원인 문자열의 주소, 아니면 nullptr입니다.
         */
        const Core::FString* FindFailure(const TKey& Key, const TStamp& Stamp);

        /**
         * @brief 결과를 만드는 데 성공했을 때 스탬프와 함께 기록합니다.
         * @param[in] Key 기록할 키입니다.
         * @param[in] Stamp 결과를 만들 때 사용한 입력 스탬프입니다.
         * @param[in] Value 보관할 결과입니다.
         * @return 보관한 결과의 참조입니다.
         */
        TValue& Commit(const TKey& Key, const TStamp& Stamp, TValue Value);

        /**
         * @brief 결과를 만들지 못했을 때 스탬프와 원인을 기록합니다.
         * @param[in] Key 기록할 키입니다.
         * @param[in] Stamp 실패한 입력 스탬프입니다.
         * @param[in] Reason 실패 원인입니다.
         */
        void CommitFailure(const TKey& Key, const TStamp& Stamp, Core::FString Reason);

        /**
         * @brief 조건에 맞는 항목을 제거합니다.
         * @details 여러 키의 결과가 같은 GPU 영역을 공유할 때, 한 키를 다시 만들면서 덮어쓴 다른 키의 항목을 무효화하는 데 사용합니다.
         * @tparam TPredicate (const TKey&, const TStamp&)를 받아 제거하려면 true를 반환하는 함수 객체입니다.
         * @param[in] Predicate 제거할 항목을 고르는 조건입니다.
         */
        template <typename TPredicate>
        void InvalidateIf(TPredicate Predicate);

        /**
         * @brief 프레임을 진행하고 축출 정책에 해당하는 항목을 제거합니다.
         */
        void BeginFrame();

        /**
         * @brief 모든 항목을 제거합니다.
         */
        void Clear();

        /**
         * @brief 보관 중인 항목 수를 반환합니다.
         * @return 성공·실패 항목을 합한 개수입니다.
         */
        std::size_t GetCount() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 키별 스탬프와 결과입니다.
         */
        Core::TMap<TKey, FEntry, THash> mEntries{};
        ReflectProperty(mEntries, "Entries", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 오래된 항목을 축출할 정책입니다.
         */
        TEviction mEviction{};
        ReflectProperty(mEviction, "Eviction", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief BeginFrame마다 증가하는 캐시 프레임 번호입니다.
         */
        Core::Uint64 mFrame{};
        ReflectProperty(mFrame, "Frame", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 같은 스탬프의 결과를 재사용한 조회 수입니다.
         */
        Core::FStat mHits;
        ReflectProperty(mHits, "Hits", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 결과가 없거나 스탬프가 달라 다시 만들어야 한 조회 수입니다.
         */
        Core::FStat mMisses;
        ReflectProperty(mMisses, "Misses", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 같은 스탬프의 실패 기록을 재사용한 조회 수입니다.
         */
        Core::FStat mFailures;
        ReflectProperty(mFailures, "Failures", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    inline bool Core::FNoEviction::ShouldEvict(Core::Uint64, Core::Uint64) const {
        return false;
    }

    inline bool Core::FFrameAgeEviction::ShouldEvict(Core::Uint64 LastUsed, Core::Uint64 Frame) const {
        return Frame > LastUsed + mMaxAge;
    }

    template <typename TKey, typename TStamp, typename TValue, typename THash, typename TEviction>
    Core::TRevisionCache<TKey, TStamp, TValue, THash, TEviction>::TRevisionCache(const Core::FString& StatName, TEviction Eviction)
		: mEviction{std::move(Eviction)},
		  mHits{"cache", StatName + " hits"},
		  mMisses{"cache", StatName + " misses"},
		  mFailures{"cache", StatName + " failures"} {
    }

    template <typename TKey, typename TStamp, typename TValue, typename THash, typename TEviction>
    TValue* Core::TRevisionCache<TKey, TStamp, TValue, THash, TEviction>::Find(const TKey& Key, const TStamp& Stamp) {
        const auto Position{mEntries.find(Key)};

        if (Core::FRevisionCacheSettings::sBypass || Position == mEntries.end() || !(Position->second.mStamp == Stamp) || !Position->second.mValue.has_value()) {
            mMisses.Add();
            return nullptr;
        }

        Position->second.mLastUsed = mFrame;
        mHits.Add();

        return &*Position->second.mValue;
    }

    template <typename TKey, typename TStamp, typename TValue, typename THash, typename TEviction>
    const Core::FString* Core::TRevisionCache<TKey, TStamp, TValue, THash, TEviction>::FindFailure(const TKey& Key, const TStamp& Stamp) {
        const auto Position{mEntries.find(Key)};

        if (Core::FRevisionCacheSettings::sBypass || Position == mEntries.end() || !(Position->second.mStamp == Stamp) || Position->second.mValue.has_value()) {
            return nullptr;
        }

        Position->second.mLastUsed = mFrame;
        mFailures.Add();

        return &Position->second.mFailure;
    }

    template <typename TKey, typename TStamp, typename TValue, typename THash, typename TEviction>
    TValue& Core::TRevisionCache<TKey, TStamp, TValue, THash, TEviction>::Commit(const TKey& Key, const TStamp& Stamp, TValue Value) {
        FEntry& Entry{mEntries[Key]};
        Entry.mStamp = Stamp;
        Entry.mValue = std::move(Value);
        Entry.mFailure.clear();
        Entry.mLastUsed = mFrame;

        return *Entry.mValue;
    }

    template <typename TKey, typename TStamp, typename TValue, typename THash, typename TEviction>
    void Core::TRevisionCache<TKey, TStamp, TValue, THash, TEviction>::CommitFailure(const TKey& Key, const TStamp& Stamp, Core::FString Reason) {
        FEntry& Entry{mEntries[Key]};
        Entry.mStamp = Stamp;
        Entry.mValue.reset();
        Entry.mFailure = std::move(Reason);
        Entry.mLastUsed = mFrame;
    }

    template <typename TKey, typename TStamp, typename TValue, typename THash, typename TEviction>
    template <typename TPredicate>
    void Core::TRevisionCache<TKey, TStamp, TValue, THash, TEviction>::InvalidateIf(TPredicate Predicate) {
        std::erase_if(mEntries, [&Predicate](const auto& Entry) {
            return Predicate(Entry.first, Entry.second.mStamp);
        });
    }

    template <typename TKey, typename TStamp, typename TValue, typename THash, typename TEviction>
    void Core::TRevisionCache<TKey, TStamp, TValue, THash, TEviction>::BeginFrame() {
        ++mFrame;
        std::erase_if(mEntries, [this](const auto& Entry) {
            return mEviction.ShouldEvict(Entry.second.mLastUsed, mFrame);
        });
    }

    template <typename TKey, typename TStamp, typename TValue, typename THash, typename TEviction>
    void Core::TRevisionCache<TKey, TStamp, TValue, THash, TEviction>::Clear() {
        mEntries.clear();
    }

    template <typename TKey, typename TStamp, typename TValue, typename THash, typename TEviction>
    std::size_t Core::TRevisionCache<TKey, TStamp, TValue, THash, TEviction>::GetCount() const {
        return mEntries.size();
    }

}

#include "TRevisionCacheEntryReflection.generated.h"
#include "TRevisionCacheReflection.generated.h"
