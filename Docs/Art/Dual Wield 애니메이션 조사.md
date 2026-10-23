# Dual Wield 애니메이션 조사

> 출처: UE 5.8 에셋 레지스트리와 시퀀스 원시 포즈 평가 · 범위: `/Game/Fab/ARPG_Pack/ARPG_Dual_Wield` · 애니메이션 시퀀스 164개

팩의 모든 애니메이션 시퀀스를 분류하고 실제 에셋의 길이, 프레임 수, 루트 이동과 현재 재생 설정을 조사했습니다. 각 모션의 GIF는 표에서 열 수 있습니다. 캐릭터 행동과 판정 구간을 결정하는 기획 문서는 별도로 작성합니다.

## 구성

| 분류 | 시퀀스 수 |
|---|---:|
| 공격 | 13 |
| 처형·처형 피격 | 11 |
| 회피 | 9 |
| 가드·패리 | 9 |
| 가드 이동 | 32 |
| 피격·기상·기절 | 12 |
| 사망 | 5 |
| 점프·공중·착지 | 5 |
| 대기·전환 | 11 |
| 이동 | 56 |
| 기준 자세 | 1 |

## 에셋과 재생 설정

- 시퀀스 폴더: `/Game/Fab/ARPG_Pack/ARPG_Dual_Wield/Animations_UE5`.
- 에셋 이름은 표의 모션 이름 앞에 `ARPG_Dual_Wield_Anim_UE_`를 붙입니다.
- 스켈레톤: `/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin`.
- 현재 루트 모션 활성 0개, 강제 루트 고정 0개. 원본 팩의 기본값과 현재 프로젝트 설정을 구분합니다.
- 노티파이가 있는 시퀀스 0개. 애디티브 시퀀스 0개.
- `_IPC`와 `_Rootmotion`은 별개의 시퀀스입니다. 이름의 `_Rootmotion`은 루트 본 이동 데이터가 있다는 뜻이며, 현재 `Enable Root Motion` 설정과 같은 뜻이 아닙니다.

## 무기

무기 폴더: `/Game/Fab/ARPG_Pack/ARPG_Dual_Wield/Demo/Weapon/Mesh`.

- `Axe`, `Sword`: 도끼와 검 스태틱 메시. 메시 로컬 Y축 바운드 길이는 각각 90.48cm, 122.26cm입니다.
- `Attack_Combo1` ~ `4`의 콤보 계열과 강공격 2종이 있습니다.
- 다른 팩의 공격자와 짝을 맞추는 `Executed_Samurai`·`Executed_Warrior` 및 사망 변형이 있습니다. 이름만으로 두 캐릭터의 거리나 동기화 시점을 확정하지 않습니다.
- 독립된 Jump·Crouch·Climb·Getup·Stun·Buff 계열 시퀀스는 없습니다. `Airborne`, 공중 공격·피격·사망과 착지 시퀀스는 있습니다.

## 모션 선택 시 확인할 점

- 두 손을 쓰는 모션입니다. GIF에는 오른손 검·왼손 도끼를 붙였습니다. 실제 공격별 타격 손과 판정 궤적은 사용할 캐릭터의 무기 구성에 맞춰 정합니다.
- `Attack_Heavy1_Start`와 `Attack_Heavy1`은 준비와 발동이 나뉜 별개 시퀀스입니다. `Attack_Heavy2`는 별도 강공격입니다.
- `Attack_Dodge_to_Combo1`이라는 연결 전용 시퀀스가 있으므로, 회피 공격에서 일반 콤보로 이어질 때 `Attack_Dodge`와 함께 비교할 수 있습니다.
- `Executed`·`Executed2`·`Executed3`에는 각각 사망 변형이 있고, Samurai·Warrior 상대용 변형도 따로 있습니다. 서로 다른 길이이므로 이름만 보고 재생 시간을 공유하지 않습니다.
- 팩의 모든 시퀀스는 현재 루트 모션 옵션과 강제 루트 고정이 꺼져 있습니다. 루트 본에 들어 있는 이동 데이터는 아래 표에서 따로 확인합니다.

## 회전 시퀀스

- 90° 제자리 회전 계열은 루트 본에도 이름과 같은 회전이 들어 있습니다. 표의 루트 회전은 오른쪽이 양수, 왼쪽이 음수입니다.
- `Sprint_Turn180` 네 변형은 루트 본의 누적 회전량이 0°입니다. 몸의 방향 전환 자세와 루트의 회전 데이터는 다르므로, 이름만 보고 캐릭터 액터가 180° 돌아간다고 가정하지 않습니다.

