/**
 * @file Stat.h
 * @brief 프레임, 시스템 단계, 메모리, 객체, 월드 틱, 피킹 및 LOD 통계와 스코프 타이머를 정의한다.
 */
#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

/**
 * @brief 전역 통계 수집 상태와 조회용 스냅샷 및 구간 평균을 관리한다.
 * @details 프레임 측정 결과는 EndFrame() 후 다음 BeginFrame()에서 공개한다. 평균은 첫 표본을 공개한 뒤 대략 0.5초 이상의 프레임 집계 구간마다 갱신한다.
 * @note 상태 접근에 내부 스레드 동기화를 제공하지 않는다.
 */
namespace Stat {
    /**
     * @brief 엔진 메모리의 할당 용도를 구분하는 태그.
     */
    enum class EMemoryTag {
        /**
         * @brief 용도가 지정되지 않은 메모리 할당.
         */
        Unknown,
        /**
         * @brief UObject 계열 객체 저장 공간의 할당.
         */
        UObject,
        /**
         * @brief 엔진 컨테이너의 저장 공간 할당.
         */
        Container,
        /**
         * @brief 엔진 문자열의 저장 공간 할당.
         */
        String,
        /**
         * @brief 메시지 페이로드의 힙 저장 공간 할당.
         */
        Message,
        /**
         * @brief 메모리 태그 개수와 태그별 배열 크기를 나타내는 값. 실제 할당 용도로 사용하지 않는다.
         */
        Count
    };

    // 태그별 메모리 통계
    /**
     * @brief 특정 메모리 태그의 현재 요청 바이트 수와 활성 할당 수.
     */
    struct FTagStats {
        /**
         * @brief 현재 해제되지 않은 할당의 요청 바이트 수 합계. 내부 할당 헤더와 정렬 여유는 제외한다.
         */
        std::size_t mAllocatedBytes{0};
        /**
         * @brief 아직 해제되지 않은 메모리 할당 수.
         */
        std::size_t mActiveAllocationCount{0};
    };

    /**
     * @brief 전체 및 태그별 메모리의 현재 상태, 최대 요청 바이트 수 및 누적 할당 계수.
     */
    struct FMemoryStats {
        /**
         * @brief 현재 해제되지 않은 할당의 요청 바이트 수 합계. 내부 할당 헤더와 정렬 여유는 제외한다.
         */
        std::size_t mAllocatedBytes{0};
        /**
         * @brief 통계 수집 이후 현재 요청 바이트 수 합계의 최댓값.
         */
        std::size_t mPeakAllocatedBytes{0};
        /**
         * @brief 아직 해제되지 않은 메모리 할당 수.
         */
        std::size_t mActiveAllocationCount{0};
        /**
         * @brief 통계 수집 이후 기록한 할당의 누적 횟수.
         */
        std::size_t mTotalAllocationCount{0};
        /**
         * @brief 통계 수집 이후 기록한 해제의 누적 횟수.
         */
        std::size_t mTotalDeallocationCount{0};

        // EMemoryTag::Count 크기만큼의 태그별 통계 배열
        /**
         * @brief EMemoryTag 값으로 접근하는 태그별 현재 할당 통계.
         */
        FTagStats mTagStats[static_cast<std::size_t>(EMemoryTag::Count)]{};
    };

    /**
     * @brief 프레임 실행 시간을 분류할 시스템 단계.
     */
    enum class ESystemStatStage : std::size_t {
        /**
         * @brief 프레임 준비와 펜스 대기 단계.
         */
        FrameSetup,
        /**
         * @brief 미리보기 렌더링 단계.
         */
        PreviewRender,
        /**
         * @brief 에디터 사용자 인터페이스 구성 단계.
         */
        EditorUi,
        /**
         * @brief 입력 처리와 월드 갱신 단계.
         */
        WorldUpdate,
        /**
         * @brief 장면 및 렌더링 데이터 준비 단계.
         */
        RenderPreparation,
        /**
         * @brief 장면 지오메트리 렌더링 단계.
         */
        Geometry,
        /**
         * @brief 에디터 오버레이 렌더링 단계.
         */
        EditorOverlays,
        /**
         * @brief 사용자 인터페이스 렌더링 단계.
         */
        UiRender,
        /**
         * @brief 화면 제출과 관련 대기 단계.
         */
        Present,
        /**
         * @brief 별도 단계 밖의 작업과 프레임 사이의 시간을 기록하는 분류.
         */
        Other,
        /**
         * @brief 시스템 측정 단계의 개수와 표본 배열 크기를 나타내는 값.
         */
        Count
    };

