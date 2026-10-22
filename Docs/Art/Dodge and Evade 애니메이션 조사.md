# Dodge and Evade 애니메이션 조사

> 갱신일 2026-10-22 · 출처: 언리얼 에디터 파이썬 포즈 평가

플레이어의 극한 회피에 쓸 Fab Dodge and Evade Anims(RamsterZ) 팩의 구성과 측정값을 정리한 메모입니다. 플레이어가 쓰는 모션은 [플레이어 사양](<../Design/플레이어 사양.md>)의 '극한 회피'를 따릅니다.

## 구성

- `/Game/Fab/Dodge_and_Evade_Anims/Anim_Seq_UE5`에 애니메이션 시퀀스 36개가 있습니다. 팩 원본은 UE4 마네킹용이라, 프로젝트의 `SK_Mannequin`으로 리타깃한 사본만 남겼습니다.
- 리타깃 사본은 원본보다 이동이 약 2% 깁니다. 골반 높이는 원본과 1cm 안쪽으로 같습니다.
- 맨손 동작입니다. 손을 쥐지 않아 카타나를 붙이면 칼 방향이 손목을 따라 바뀝니다.
- 시작과 끝은 팩의 맨손 대기 자세입니다. Samurai 팩의 대기 자세와 다르므로 블렌드로 잇습니다.
- `bEnableRootMotion`은 꺼져 있고, 루트 본에 이동이 들어 있습니다.

## 종류

| 종류 | 방향 | 동작 |
| - | - | - |
| `AvoidBallStep` | 좌·우 | 몸을 숙이며 옆으로 비켜섭니다. |
| `BigStep` | 앞·뒤·좌·우 | 크게 한 걸음 뛰어 비킵니다. |
| `Cartwheel` | 앞·뒤·좌·우 | 손을 짚고 물구나무 자세를 지나는 옆돌기입니다. |
| `Dive` | 좌·우 | 몸을 날려 엎드리듯 바닥을 짚은 뒤 일어납니다. |
| `EvadeStep` | 앞·뒤·좌·우 | 짧게 스텝을 밟습니다. |
| `Float` | 좌·우 | 낮게 웅크린 채 옆으로 길게 미끄러지듯 이동합니다. 손은 짚지 않습니다. |
| `HugeStep` | 앞·뒤·좌·우 | 웅크리며 멀리 뛰어 비킵니다. |
| `InPlace_Evade` | 앞·뒤·좌·우 | 제자리에서 상체로 피합니다. |
| `NoHandSpin` | 좌·우 | 손을 짚지 않는 공중 옆돌기입니다. 아래 '공중 옆돌기'를 참고합니다. |
| `Roll` | 앞·뒤·좌·우 | 구르기입니다. |
| `SmallStep` | 앞·뒤·좌·우 | 작게 스텝을 밟습니다. |

## 측정값

- 길이와 전진·오른쪽은 [에셋 조사 개요](<에셋 조사 개요.md>)의 '시퀀스 측정 기준'을 따릅니다.
- 손목 최저 높이는 30fps로 평가한 `hand_r`·`hand_l` 본 높이의 최솟값(cm)입니다. 10cm보다 낮으면 손으로 바닥을 짚는 동작입니다.