## 측정 방법

측정 단위·좌표·샘플링과 설정 표기는 [시퀀스 측정 기준](<에셋 조사 개요.md#시퀀스-측정-기준>)을 따릅니다.

## 공격 (13)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Attack_Airborne_End](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Airborne_End.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Airborne_Mid](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Airborne_Mid.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Airborne_Start](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Airborne_Start.gif>) | 0.317 | 19 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Combo1](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Combo1.gif>) | 0.733 | 44 | 0.96 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Combo2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Combo2.gif>) | 0.767 | 46 | 1.21 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Combo3](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Combo3.gif>) | 1.250 | 75 | 1.14 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Combo4](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Combo4.gif>) | 0.933 | 56 | 1.38 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Dodge](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Dodge.gif>) | 0.950 | 57 | 2.33 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Dodge_to_Combo1](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Dodge_to_Combo1.gif>) | 0.733 | 44 | 0.96 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Heavy1](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Heavy1.gif>) | 1.000 | 60 | 4.73 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Heavy1_Start](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Heavy1_Start.gif>) | 0.817 | 49 | -0.54 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Heavy2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Heavy2.gif>) | 0.983 | 59 | 2.87 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Sprint](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Attack_Sprint.gif>) | 0.883 | 53 | 2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 처형·처형 피격 (11)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Execute](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Execute.gif>) | 1.733 | 104 | 1.44 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Executed](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Executed.gif>) | 2.500 | 150 | -4.50 | 0.00 | 0.00 | 1.73 | 0.0 | — |
| [Executed2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Executed2.gif>) | 1.617 | 97 | -1.54 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Executed3](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Executed3.gif>) | 3.517 | 211 | -2.44 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Executed_Death](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Executed_Death.gif>) | 1.817 | 109 | -4.10 | 0.00 | 0.00 | 1.73 | 0.0 | — |
| [Executed_Death2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Executed_Death2.gif>) | 1.383 | 83 | -2.02 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Executed_Death3](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Executed_Death3.gif>) | 2.533 | 152 | -2.52 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Executed_Death_Samurai](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Executed_Death_Samurai.gif>) | 1.817 | 109 | -4.13 | 0.00 | 0.00 | 1.73 | 0.0 | — |
| [Executed_Death_Warrior](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Executed_Death_Warrior.gif>) | 1.817 | 109 | -4.10 | 0.00 | 0.00 | 1.73 | 0.0 | — |
| [Executed_Samurai](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Executed_Samurai.gif>) | 2.567 | 154 | -4.56 | 0.00 | 0.00 | 1.73 | 0.0 | — |
| [Executed_Warrior](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Executed_Warrior.gif>) | 2.500 | 150 | -4.57 | 0.00 | 0.00 | 1.73 | 0.0 | — |

## 회피 (9)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Dodge](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Dodge.gif>) | 0.700 | 42 | -2.97 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Backward](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Evade_Backward.gif>) | 0.900 | 54 | -3.96 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Backward_Left](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Evade_Backward_Left.gif>) | 0.900 | 54 | -2.97 | -2.97 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Backward_Right](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Evade_Backward_Right.gif>) | 0.900 | 54 | -2.97 | 2.97 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Forward](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Evade_Forward.gif>) | 0.900 | 54 | 3.96 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Forward_Left](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Evade_Forward_Left.gif>) | 0.900 | 54 | 2.97 | -2.97 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Forward_Right](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Evade_Forward_Right.gif>) | 0.900 | 54 | 2.97 | 2.97 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Left](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Evade_Left.gif>) | 0.900 | 54 | 0.00 | -3.96 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Right](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Evade_Right.gif>) | 0.900 | 54 | 0.00 | 3.96 | 0.00 | 0.00 | 0.0 | — |