    /**
     * @brief 렌더 준비 단계 안에서 측정할 세부 작업 분류.
     */
    enum class ERenderPreparationStage : std::size_t {
        /**
         * @brief 장면 데이터 수집 단계.
         */
        SceneData,
        /**
         * @brief 장면 상태 동기화 단계.
         */
        SceneSynchronization,
        /**
         * @brief 뷰 데이터 설정 단계.
         */
        ViewSetup,
        /**
         * @brief 머티리얼 버퍼 반영 단계.
         */
        MaterialBuffer,
        /**
         * @brief 렌더 큐 구성 단계.
         */
        RenderQueue,
        /**
         * @brief 뷰 버퍼 준비 단계.
         */
        ViewBuffers,
        /**
         * @brief 렌더 준비 세부 단계의 개수와 표본 배열 크기를 나타내는 값.
         */
        Count
    };

    /**
     * @brief 한 프레임의 단계별 총 시간, 독점 시간 및 호출 횟수.
     * @details 시스템 스코프의 총 시간은 중첩 시간을 포함하고 독점 시간은 활성 하위 단계 시간을 제외한다. 렌더 준비 세부 타이머는 총 시간과 독점 시간에 같은 측정값을 기록한다.
     */
    struct FSystemStatSample {
        /**
         * @brief 해당 프레임에 기록한 총 측정 시간. 단위는 밀리초이다.
         */
        double mTotalMilliseconds{};
        /**
         * @brief 해당 프레임에 기록한 단계의 독점 시간. 단위는 밀리초이다.
         */
        double mExclusiveMilliseconds{};
        /**
         * @brief 해당 프레임에 이 단계의 총 시간 표본을 기록한 횟수.
         */
        std::uint64_t mCallCount{};
    };

    /**
     * @brief 프레임 시작 간격, 누적 시간 및 최근 집계 구간의 FPS와 평균 프레임 시간.
     */
    struct FFrameStats {
        /**
         * @brief 직전 공개 프레임의 시작부터 다음 프레임 시작까지의 간격. 단위는 초이다.
         */
        double mDeltaSeconds{};
        /**
         * @brief 초기화 이후 공개한 프레임 시작 간격의 누적 합. 단위는 초이다.
         */
        double mElapsedSeconds{};
        /**
         * @brief 최근 집계 구간의 프레임 수를 경과 시간으로 나눈 FPS.
         */
        double mFramesPerSecond{};
        /**
         * @brief 최근 집계 구간의 평균 프레임 시작 간격. 단위는 밀리초이다.
         */
        double mAverageFrameMilliseconds{};
        /**
         * @brief 초기화 이후 공개한 프레임의 누적 개수.
         */
        std::uint64_t mFrameCount{};
    };

    /**
     * @brief 최근 공개한 프레임의 시스템 단계별 표본과 공개 프레임 수.
     */
    struct FSystemStats {
        /**
         * @brief ESystemStatStage 값으로 접근할 시스템 단계별 표본.
         */
        std::array<FSystemStatSample, static_cast<std::size_t>(ESystemStatStage::Count)> mSamples{};

        /**
         * @brief 시스템 측정 표본에 반영한 프레임의 누적 개수.
         */
        std::uint64_t mFrameCount{};
    };

    /**
     * @brief 가장 최근에 기록한 전체 객체 수와 액터 수.
     */
    struct FObjectStats {
        /**
         * @brief 가장 최근에 기록한 전체 객체 수.
         */
        std::size_t mObjectCount{};
        /**
         * @brief 가장 최근에 기록한 액터 수.
         */
        std::size_t mActorCount{};
    };

    /**
     * @brief 한 프레임의 월드 틱 시간과 액터 대상 수 및 컴포넌트 순회 계수.
     */
    struct FWorldTickStats {
        /**
         * @brief 해당 프레임의 월드 틱 측정 시간 합계. 단위는 밀리초이다.
         */
        double mTotalMilliseconds{};
        /**
         * @brief 월드 틱 타이머에 전달하여 합산한 액터 틱 대상 수.
         */
        std::uint64_t mActorTickCount{};
        /**
         * @brief 월드 틱 중 컴포넌트를 방문한 횟수.
         */
        std::uint64_t mComponentVisitCount{};
        /**
         * @brief 월드 틱 중 컴포넌트 틱을 실행한 횟수.
         */
        std::uint64_t mComponentTickCount{};
    };

