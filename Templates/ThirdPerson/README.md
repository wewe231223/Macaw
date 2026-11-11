# Third Person

이동과 점프가 준비된 3인칭 게임의 시작 프로젝트입니다. 프로젝트 생성 화면에서 Third Person을 선택한 다음, C# 자동 빌드가 끝나면 Play를 누르세요. 스크립트 빌드에는 .NET 10 SDK가 필요합니다.

| 입력 | 동작 |
|---|---|
| W / A / S / D | 카메라 방향을 기준으로 이동 |
| 마우스 | 캐릭터 주위로 카메라 회전 |
| Space | 점프 |

시작 맵은 `Content/Maps/Main.masset`입니다. 플레이어 한 명과 바닥, 외벽, 계단, 경사로, 높이가 다른 블록이 배치되어 있습니다. 캐릭터는 이동 방향으로 회전하며, 카메라는 장애물 앞에서 거리를 줄입니다.

`Scripts/ThirdPersonController.cs`에서 입력, 이동, 점프, 카메라를 수정할 수 있습니다. 캐릭터의 Controller 컴포넌트에서는 이동 속도, 가속도, 점프 속도, 중력, 회전 속도, 계단 높이, 카메라 거리와 마우스 감도를 조절할 수 있습니다.

애니메이션 그래프와 Blend Space는 `Content/Animation`, 캐릭터의 모션 에셋은 `Content/Characters`에 있습니다.

캐릭터 에셋은 `Content/Characters`, 시작 맵은 `Content/Maps`, 레벨 재질은 `Content/Materials`에 있습니다. 맵의 블록을 바꾸고 게임 로직을 추가해 프로젝트를 확장할 수 있습니다.