## 가드·패리 (9)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Guard](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Break](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Break.gif>) | 0.817 | 49 | -0.77 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Hit1](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Hit1.gif>) | 0.567 | 34 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Hit2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Hit2.gif>) | 0.567 | 34 | -0.50 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Hit3](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Hit3.gif>) | 0.567 | 34 | -0.50 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Start](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Start.gif>) | 0.117 | 7 | -0.08 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Turn90L](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Turn90L.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | -90.0 | — |
| [Guard_Turn90R](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Turn90R.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 90.0 | — |
| [Parry](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Parry.gif>) | 0.500 | 30 | 0.08 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 가드 이동 (32)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Guard_Run_Backward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Backward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Backward_Left_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Backward_Left_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Backward_Left_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Backward_Left_Rootmotion.gif>) | 0.733 | 44 | -1.93 | -1.93 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Backward_Right_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Backward_Right_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Backward_Right_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Backward_Right_Rootmotion.gif>) | 0.733 | 44 | -1.93 | 1.93 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Backward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Backward_Rootmotion.gif>) | 0.733 | 44 | -2.73 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Forward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Forward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Forward_Left_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Forward_Left_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Forward_Left_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Forward_Left_Rootmotion.gif>) | 0.733 | 44 | 1.93 | -1.93 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Forward_Right_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Forward_Right_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Forward_Right_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Forward_Right_Rootmotion.gif>) | 0.733 | 44 | 1.93 | 1.93 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Forward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Forward_Rootmotion.gif>) | 0.733 | 44 | 2.73 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Leftward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Leftward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Leftward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Leftward_Rootmotion.gif>) | 0.733 | 44 | 0.00 | -2.73 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Rightward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Rightward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Rightward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Run_Rightward_Rootmotion.gif>) | 0.733 | 44 | 0.00 | 2.73 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Backward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Backward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Backward_Left_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Backward_Left_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Backward_Left_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Backward_Left_Rootmotion.gif>) | 0.983 | 59 | -1.75 | -1.75 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Backward_Right_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Backward_Right_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Backward_Right_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Backward_Right_Rootmotion.gif>) | 0.983 | 59 | -1.75 | 1.75 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Backward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Backward_Rootmotion.gif>) | 0.983 | 59 | -2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Forward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Forward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Forward_Left_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Forward_Left_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Forward_Left_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Forward_Left_Rootmotion.gif>) | 0.983 | 59 | 1.75 | -1.75 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Forward_Right_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Forward_Right_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Forward_Right_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Forward_Right_Rootmotion.gif>) | 0.983 | 59 | 1.75 | 1.75 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Forward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Forward_Rootmotion.gif>) | 0.983 | 59 | 2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Leftward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Leftward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Leftward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Leftward_Rootmotion.gif>) | 0.983 | 59 | 0.00 | -2.48 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Rightward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Rightward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Rightward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Guard_Walk_Rightward_Rootmotion.gif>) | 0.983 | 59 | 0.00 | 2.48 | 0.00 | 0.00 | 0.0 | — |