    /**
     * @brief 마지막 피킹 시도 시간과 초기화 이후 누적 시간 및 시도 횟수.
     */
    struct FPickingStats {
        /**
         * @brief 마지막 유효한 피킹 시간 표본. 단위는 밀리초이다.
         */
        double mLastMilliseconds{};
        /**
         * @brief 초기화 이후 피킹 시간의 누적 합. 단위는 밀리초이다.
         */
        double mTotalMilliseconds{};
        /**
         * @brief 초기화 이후 기록한 유효한 피킹 시도 횟수.
         */
        std::uint64_t mAttemptCount{};
    };

    /**
     * @brief 현재 프레임에 기록한 최대 LOD 레벨과 삼각형 및 드로 호출 수 합계.
     */
    struct FLODStats {
        /**
         * @brief 현재 프레임에 기록한 LOD 레벨의 최댓값.
         */
        std::uint32_t mLevel{};
        /**
         * @brief 현재 프레임에 기록한 렌더링 삼각형 수 합계.
         */
        std::uint64_t mRenderedTriangleCount{};
        /**
         * @brief 현재 프레임에 대응하는 원본 삼각형 수 합계.
         */
        std::uint64_t mOriginalTriangleCount{};
        /**
         * @brief 현재 프레임에 기록한 드로 호출 수 합계.
         */
        std::uint64_t mDrawCallCount{};
    };

    /**
     * @brief 조회 시점의 프레임, 단계, 메모리, 객체, 월드 틱, 피킹 및 LOD 통계 사본.
     * @details 시스템 및 월드 틱은 공개된 프레임 결과이고 메모리, 객체, 피킹 및 LOD는 각 기록 함수가 갱신한 상태이다.
     */
    struct FStats {
        /**
         * @brief 최근 공개된 프레임 간격과 누적 프레임 수 및 구간 FPS 정보.
         */
        FFrameStats mFrame{};
        /**
         * @brief 최근 공개된 시스템 단계별 프레임 측정 결과.
         */
        FSystemStats mSystem{};

        /**
         * @brief 최근 공개된 렌더 준비 세부 단계별 측정 결과.
         */
        std::array<FSystemStatSample, static_cast<std::size_t>(ERenderPreparationStage::Count)> mRenderPreparationSamples{};

        /**
         * @brief 현재 메모리 할당 상태와 최대 및 누적 계수.
         */
        FMemoryStats mMemory{};
        /**
         * @brief 가장 최근에 기록한 객체와 액터 수.
         */
        FObjectStats mObjects{};
        /**
         * @brief 최근 공개된 프레임의 월드 틱 측정 결과.
         */
        FWorldTickStats mWorldTick{};
        /**
         * @brief 현재 피킹 시간과 누적 시도 통계.
         */
        FPickingStats mPicking{};
        /**
         * @brief 현재 프레임에 수집한 LOD 통계.
         */
        FLODStats mLOD{};
    };

    /**
     * @brief 집계 구간의 프레임당 평균 단계 시간과 평균 호출 횟수.
     */
    struct FSystemStatAverage {
        /**
         * @brief 집계 구간의 프레임당 평균 총 측정 시간. 단위는 밀리초이다.
         */
        double mTotalMilliseconds{};
        /**
         * @brief 집계 구간의 프레임당 평균 독점 시간. 단위는 밀리초이다.
         */
        double mExclusiveMilliseconds{};
        /**
         * @brief 집계 구간의 프레임당 평균 호출 횟수.
         */
        double mCallCount{};
    };

    /**
     * @brief 집계 구간의 프레임별 태그 메모리 스냅샷 평균.
     */
    struct FTagStatAverage {
        /**
         * @brief 집계 구간의 프레임별 요청 바이트 수 합계의 산술평균.
         */
        double mAllocatedBytes{};
        /**
         * @brief 집계 구간의 프레임별 활성 할당 수의 산술평균.
         */
        double mActiveAllocationCount{};
    };

