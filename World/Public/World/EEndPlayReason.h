#pragma once

/// <summary>Actor와 컴포넌트의 플레이가 끝난 이유를 구분합니다.</summary>
enum class EEndPlayReason {
    Destroyed, // 명시적인 파괴 요청입니다.
    LevelTransition, // 다른 장면으로 전환하면서 종료합니다.
    EndPlayInEditor, // 에디터의 플레이 세션을 종료합니다.
    RemovedFromWorld, // 소속 World에서 제거되어 종료합니다.
    Quit // World 정리나 프로그램 종료에 따른 종료입니다.
};