| 시퀀스 | 길이(초) | 전진(m) | 오른쪽(m) | 손목 최저 높이(cm) |
| - | -: | -: | -: | -: |
| [Dodge_AvoidBallStep_Left](<GIF/Dodge_and_Evade/Dodge_AvoidBallStep_Left.gif>) | 1.33 | 0.00 | -1.47 | 67 |
| [Dodge_AvoidBallStep_Right](<GIF/Dodge_and_Evade/Dodge_AvoidBallStep_Right.gif>) | 1.33 | 0.00 | 1.50 | 55 |
| [Dodge_BigStep_Back](<GIF/Dodge_and_Evade/Dodge_BigStep_Back.gif>) | 1.67 | -1.71 | 0.00 | 61 |
| [Dodge_BigStep_Forward](<GIF/Dodge_and_Evade/Dodge_BigStep_Forward.gif>) | 1.67 | 1.81 | 0.00 | 62 |
| [Dodge_BigStep_Left](<GIF/Dodge_and_Evade/Dodge_BigStep_Left.gif>) | 1.67 | -0.03 | -1.76 | 66 |
| [Dodge_BigStep_Right](<GIF/Dodge_and_Evade/Dodge_BigStep_Right.gif>) | 1.67 | 0.00 | 1.79 | 53 |
| [Dodge_Cartwheel_Back](<GIF/Dodge_and_Evade/Dodge_Cartwheel_Back.gif>) | 1.33 | -2.93 | 0.00 | 8 |
| [Dodge_Cartwheel_Forward](<GIF/Dodge_and_Evade/Dodge_Cartwheel_Forward.gif>) | 1.33 | 2.63 | 0.00 | 6 |
| [Dodge_Cartwheel_Left](<GIF/Dodge_and_Evade/Dodge_Cartwheel_Left.gif>) | 1.33 | 0.00 | -2.95 | 5 |
| [Dodge_Cartwheel_Right](<GIF/Dodge_and_Evade/Dodge_Cartwheel_Right.gif>) | 1.33 | 0.00 | 2.67 | 7 |
| [Dodge_Dive_Left](<GIF/Dodge_and_Evade/Dodge_Dive_Left.gif>) | 2.00 | 0.00 | -5.24 | -1 |
| [Dodge_Dive_Right](<GIF/Dodge_and_Evade/Dodge_Dive_Right.gif>) | 2.00 | 0.00 | 5.27 | -2 |
| [Dodge_EvadeStep_Back](<GIF/Dodge_and_Evade/Dodge_EvadeStep_Back.gif>) | 1.33 | -1.13 | 0.00 | 83 |
| [Dodge_EvadeStep_Forward](<GIF/Dodge_and_Evade/Dodge_EvadeStep_Forward.gif>) | 1.33 | 1.13 | 0.00 | 80 |
| [Dodge_EvadeStep_Left](<GIF/Dodge_and_Evade/Dodge_EvadeStep_Left.gif>) | 1.33 | 0.00 | -0.87 | 80 |
| [Dodge_EvadeStep_Right](<GIF/Dodge_and_Evade/Dodge_EvadeStep_Right.gif>) | 1.33 | 0.00 | 0.95 | 75 |
| [Dodge_Float_Left](<GIF/Dodge_and_Evade/Dodge_Float_Left.gif>) | 1.67 | 0.00 | -5.09 | 33 |
| [Dodge_Float_Right](<GIF/Dodge_and_Evade/Dodge_Float_Right.gif>) | 1.67 | 0.00 | 5.12 | 34 |
| [Dodge_HugeStep_Back](<GIF/Dodge_and_Evade/Dodge_HugeStep_Back.gif>) | 1.67 | -2.82 | 0.00 | 57 |
| [Dodge_HugeStep_Forward](<GIF/Dodge_and_Evade/Dodge_HugeStep_Forward.gif>) | 1.67 | 2.86 | 0.00 | 43 |
| [Dodge_HugeStep_Left](<GIF/Dodge_and_Evade/Dodge_HugeStep_Left.gif>) | 1.67 | 0.00 | -2.81 | 26 |
| [Dodge_HugeStep_Right](<GIF/Dodge_and_Evade/Dodge_HugeStep_Right.gif>) | 1.67 | 0.00 | 2.89 | 36 |
| [Dodge_InPlace_Evade_Back](<GIF/Dodge_and_Evade/Dodge_InPlace_Evade_Back.gif>) | 1.33 | 0.00 | 0.00 | 76 |
| [Dodge_InPlace_Evade_Forward](<GIF/Dodge_and_Evade/Dodge_InPlace_Evade_Forward.gif>) | 1.33 | 0.00 | 0.00 | 83 |
| [Dodge_InPlace_Evade_Left](<GIF/Dodge_and_Evade/Dodge_InPlace_Evade_Left.gif>) | 1.33 | 0.00 | 0.00 | 80 |
| [Dodge_InPlace_Evade_Right](<GIF/Dodge_and_Evade/Dodge_InPlace_Evade_Right.gif>) | 1.33 | 0.00 | 0.00 | 79 |
| [Dodge_NoHandSpin_Left](<GIF/Dodge_and_Evade/Dodge_NoHandSpin_Left.gif>) | 1.33 | 0.00 | -2.75 | 41 |
| [Dodge_NoHandSpin_Right](<GIF/Dodge_and_Evade/Dodge_NoHandSpin_Right.gif>) | 1.33 | 0.00 | 2.78 | 35 |
| [Dodge_Roll_Back](<GIF/Dodge_and_Evade/Dodge_Roll_Back.gif>) | 1.33 | -4.39 | 0.00 | 2 |
| [Dodge_Roll_Forward](<GIF/Dodge_and_Evade/Dodge_Roll_Forward.gif>) | 1.33 | 4.46 | 0.00 | 3 |
| [Dodge_Roll_Left](<GIF/Dodge_and_Evade/Dodge_Roll_Left.gif>) | 1.33 | 0.00 | -4.41 | 1 |
| [Dodge_Roll_Right](<GIF/Dodge_and_Evade/Dodge_Roll_Right.gif>) | 1.33 | 0.00 | 4.44 | -1 |
| [Dodge_SmallStep_Back](<GIF/Dodge_and_Evade/Dodge_SmallStep_Back.gif>) | 1.33 | -1.18 | 0.00 | 74 |
| [Dodge_SmallStep_Forward](<GIF/Dodge_and_Evade/Dodge_SmallStep_Forward.gif>) | 1.33 | 0.91 | 0.00 | 67 |
| [Dodge_SmallStep_Left](<GIF/Dodge_and_Evade/Dodge_SmallStep_Left.gif>) | 1.33 | 0.00 | -0.80 | 70 |
| [Dodge_SmallStep_Right](<GIF/Dodge_and_Evade/Dodge_SmallStep_Right.gif>) | 1.33 | 0.00 | 0.82 | 78 |