    /**
     * @brief 프레임별 메모리 상태의 평균과 집계 시점의 최대 및 누적 할당 계수.
     */
    struct FMemoryStatAverage {
        /**
         * @brief 집계 구간의 프레임별 요청 바이트 수 합계의 산술평균.
         */
        double mAllocatedBytes{};
        /**
         * @brief 집계 구간의 프레임별 활성 할당 수의 산술평균.
         */
        double mActiveAllocationCount{};
        /**
         * @brief 집계 시점까지의 최대 요청 바이트 수 합계. 평균값이 아니다.
         */
        std::size_t mPeakAllocatedBytes{};
        /**
         * @brief 집계 시점까지의 누적 할당 횟수.
         */
        std::size_t mTotalAllocationCount{};
        /**
         * @brief 집계 시점까지의 누적 해제 횟수.
         */
        std::size_t mTotalDeallocationCount{};

        /**
         * @brief 태그별 프레임 스냅샷 평균.
         */
        std::array<FTagStatAverage, static_cast<std::size_t>(EMemoryTag::Count)> mTagStats{};
    };

    /**
     * @brief 집계 구간의 프레임별 객체 및 액터 수 평균.
     */
    struct FObjectStatAverage {
        /**
         * @brief 집계 구간의 프레임당 평균 객체 수.
         */
        double mObjectCount{};
        /**
         * @brief 집계 구간의 프레임당 평균 액터 수.
         */
        double mActorCount{};
    };

    /**
     * @brief 집계 구간의 프레임당 평균 월드 틱 시간과 순회 계수.
     */
    struct FWorldTickStatAverage {
        /**
         * @brief 집계 구간의 프레임당 평균 월드 틱 시간. 단위는 밀리초이다.
         */
        double mTotalMilliseconds{};
        /**
         * @brief 집계 구간의 프레임당 평균 액터 틱 대상 수.
         */
        double mActorTickCount{};
        /**
         * @brief 집계 구간의 프레임당 평균 컴포넌트 방문 횟수.
         */
        double mComponentVisitCount{};
        /**
         * @brief 집계 구간의 프레임당 평균 컴포넌트 틱 횟수.
         */
        double mComponentTickCount{};
    };

    /**
     * @brief 집계 구간의 피킹 시도당 및 프레임당 비용과 빈도.
     */
    struct FPickingStatAverage {
        /**
         * @brief 집계 구간의 피킹 시도당 평균 시간. 시도가 없으면 0이다.
         */
        double mAverageMilliseconds{};
        /**
         * @brief 집계 구간의 프레임당 피킹 시간.
         */
        double mMillisecondsPerFrame{};
        /**
         * @brief 집계 구간의 프레임당 피킹 시도 횟수.
         */
        double mAttemptsPerFrame{};
    };

    /**
     * @brief 집계 구간의 평균 삼각형 및 드로 호출 수와 마지막 프레임의 LOD 레벨.
     */
    struct FLODStatAverage {
        /**
         * @brief 평균 집계 구간의 마지막 프레임에 기록한 최대 LOD 레벨.
         */
        std::uint32_t mLevel{};
        /**
         * @brief 집계 구간의 프레임당 평균 렌더링 삼각형 수.
         */
        double mRenderedTriangleCount{};
        /**
         * @brief 집계 구간의 프레임당 평균 원본 삼각형 수.
         */
        double mOriginalTriangleCount{};
        /**
         * @brief 집계 구간의 프레임당 평균 드로 호출 수.
         */
        double mDrawCallCount{};
    };

    /**
     * @brief 마지막으로 공개한 집계 구간의 통계 평균과 집계 프레임 수.
     */
    struct FStatAverages {
        /**
         * @brief 최근 공개된 프레임 간격과 누적 프레임 수 및 구간 FPS 정보.
         */
        FFrameStats mFrame{};

        /**
         * @brief 시스템 단계별 프레임당 평균 표본.
         */
        std::array<FSystemStatAverage, static_cast<std::size_t>(ESystemStatStage::Count)> mSystemSamples{};
        /**
         * @brief 렌더 준비 세부 단계별 프레임당 평균 표본.
         */
        std::array<FSystemStatAverage, static_cast<std::size_t>(ERenderPreparationStage::Count)> mRenderPreparationSamples{};

