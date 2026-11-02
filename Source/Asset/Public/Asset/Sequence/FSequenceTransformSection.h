/**
 * @file FSequenceTransformSection.h
 * @brief 시간 구간의 위치·회전·스케일 채널에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Sequence/FSequenceChannel.h"
#include "Core/Math/FTransform.h"

#include <array>

namespace Asset {

    /**
     * @brief 변환 섹션의 채널 순서를 구분합니다.
     * @ingroup Asset
     */
    enum class ESequenceTransformChannel : Core::Uint8 {
        /**
         * @brief 위치 X입니다.
         */
        LocationX,
        /**
         * @brief 위치 Y입니다.
         */
        LocationY,
        /**
         * @brief 위치 Z입니다.
         */
        LocationZ,
        /**
         * @brief 도 단위 피치입니다.
         */
        RotationPitch,
        /**
         * @brief 도 단위 요입니다.
         */
        RotationYaw,
        /**
         * @brief 도 단위 롤입니다.
         */
        RotationRoll,
        /**
         * @brief 스케일 X입니다.
         */
        ScaleX,
        /**
         * @brief 스케일 Y입니다.
         */
        ScaleY,
        /**
         * @brief 스케일 Z입니다.
         */
        ScaleZ
    };

    ReflectEnum(ESequenceTransformChannel, LocationX, LocationY, LocationZ, RotationPitch, RotationYaw, RotationRoll, ScaleX, ScaleY, ScaleZ);

    /**
     * @brief 시간 구간의 위치·회전·스케일 채널을 저장합니다.
     * @ingroup Asset
     */
    class FSequenceTransformSection {
    public:
        /**
         * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
         */
        ReflectTypes(FSequenceTransformSection);

    public:
        /**
         * @brief 섹션이 소유하는 채널의 개수입니다.
         */
        static constexpr std::size_t ChannelCount{9};

    public:
        /**
         * @brief 정적 구조체 메타데이터를 반환합니다.
         * @return 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSequenceTransformSection);

        /**
         * @brief 시간이 섹션 구간에 포함되는지 검사합니다.
         * @param[in] Tick 검사할 시간이며 틱 단위입니다.
         * @return 시작과 끝을 포함한 구간 안이면 true입니다.
         */
        bool Contains(double Tick) const;

        /**
         * @brief 지정한 시간이 포함되도록 섹션 구간을 넓힙니다.
         * @param[in] Tick 포함할 시간이며 틱 단위입니다.
         */
        void ExpandToInclude(Core::Int32 Tick);

        /**
         * @brief 지정한 채널을 반환합니다.
         * @param[in] Channel 조회할 채널의 순서이며 ChannelCount보다 작아야 합니다.
         * @return 섹션이 소유한 채널의 참조입니다.
         */
        Asset::FSequenceChannel& GetChannel(std::size_t Channel);

        /**
         * @brief 채널의 저장 이름을 반환합니다.
         * @param[in] Channel 조회할 채널의 순서이며 ChannelCount보다 작아야 합니다.
         * @return 정적 수명의 채널 이름입니다.
         */
        static const char* GetChannelName(std::size_t Channel);

        /**
         * @brief 지정한 시간의 변환을 계산합니다.
         * @param[in] Tick 계산할 시간이며 틱 단위입니다.
         * @param[in] Base 키가 없는 채널에 사용할 기준 변환입니다.
         * @return 키가 있는 채널만 기준 변환에 덮어쓴 결과입니다.
         */
        Core::FTransform Evaluate(double Tick, const Core::FTransform& Base) const;

        /**
         * @brief 아카이브 모드에 따라 구간과 키가 있는 채널을 읽거나 저장합니다.
         * @param[in,out] Archive 값을 읽거나 쓸 아카이브입니다.
         */
        void Serialize(Core::FArchive& Archive);

    private:
        /**
         * @brief 생성된 멤버 등록 함수의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 틱 단위 시작 시간입니다.
         */
        Core::Int32 mStartTick{};
        ReflectProperty(mStartTick, "StartTick", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 틱 단위 끝 시간이며 이 시간까지 포함합니다.
         */
        Core::Int32 mEndTick{};
        ReflectProperty(mEndTick, "EndTick", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief ESequenceTransformChannel 순서의 채널입니다.
         */
        std::array<Asset::FSequenceChannel, ChannelCount> mChannels{};
        ReflectProperty(mChannels, "Channels", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