`Dive`와 `Roll`은 바닥을 짚을 때 손목 본이 바닥보다 1~2cm 낮게 내려가 손바닥이 바닥에 조금 묻힐 수 있습니다.

## 공중 옆돌기

`NoHandSpin`의 두 방향은 서로 좌우 반전이 아니라 다른 동작입니다. 시각은 시퀀스 기준입니다.

| 단계 | Left | Right |
| - | - | - |
| 준비 | 0 ~ 0.17초 팔을 들고, 0.30초까지 골반을 16cm 낮추며 몸을 기울입니다. | 0 ~ 0.23초 팔을 들고, 0.30초까지 몸을 56° 기울입니다. |
| 도약 | 0.33초에 오른발, 0.43초에 왼발이 바닥을 떠납니다. | 0.33초부터 왼다리를 차올리고, 0.43초에 오른발이 바닥을 떠납니다. |
| 공중 | 0.47 ~ 0.53초에 몸이 수평에 가깝게 눕고, 두 발이 함께 1.2m 넘게 올라갑니다. | 0.43 ~ 0.53초에 몸이 수평보다 조금 더 기웁니다. 다리를 번갈아 1.4m까지 차올리지만, 두 발이 함께 뜬 높이는 최고 0.6m입니다. |
| 착지 | 0.67초에 오른발, 0.73초에 왼발이 닿습니다. 이동은 0.90초에 끝납니다. | 0.57초에 왼발, 0.73초에 오른발이 닿습니다. 이동은 0.70초에 대부분 끝납니다. |
| 일어서기 | 0.93초에 섭니다. 1.33초까지 대기 자세로 돌아갑니다. | 0.93초에 섭니다. 1.33초까지 대기 자세로 돌아갑니다. |