        /**
         * @brief 메모리 상태 평균과 집계 시점의 누적 계수.
         */
        FMemoryStatAverage mMemory{};
        /**
         * @brief 프레임당 평균 객체와 액터 수.
         */
        FObjectStatAverage mObjects{};
        /**
         * @brief 프레임당 평균 월드 틱 비용과 순회 계수.
         */
        FWorldTickStatAverage mWorldTick{};
        /**
         * @brief 피킹 시도당 및 프레임당 평균 비용과 빈도.
         */
        FPickingStatAverage mPicking{};
        /**
         * @brief 프레임당 평균 LOD 처리량과 마지막 프레임의 LOD 레벨.
         */
        FLODStatAverage mLOD{};
        /**
         * @brief 현재 공개된 평균을 계산하는 데 사용한 집계 구간의 프레임 개수.
         */
        std::uint64_t mFrameCount{};
    };

    /**
     * @brief 이전에 종료한 프레임의 결과를 공개하고 새 프레임의 측정을 시작한다.
     * @details 이전 프레임의 시작 간격과 단계별 표본을 공개하며 필요하면 구간 평균을 갱신한다. 새 단계 표본과 월드 틱 및 LOD 수집값을 초기화한다. 이미 프레임이 활성 상태이면 아무 작업도 하지 않는다.
     */
    void BeginFrame();
    /**
     * @brief 현재 프레임의 마지막 독점 시간 구간을 기록하고 측정을 종료한다.
     * @details 결과는 다음 BeginFrame()에서 공개한다. 활성 프레임이 없으면 아무 작업도 하지 않는다.
     */
    void EndFrame();
    /**
     * @brief 프레임, 단계, 월드 틱, 피킹, LOD와 구간 평균 상태를 초기화한다.
     * @details 활성 프레임을 종료하고 프레임 식별자를 변경한다. 현재 메모리 계수와 객체 수는 유지한다.
     */
    void ResetFrameStats();

    /**
     * @brief 활성 프레임의 단계 총 시간과 호출 횟수에 외부 측정값을 추가한다.
     * @param Stage 기록할 유효한 시스템 단계.
     * @param Milliseconds 추가할 유한한 비음수 시간. 단위는 밀리초이다.
     * @details 활성 프레임이 없거나 단계 및 시간이 유효하지 않으면 기록하지 않는다. 독점 시간은 변경하지 않는다.
     */
    void RecordSystemTime(ESystemStatStage Stage, double Milliseconds);
    /**
     * @brief 전체 및 유효한 태그의 할당 통계를 증가시킨다.
     * @param Size 요청한 사용 영역의 바이트 수.
     * @param Tag 할당 용도의 통계 태그.
     * @details 현재 바이트 수, 활성 및 누적 할당 수를 증가시키고 최대 바이트 수를 갱신한다. 범위 밖의 태그는 전체 통계만 갱신한다. 실제 메모리 할당은 수행하지 않는다.
     */
    void RecordAllocation(std::size_t Size, EMemoryTag Tag);
    /**
     * @brief 전체 및 유효한 태그의 해제 통계를 반영한다.
     * @param Size 해제한 사용 영역의 요청 바이트 수.
     * @param Tag 해제할 때 적용할 기존 할당 태그.
     * @details 현재 바이트 수와 활성 할당 수를 감소시키고 누적 해제 수를 증가시킨다. 실제 메모리 해제는 수행하지 않는다.
     * @pre 기존에 기록한 활성 할당의 크기와 태그에 맞게 호출해야 한다.
     */
    void RecordDeallocation(std::size_t Size, EMemoryTag Tag);
    /**
     * @brief 현재 객체 및 액터 수를 최신 값으로 교체한다.
     * @param ObjectCount 전체 객체 수.
     * @param ActorCount 액터 수.
     */
    void RecordObjectCounts(std::size_t ObjectCount, std::size_t ActorCount);
    /**
     * @brief 피킹 시도 시간과 누적 및 구간 통계를 기록한다.
     * @param Milliseconds 유한한 비음수 피킹 시간. 단위는 밀리초이다.
     * @details 마지막 시간, 누적 시간, 시도 횟수 및 평균 집계용 시간과 횟수를 갱신한다. 유효하지 않은 시간은 무시하며 프레임 활성 여부와 관계없이 기록한다.
     */
    void RecordPickingTime(double Milliseconds);
    /**
     * @brief 현재 LOD 통계에 렌더링 처리량을 합산한다.
     * @param Level 기록할 LOD 레벨. 현재 값과 비교하여 최댓값을 보관한다.
     * @param RenderedTriangleCount 추가할 렌더링 삼각형 수.
     * @param OriginalTriangleCount 추가할 원본 삼각형 수.
     * @param DrawCallCount 추가할 드로 호출 수.
     */
    void RecordLODStats(std::uint32_t Level, std::uint64_t RenderedTriangleCount, std::uint64_t OriginalTriangleCount, std::uint64_t DrawCallCount);