## 피격·기상·기절 (12)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Collide](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Collide.gif>) | 1.067 | 64 | -0.74 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit1](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Hit1.gif>) | 0.817 | 49 | -0.59 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Hit2.gif>) | 0.817 | 49 | -0.64 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit3](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Hit3.gif>) | 0.817 | 49 | -0.86 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit4](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Hit4.gif>) | 0.817 | 49 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit5](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Hit5.gif>) | 0.817 | 49 | 0.31 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_Air](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Hit_Air.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_Air_Down](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Hit_Air_Down.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_Air_Up](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Hit_Air_Up.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_Airborne](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Hit_Airborne.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_knockdown](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Hit_knockdown.gif>) | 1.317 | 79 | -4.01 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_knockdown_Death](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Hit_knockdown_Death.gif>) | 1.567 | 94 | -3.11 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 사망 (5)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Death](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Death.gif>) | 0.950 | 57 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Death2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Death2.gif>) | 1.233 | 74 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Death_Airborne_End](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Death_Airborne_End.gif>) | 0.233 | 14 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Death_Airborne_Mid](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Death_Airborne_Mid.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Death_Airborne_Start](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Death_Airborne_Start.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 점프·공중·착지 (5)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Airborne](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Airborne.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Landing](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Landing.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Landing2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Landing2.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Landing3](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Landing3.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Landing4](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Landing4.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 대기·전환 (11)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Equip_Idle1](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Equip_Idle1.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Equip_Idle2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Equip_Idle2.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Idle1](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Idle1.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Idle1_to_Idle2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Idle1_to_Idle2.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Idle2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Idle2.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Idle2_to_Idle1](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Idle2_to_Idle1.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Idle_no_weapon](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Idle_no_weapon.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Turn_90L](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Turn_90L.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | -90.0 | — |
| [Turn_90R](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Turn_90R.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 90.0 | — |
| [Unequip_Idle1](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Unequip_Idle1.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Unequip_Idle2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Unequip_Idle2.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 이동 (56)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Brake](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Brake.gif>) | 0.417 | 25 | 0.40 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Brake2](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Brake2.gif>) | 0.417 | 25 | 0.12 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Brake2_no_weapon](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Brake2_no_weapon.gif>) | 0.417 | 25 | 0.33 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Brake_no_weapon](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Brake_no_weapon.gif>) | 0.417 | 25 | 0.33 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Backward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Backward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Backward_Left_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Backward_Left_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Backward_Left_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Backward_Left_Rootmotion.gif>) | 0.733 | 44 | -1.93 | -1.93 | 0.00 | 0.00 | 0.0 | — |
| [Run_Backward_Right_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Backward_Right_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Backward_Right_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Backward_Right_Rootmotion.gif>) | 0.733 | 44 | -1.93 | 1.93 | 0.00 | 0.00 | 0.0 | — |
| [Run_Backward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Backward_Rootmotion.gif>) | 0.733 | 44 | -2.73 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Forward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Forward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Forward_Left_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Forward_Left_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Forward_Left_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Forward_Left_Rootmotion.gif>) | 0.733 | 44 | 1.93 | -1.93 | 0.00 | 0.00 | 0.0 | — |
| [Run_Forward_Right_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Forward_Right_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Forward_Right_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Forward_Right_Rootmotion.gif>) | 0.733 | 44 | 1.93 | 1.93 | 0.00 | 0.00 | 0.0 | — |
| [Run_Forward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Forward_Rootmotion.gif>) | 0.733 | 44 | 2.73 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Leftward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Leftward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Leftward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Leftward_Rootmotion.gif>) | 0.733 | 44 | 0.00 | -2.73 | 0.00 | 0.00 | 0.0 | — |
| [Run_Rightward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Rightward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Rightward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_Rightward_Rootmotion.gif>) | 0.733 | 44 | 0.00 | 2.73 | 0.00 | 0.00 | 0.0 | — |
| [Run_no_weapon_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_no_weapon_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_no_weapon_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Run_no_weapon_Rootmotion.gif>) | 0.733 | 44 | 2.73 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_IPC.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_Rootmotion.gif>) | 0.650 | 39 | 2.97 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Start_L_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_Start_L_IPC.gif>) | 0.750 | 45 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Start_L_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_Start_L_Rootmotion.gif>) | 0.750 | 45 | 3.42 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Start_R_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_Start_R_IPC.gif>) | 0.750 | 45 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Start_R_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_Start_R_Rootmotion.gif>) | 0.750 | 45 | 3.42 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Start_no_weapon_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_Start_no_weapon_IPC.gif>) | 0.750 | 45 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Start_no_weapon_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_Start_no_weapon_Rootmotion.gif>) | 0.750 | 45 | 3.42 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Turn180_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_Turn180_IPC.gif>) | 0.550 | 33 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Turn180_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_Turn180_Rootmotion.gif>) | 0.550 | 33 | 2.41 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Turn180_no_weapon_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_Turn180_no_weapon_IPC.gif>) | 0.550 | 33 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Turn180_no_weapon_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_Turn180_no_weapon_Rootmotion.gif>) | 0.550 | 33 | 2.41 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_no_weapon_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_no_weapon_IPC.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_no_weapon_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Sprint_no_weapon_Rootmotion.gif>) | 0.650 | 39 | 2.97 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk2_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk2_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk2_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk2_Rootmotion.gif>) | 0.983 | 59 | 2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Backward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Backward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Backward_Left_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Backward_Left_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Backward_Left_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Backward_Left_Rootmotion.gif>) | 0.983 | 59 | -1.75 | -1.75 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Backward_Right_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Backward_Right_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Backward_Right_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Backward_Right_Rootmotion.gif>) | 0.983 | 59 | -1.75 | 1.75 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Backward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Backward_Rootmotion.gif>) | 0.983 | 59 | -2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Forward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Forward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Forward_Left_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Forward_Left_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Forward_Left_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Forward_Left_Rootmotion.gif>) | 0.983 | 59 | 1.75 | -1.75 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Forward_Right_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Forward_Right_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Forward_Right_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Forward_Right_Rootmotion.gif>) | 0.983 | 59 | 1.75 | 1.75 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Forward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Forward_Rootmotion.gif>) | 0.983 | 59 | 2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Leftward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Leftward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Leftward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Leftward_Rootmotion.gif>) | 0.983 | 59 | 0.00 | -2.48 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Rightward_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Rightward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Rightward_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_Rightward_Rootmotion.gif>) | 0.983 | 59 | 0.00 | 2.48 | 0.00 | 0.00 | 0.0 | — |
| [Walk_no_weapon_IPC](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_no_weapon_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_no_weapon_Rootmotion](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_Walk_no_weapon_Rootmotion.gif>) | 0.983 | 59 | 2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 기준 자세 (1)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [A_Bindpose](<GIF/Dual_Wield/ARPG_Dual_Wield_Anim_UE_A_Bindpose.gif>) | 0.033 | 1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
