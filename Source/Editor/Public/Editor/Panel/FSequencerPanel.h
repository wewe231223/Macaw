/**
 * @file FSequencerPanel.h
 * @brief 시퀀스 액터의 타임라인 편집과 프리뷰를 제공하는 에디터 패널입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/TObjectRef.h"
#include "Editor/Panel/FEditorPanel.h"
#include "Engine/Sequence/FSequencePlayer.h"

namespace Asset {
    class FSequenceBinding;
    class FSequenceChannel;
    class ULevelSequence;
}

namespace Engine {
    class ALevelSequenceActor;
}

namespace Editor {

    /**
     * @brief 시퀀스 액터의 타임라인 편집과 프리뷰를 제공하는 에디터 패널입니다.
     * @ingroup Editor
     * @note 프리뷰 값은 편집 월드에 직접 쓰며 장면 수정 상태로 표시하지 않습니다. 저장, 장면 교체와 PIE 시작 전에 복원합니다.
     */
    class FSequencerPanel final : public Editor::FEditorPanel {
    private:
        /**
         * @brief 타임라인에서 선택하거나 드래그 중인 키입니다.
         */
        struct FKeySelection {
        public:
            /**
             * @brief 키가 속한 바인딩의 순서이며 음수이면 선택이 없습니다.
             */
            Core::Int32 mBinding{-1};

            /**
             * @brief 키가 속한 채널의 순서입니다.
             */
            Core::Int32 mChannel{};

            /**
             * @brief 키의 시간이며 틱 단위입니다.
             */
            Core::Int32 mTick{};
        };

    public:
        /**
         * @brief 패널을 구성하고 컨텍스트에 프리뷰 복원 콜백을 등록합니다.
         * @param[in,out] Context 패널이 조회하고 편집할 에디터 컨텍스트입니다.
         */
        explicit FSequencerPanel(Editor::FEditorContext* Context);

        /**
         * @brief 프리뷰 복원 콜백을 해제합니다.
         */
        ~FSequencerPanel() override;

    public:
        /**
         * @brief 프리뷰를 끝내고 월드에 쓴 값을 원래대로 되돌립니다.
         * @note 프리뷰 중이 아니면 아무 작업도 하지 않습니다.
         */
        void RestorePreview();

    private:
        /**
         * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
         */
        void DrawContent(float DeltaTime) override;

        /**
         * @brief 편집할 시퀀스 액터를 고르거나 만드는 위젯을 그립니다.
         * @return 편집할 시퀀스 액터의 비소유 주소이며 없으면 nullptr입니다.
         */
        Engine::ALevelSequenceActor* DrawActorPicker();

        /**
         * @brief 액터가 재생할 시퀀스 에셋을 고르거나 만드는 위젯을 그립니다.
         * @param[in,out] Actor 편집 중인 시퀀스 액터입니다.
         * @return 편집할 시퀀스의 비소유 주소이며 없으면 nullptr입니다.
         */
        Asset::ULevelSequence* DrawSequencePicker(Engine::ALevelSequenceActor* Actor);

        /**
         * @brief 재생 컨트롤과 시퀀스 설정 위젯을 그립니다.
         * @param[in,out] Actor 편집 중인 시퀀스 액터입니다.
         * @param[in,out] Sequence 편집 중인 시퀀스입니다.
         */
        void DrawToolbar(Engine::ALevelSequenceActor* Actor, Asset::ULevelSequence* Sequence);

        /**
         * @brief 바인딩을 고르고 추가·제거하며 키를 찍는 위젯을 그립니다.
         * @param[in,out] Actor 편집 중인 시퀀스 액터입니다.
         * @param[in,out] Sequence 편집 중인 시퀀스입니다.
         */
        void DrawBindingBar(Engine::ALevelSequenceActor* Actor, Asset::ULevelSequence* Sequence);

        /**
         * @brief 바인딩별 채널의 키를 타임라인으로 그리고 스크럽과 드래그를 처리합니다.
         * @param[in,out] Sequence 편집 중인 시퀀스입니다.
         * @param[in] Height 타임라인 높이이며 픽셀 단위입니다.
         */
        void DrawTimeline(Asset::ULevelSequence* Sequence, float Height);

        /**
         * @brief 선택한 키의 값과 보간 방식을 편집하는 위젯을 그립니다.
         * @param[in,out] Sequence 편집 중인 시퀀스입니다.
         */
        void DrawKeyEditor(Asset::ULevelSequence* Sequence);

        /**
         * @brief 선택한 카메라 컷의 카메라를 바꾸거나 컷을 지우는 위젯을 그립니다.
         * @param[in,out] Sequence 편집 중인 시퀀스입니다.
         */
        void DrawCameraCutEditor(Asset::ULevelSequence* Sequence);

        /**
         * @brief 카메라 컷 구간과 시작 마커를 타임라인 한 줄로 그립니다.
         * @param[in,out] Sequence 편집 중인 시퀀스입니다.
         * @param[in] EndFrame 재생 구간의 끝 프레임입니다.
         */
        void DrawCameraCutRow(Asset::ULevelSequence* Sequence, Core::Int32 EndFrame);

        /**
         * @brief 채널의 키를 타임라인 한 줄로 그립니다.
         * @param[in] Sequence 편집 중인 시퀀스입니다.
         * @param[in] Binding 채널이 속한 바인딩의 순서입니다.
         * @param[in] ChannelId 패널이 구분하는 채널 번호입니다.
         * @param[in] Label 줄에 표시할 이름입니다.
         * @param[in] Channel 그릴 채널입니다.
         */
        void DrawChannelRow(const Asset::ULevelSequence* Sequence, Core::Int32 Binding, Core::Int32 ChannelId, const char* Label, const Asset::FSequenceChannel& Channel);

        /**
         * @brief 키 마커를 그리고 클릭하면 선택과 드래그를 시작합니다.
         * @param[in] Sequence 편집 중인 시퀀스입니다.
         * @param[in] Binding 키가 속한 바인딩의 순서입니다.
         * @param[in] ChannelId 패널이 구분하는 채널 번호입니다.
         * @param[in] Tick 키의 시간이며 틱 단위입니다.
         * @param[in] Index 줄 안에서 키를 구분하는 번호입니다.
         * @return 키 위에 마우스가 있으면 true입니다.
         */
        bool DrawKeyMarker(const Asset::ULevelSequence* Sequence, Core::Int32 Binding, Core::Int32 ChannelId, Core::Int32 Tick, Core::Int32 Index);

        /**
         * @brief 선택한 바인딩의 현재 변환으로 현재 프레임에 키를 찍습니다.
         * @param[in,out] Sequence 편집 중인 시퀀스입니다.
         */
        void AddTransformKeys(Asset::ULevelSequence* Sequence);

        /**
         * @brief 선택한 바인딩의 현재 프로퍼티 값으로 현재 프레임에 키를 찍습니다.
         * @param[in,out] Sequence 편집 중인 시퀀스입니다.
         */
        void AddPropertyKeys(Asset::ULevelSequence* Sequence);

        /**
         * @brief 현재 복원 상태를 정리하고 플레이어를 시퀀스와 액터에 다시 연결합니다.
         * @param[in,out] Actor 편집 중인 시퀀스 액터입니다.
         * @param[in] Sequence 편집 중인 시퀀스입니다.
         */
        void RebindPlayer(Engine::ALevelSequenceActor* Actor, const Asset::ULevelSequence* Sequence);

        /**
         * @brief 지정한 시간으로 이동하여 프리뷰를 켭니다.
         * @param[in] Tick 이동할 시간이며 틱 단위입니다.
         */
        void SeekPreview(double Tick);

        /**
         * @brief 시퀀스 변경을 표시하고 프리뷰 중이면 현재 시간의 값을 다시 적용합니다.
         */
        void NotifySequenceChanged();

    private:
        /**
         * @brief 프리뷰에 사용하는 시퀀스 플레이어입니다.
         */
        Engine::FSequencePlayer mPlayer{};

        /**
         * @brief 편집 중인 시퀀스 액터의 핸들입니다.
         */
        CoreUObject::TObjectRef<Engine::ALevelSequenceActor> mActor{};

        /**
         * @brief 플레이어에 연결된 액터의 핸들입니다.
         */
        CoreUObject::TObjectRef<Engine::ALevelSequenceActor> mBoundActor{};

        /**
         * @brief 플레이어에 연결된 시퀀스의 비소유 주소입니다.
         */
        CoreUObject::TObjectRef<const Asset::ULevelSequence> mBoundSequence{};

        /**
         * @brief 시퀀스 콤보에 표시할 Content 상대 경로 목록입니다.
         */
        Core::TArray<Core::FString> mSequencePaths{};

        /**
         * @brief 마지막으로 로드에 실패한 시퀀스 경로이며 매 프레임 재시도를 막습니다.
         */
        Core::FString mFailedPath{};

        /**
         * @brief 바인딩별 그룹의 펼침 상태입니다.
         */
        Core::TMap<Core::FString, bool> mOpenBindings{};

        /**
         * @brief 선택한 키입니다.
         */
        FKeySelection mSelection{};

        /**
         * @brief 마지막으로 확인한 에디터 선택의 개정 번호입니다.
         */
        Core::Uint64 mSelectionRevision{};

        /**
         * @brief 키를 찍고 제거할 바인딩의 순서입니다.
         */
        Core::Int32 mActiveBinding{};

        /**
         * @brief 키 드래그를 시작한 프레임입니다.
         */
        Core::Int32 mDragStartFrame{};

        /**
         * @brief 키 드래그를 시작한 마우스 가로 위치입니다.
         */
        float mDragStartMouseX{};

        /**
         * @brief 키 드래그 동안 사용하는 프레임당 픽셀 수입니다.
         */
        float mDragFrameWidth{1.0f};

        /**
         * @brief 선택한 키를 드래그 중인지 여부입니다.
         */
        bool mDragging{};

        /**
         * @brief 프리뷰 값이 월드에 적용되어 있는지 여부입니다.
         */
        bool mPreviewActive{};

        /**
         * @brief 프리뷰를 반복 재생할지 여부입니다.
         */
        bool mLoop{};

        /**
         * @brief 프리뷰 중 원근 뷰포트를 카메라 컷 시점으로 전환할지 여부입니다.
         */
        bool mPreviewCuts{true};

        /**
         * @brief 저장하지 않은 시퀀스 변경이 있는지 여부입니다.
         */
        bool mSequenceDirty{};
    };

}