    /**
     * @brief 현재 공개 및 기록된 통계의 사본을 조회한다.
     * @return 조회 시점의 FStats 값.
     */
    FStats GetStats();
    /**
     * @brief 마지막으로 공개한 집계 구간의 평균 통계를 조회한다.
     * @return 마지막 공개 구간의 평균 사본. 아직 평균이 공개되지 않았으면 기본 초기화된 값.
     */
    FStatAverages GetStatAverages();
    /**
     * @brief 최근 공개된 프레임 간격과 FPS 정보를 조회한다.
     * @return 프레임 통계의 사본.
     */
    FFrameStats GetFrameStats();
    /**
     * @brief 최근 공개된 시스템 단계별 프레임 표본을 조회한다.
     * @return 시스템 통계의 사본.
     */
    FSystemStats GetSystemStats();
    /**
     * @brief 최근 공개된 특정 시스템 단계의 표본을 조회한다.
     * @param Stage 조회할 시스템 단계.
     * @return 해당 표본의 사본. 범위 밖의 단계이면 기본 초기화된 표본.
     */
    FSystemStatSample GetSystemSample(ESystemStatStage Stage);
    /**
     * @brief 최근 공개된 특정 렌더 준비 세부 단계의 표본을 조회한다.
     * @param Stage 조회할 렌더 준비 단계.
     * @return 해당 표본의 사본. 범위 밖의 단계이면 기본 초기화된 표본.
     */
    FSystemStatSample GetRenderPreparationSample(ERenderPreparationStage Stage);
    /**
     * @brief 현재 전체 및 태그별 메모리 통계를 조회한다.
     * @return 현재 메모리 통계의 사본.
     */
    FMemoryStats GetMemoryStats();
    /**
     * @brief 가장 최근에 기록한 객체 및 액터 수를 조회한다.
     * @return 객체 통계의 사본.
     */
    FObjectStats GetObjectStats();
    /**
     * @brief 최근 공개된 프레임의 월드 틱 통계를 조회한다.
     * @return 월드 틱 통계의 사본.
     */
    FWorldTickStats GetWorldTickStats();
    /**
     * @brief 현재 월드 틱 스코프의 집계용 통계를 조회한다.
     * @return 활성 프레임이 WorldUpdate 단계이고 월드 틱 타이머가 있으면 타이머가 소유하는 통계 포인터, 그 외에는 nullptr.
     * @note 반환된 포인터는 해당 월드 틱 타이머의 수명 안에서만 사용한다.
     */
    FWorldTickStats* GetActiveWorldTickStats();
    /**
     * @brief 현재 마지막 및 누적 피킹 통계를 조회한다.
     * @return 피킹 통계의 사본.
     */
    FPickingStats GetPickingStats();
    /**
     * @brief 현재 프레임에 수집한 LOD 통계를 조회한다.
     * @return 현재 LOD 통계의 사본.
     */
    FLODStats GetLODStats();

    /**
     * @brief 시스템 단계의 표시 이름을 조회한다.
     * @param Stage 이름을 조회할 단계.
     * @return 정적 표시 문자열. 유효하지 않은 값이면 Unknown 문자열.
     */
    const char* GetSystemStageName(ESystemStatStage Stage);
    /**
     * @brief 렌더 준비 세부 단계의 표시 이름을 조회한다.
     * @param Stage 이름을 조회할 단계.
     * @return 정적 표시 문자열. 유효하지 않은 값이면 Unknown 문자열.
     */
    const char* GetRenderPreparationStageName(ERenderPreparationStage Stage);
    // 태그 이름을 문자열로 반환하는 헬퍼 함수
    /**
     * @brief 메모리 태그의 표시 이름을 조회한다.
     * @param Tag 이름을 조회할 태그.
     * @return 정적 태그 이름. Count를 포함한 유효하지 않은 값이면 Invalid 문자열.
     */
    const char* GetMemoryTagName(EMemoryTag Tag);

    /**
     * @brief 시스템 단계의 총 시간과 중첩 단계를 제외한 독점 시간을 수집하는 스코프 타이머.
     * @details 활성 프레임의 유효한 단계에서만 측정하며 PreviewRender 안의 중첩 시스템 타이머는 활성화하지 않는다. 같은 프레임이 유지된 경우에만 소멸 시 결과를 기록한다.
     */
    class FScopedSystemStatTimer {
    public:
        /**
         * @brief 현재 단계를 보관하고 지정한 시스템 단계의 측정을 시작한다.
         * @param Stage 측정할 시스템 단계.
         */
        explicit FScopedSystemStatTimer(ESystemStatStage Stage);
        /**
         * @brief 활성화된 같은 프레임의 독점 시간을 기록하고 이전 단계를 복원하며 총 시간과 호출 횟수를 추가한다.
         */
        ~FScopedSystemStatTimer();
        /**
         * @brief 측정 및 결과 기록 책임을 가진 타이머의 복사 생성을 금지한다.
         */
        FScopedSystemStatTimer(const FScopedSystemStatTimer&) = delete;
        /**
         * @brief 스코프 타이머의 복사 할당을 금지한다.
         */
        FScopedSystemStatTimer& operator=(const FScopedSystemStatTimer&) = delete;
        /**
         * @brief 스코프 타이머의 이동 생성을 금지한다.
         */
        FScopedSystemStatTimer(FScopedSystemStatTimer&&) = delete;
        /**
         * @brief 스코프 타이머의 이동 할당을 금지한다.
         */
        FScopedSystemStatTimer& operator=(FScopedSystemStatTimer&&) = delete;

    private:
        /**
         * @brief 현재 스코프에서 측정할 시스템 단계.
         */
        ESystemStatStage mStage{};
        /**
         * @brief 측정이 끝나면 복원할 이전 활성 시스템 단계.
         */
        ESystemStatStage mPreviousStage{};
        /**
         * @brief 활성 스코프의 측정 시작 시각.
         */
        std::chrono::steady_clock::time_point mStartTime{};
        /**
         * @brief 생성 시의 활성 프레임 식별자. 다른 프레임으로 넘어간 결과를 기록하지 않는 데 사용한다.
         */
        std::uint64_t mFrameId{};
        /**
         * @brief 해당 스코프가 실제 측정과 결과 기록을 수행하는지 나타내는 상태.
         */
        bool mActive{};
    };

    /**
     * @brief 가장 바깥 피킹 스코프의 경과 시간을 측정하여 피킹 통계에 기록한다. 중첩 피킹 타이머는 비활성 상태로 유지한다.
     */
    class FScopedPickingStatTimer {
    public:
        /**
         * @brief 다른 피킹 타이머가 활성화되지 않았으면 측정을 시작한다.
         */
        FScopedPickingStatTimer();
        /**
         * @brief 활성 피킹 스코프의 경과 시간을 기록하고 피킹 타이머 활성 상태를 해제한다.
         */
        ~FScopedPickingStatTimer();
        /**
         * @brief 측정 및 결과 기록 책임을 가진 타이머의 복사 생성을 금지한다.
         */
        FScopedPickingStatTimer(const FScopedPickingStatTimer&) = delete;
        /**
         * @brief 스코프 타이머의 복사 할당을 금지한다.
         */
        FScopedPickingStatTimer& operator=(const FScopedPickingStatTimer&) = delete;
        /**
         * @brief 스코프 타이머의 이동 생성을 금지한다.
         */
        FScopedPickingStatTimer(FScopedPickingStatTimer&&) = delete;
        /**
         * @brief 스코프 타이머의 이동 할당을 금지한다.
         */
        FScopedPickingStatTimer& operator=(FScopedPickingStatTimer&&) = delete;

    private:
        /**
         * @brief 활성 스코프의 측정 시작 시각.
         */
        std::chrono::steady_clock::time_point mStartTime{};
        /**
         * @brief 해당 스코프가 실제 측정과 결과 기록을 수행하는지 나타내는 상태.
         */
        bool mActive{};
    };

    /**
     * @brief WorldUpdate 단계의 월드 틱 시간과 액터 및 컴포넌트 계수를 집계하는 스코프 타이머.
     * @details 중첩 월드 틱 스코프는 바깥 타이머에 액터 수만 합산하여 시간을 중복 측정하지 않는다. 활성 프레임과 단계가 맞을 때만 시작한다.
     */
    class FScopedWorldTickStatTimer {
    public:
        /**
         * @brief 월드 틱 측정을 시작하거나 바깥 틱 타이머에 액터 수를 합산한다.
         * @param ActorTickCount 이번 스코프에서 집계할 액터 틱 대상 수.
         */
        explicit FScopedWorldTickStatTimer(std::size_t ActorTickCount);
        /**
         * @brief 집계용 포인터를 해제하고 같은 활성 프레임이면 틱 시간과 수집한 계수를 현재 프레임에 합산한다.
         */
        ~FScopedWorldTickStatTimer();
        /**
         * @brief 측정 및 결과 기록 책임을 가진 타이머의 복사 생성을 금지한다.
         */
        FScopedWorldTickStatTimer(const FScopedWorldTickStatTimer&) = delete;
        /**
         * @brief 스코프 타이머의 복사 할당을 금지한다.
         */
        FScopedWorldTickStatTimer& operator=(const FScopedWorldTickStatTimer&) = delete;
        /**
         * @brief 스코프 타이머의 이동 생성을 금지한다.
         */
        FScopedWorldTickStatTimer(FScopedWorldTickStatTimer&&) = delete;
        /**
         * @brief 스코프 타이머의 이동 할당을 금지한다.
         */
        FScopedWorldTickStatTimer& operator=(FScopedWorldTickStatTimer&&) = delete;

    private:
        /**
         * @brief 현재 월드 틱 스코프가 소유하고 호출자가 컴포넌트 계수를 추가할 수 있는 집계 정보.
         */
        FWorldTickStats mStats{};
        /**
         * @brief 활성 스코프의 측정 시작 시각.
         */
        std::chrono::steady_clock::time_point mStartTime{};
        /**
         * @brief 생성 시의 활성 프레임 식별자. 다른 프레임으로 넘어간 결과를 기록하지 않는 데 사용한다.
         */
        std::uint64_t mFrameId{};
        /**
         * @brief 해당 스코프가 실제 측정과 결과 기록을 수행하는지 나타내는 상태.
         */
        bool mActive{};
    };

    /**
     * @brief 활성 RenderPreparation 단계 안에서 세부 작업의 경과 시간을 측정한다.
     * @details 같은 활성 프레임이 유지되면 소멸 시 총 시간과 독점 시간에 같은 값을 더하고 호출 횟수를 증가시킨다.
     */
    class FScopedRenderPreparationStatTimer {
    public:
        /**
         * @brief 유효한 렌더 준비 세부 단계의 측정을 시작한다.
         * @param Stage 측정할 렌더 준비 세부 단계.
         */
        explicit FScopedRenderPreparationStatTimer(ERenderPreparationStage Stage);
        /**
         * @brief 활성화된 같은 프레임의 렌더 준비 세부 표본에 경과 시간과 호출 횟수를 추가한다.
         */
        ~FScopedRenderPreparationStatTimer();
        /**
         * @brief 측정 및 결과 기록 책임을 가진 타이머의 복사 생성을 금지한다.
         */
        FScopedRenderPreparationStatTimer(const FScopedRenderPreparationStatTimer&) = delete;
        /**
         * @brief 스코프 타이머의 복사 할당을 금지한다.
         */
        FScopedRenderPreparationStatTimer& operator=(const FScopedRenderPreparationStatTimer&) = delete;
        /**
         * @brief 스코프 타이머의 이동 생성을 금지한다.
         */
        FScopedRenderPreparationStatTimer(FScopedRenderPreparationStatTimer&&) = delete;
        /**
         * @brief 스코프 타이머의 이동 할당을 금지한다.
         */
        FScopedRenderPreparationStatTimer& operator=(FScopedRenderPreparationStatTimer&&) = delete;

    private:
        /**
         * @brief 현재 스코프에서 측정할 렌더 준비 세부 단계.
         */
        ERenderPreparationStage mStage{};
        /**
         * @brief 활성 스코프의 측정 시작 시각.
         */
        std::chrono::steady_clock::time_point mStartTime{};
        /**
         * @brief 생성 시의 활성 프레임 식별자. 다른 프레임으로 넘어간 결과를 기록하지 않는 데 사용한다.
         */
        std::uint64_t mFrameId{};
        /**
         * @brief 해당 스코프가 실제 측정과 결과 기록을 수행하는지 나타내는 상태.
         */
        bool mActive{};
    };
}
