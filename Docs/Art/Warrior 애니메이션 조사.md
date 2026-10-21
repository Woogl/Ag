# Warrior 애니메이션 조사

> 조사일 2026-10-21 · 출처: UE 5.8 에셋 레지스트리와 시퀀스 원시 포즈 평가 · 범위: `/Game/Fab/ARPG_Pack/ARPG_Warrior` · 애니메이션 시퀀스 273개

팩의 모든 애니메이션 시퀀스를 분류하고 실제 에셋의 길이, 프레임 수, 루트 이동과 현재 재생 설정을 조사했습니다. 각 모션의 GIF는 표에서 열 수 있습니다. 캐릭터 행동과 판정 구간을 결정하는 기획 문서는 별도로 작성합니다.

## 구성

| 분류 | 시퀀스 수 |
|---|---:|
| 공격 | 27 |
| 처형·처형 피격 | 7 |
| 회피 | 9 |
| 가드·패리 | 11 |
| 가드 이동 | 32 |
| 피격·기상·기절 | 36 |
| 사망 | 5 |
| 점프·공중·착지 | 16 |
| 대기·전환 | 14 |
| 이동 | 56 |
| 숙이기 | 46 |
| 오르기 | 13 |
| 기준 자세 | 1 |

## 에셋과 재생 설정

- 시퀀스 폴더: `/Game/Fab/ARPG_Pack/ARPG_Warrior/Animations_UE5`.
- 에셋 이름은 표의 모션 이름 앞에 `ARPG_Warrior_Anim_UE_`를 붙입니다.
- 스켈레톤: `/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin`.
- 현재 루트 모션 활성 3개, 강제 루트 고정 4개. 원본 팩의 기본값과 현재 프로젝트 설정을 구분합니다.
- 노티파이가 있는 시퀀스 0개. 애디티브 시퀀스 0개.
- `_IPC`와 `_Rootmotion`은 별개의 시퀀스입니다. 이름의 `_Rootmotion`은 루트 본 이동 데이터가 있다는 뜻이며, 현재 `Enable Root Motion` 설정과 같은 뜻이 아닙니다.

## 무기

무기 폴더: `/Game/Fab/ARPG_Pack/ARPG_Warrior/Demo/Weapon/Mesh`.

- `Sword`, `Shield`: 검과 방패 스태틱 메시. 메시 로컬 Y축 바운드 길이는 각각 122.26cm, 68.22cm입니다.
- `Attack_Combo1` ~ `4`와 `Attack_ComboB1` ~ `5`의 두 콤보 계열이 있습니다.
- 강공격에는 시작·유지·발동 구간과 유지 중 45°·90° 회전 시퀀스가 따로 있습니다.
- 점프 9방향 계열, 숙이기와 오르기, 기상·기절 시퀀스까지 있어 전투 외 동작 후보도 확인할 수 있습니다.

## 모션 선택 시 확인할 점

- 콤보 A는 4타, B는 5타로 나뉩니다. 이름의 번호는 순서를 가리키지만, 이전 시퀀스의 끝과 다음 시퀀스의 시작이 자동으로 자연스럽게 연결된다는 뜻은 아닙니다.
- `Attack_Heavy1_Start`는 뒤로 물러나는 준비 구간이고 `Attack_Heavy1`은 전진하는 발동 구간입니다. `Attack_Heavy_Mid`와 그 회전 변형은 별도 시퀀스이므로 유지 시간은 해당 동작의 기획에서 정합니다.
- 공중 공격에는 Start·Mid·End 외에 `Attack_Airborne`과 `Attack_Airborne_Landing`이 있습니다. 점프·낙하·착지 동작과 함께 비교할 수 있습니다.
- 처형 공격자는 `Execute`, `Execute_B`, `Execute_B2`, 피해자는 `Executed`, `Executed_B`, `Executed_B_2`, `Executed_Death`입니다. 이름에 같은 B가 있어도 대응 상대·시작 위치·동기화 시각은 시각적으로 맞춰 확인해야 합니다.
- 현재 루트 모션을 켠 시퀀스는 `Execute`, `Executed`, `Executed_Death`입니다. `Turn_45L`·`45R`·`90L`·`90R`은 강제 루트 고정 상태입니다.

## 처형·기상 자세

- `Execute`는 크게 들어 올린 검을 아래로 내리고 복귀합니다. `Execute_B`·`Execute_B2`는 검을 몸 앞 아래로 세워 누르는 자세가 중심이며, 팔과 몸의 회수 동작이 다릅니다.
- `Executed_B`와 `Executed_B_2`는 마지막에 눕는 방향이 서로 다릅니다. 이름에 Death가 없더라도 끝 자세가 서 있는 것은 아닙니다.
- `Getup1` 계열은 엎드린 쪽으로 몸을 짚으며, `Getup2` 계열은 상체를 앉히며 일어납니다. 각각 Idle1·Idle2로 끝나는 두 벌이 있습니다.
- GIF에는 상대 캐릭터를 넣지 않았으므로, 처형을 함께 재생할 두 캐릭터의 시작 간격과 타이밍은 이 조사에서 확정하지 않습니다.

## 회전 시퀀스

- 45°·90° 제자리 회전 계열은 루트 본에도 이름과 같은 회전이 들어 있습니다. 표의 루트 회전은 오른쪽이 양수, 왼쪽이 음수입니다.
- `Sprint_Turn180` 네 변형은 루트 본의 누적 회전량이 0°입니다. 몸의 방향 전환 자세와 루트의 회전 데이터는 다르므로, 이름만 보고 캐릭터 액터가 180° 돌아간다고 가정하지 않습니다.

## 측정 방법

측정 단위·좌표·샘플링과 설정 표기는 [시퀀스 측정 기준](<에셋 조사 개요.md#시퀀스-측정-기준>)을 따릅니다.

## 주요 공격의 자세

아래는 캡처한 포즈를 보고 구분한 동작 설명입니다. 공격 판정의 종류·횟수·시각을 뜻하지 않습니다. 시퀀스별 길이와 이동량은 아래 전체 표를 따릅니다.

| 모션 | 화면에서 구분되는 동작 |
|---|---|
| `Attack_Combo1` | 한 발 내딛으며 세운 검을 아래로 베고 낮은 자세로 마무리 |
| `Attack_Combo2` | 낮아진 검을 몸 앞에서 위로 걷어 올리는 동작 |
| `Attack_Combo3` | 몸통을 돌리며 검을 크게 휘두르고, 양팔이 벌어진 자세를 거쳐 복귀 |
| `Attack_Combo4` | 발을 크게 바꾸며 몸을 돌리고 검을 머리 위로 넘기는 큰 공격 동작 |
| `Attack_ComboB1` | 한 발 내딛으며 검과 오른팔을 몸 앞쪽으로 뻗는 공격 |
| `Attack_ComboB2` | 검을 몸통 앞쪽에서 가로질러 움직인 뒤 다시 세우는 공격 |
| `Attack_ComboB3` | 낮은 검을 머리 위로 들어 올리고 오른팔을 바깥으로 펼치는 공격 |
| `Attack_ComboB4` | 머리 위 검을 아래로 휘두른 뒤 몸 반대편 위로 회수 |
| `Attack_ComboB5` | 몸을 깊게 기울이고 크게 회전하며 검을 위로 넘기는 마무리 |
| `Attack_Dodge` | 몸을 앞으로 낮추면서 검을 휘두르고 앞으로 뻗는 자세로 마무리 |
| `Attack_Dodge_to_Combo1` | 검을 들어 `Attack_Combo1`과 유사한 내려베기로 이어지는 동작 |
| `Attack_Evade` | 웅크린 자세에서 앞으로 뻗어 베고 옆으로 넓게 선 자세로 회수 |
| `Attack_Heavy1` | 검을 뒤로 빼 둔 자세에서 크게 전진하며 휘두르고 몸을 돌려 회수 |
| `Attack_Heavy1_Start` | 뒤로 물러나 무릎을 굽히고 검을 머리 위에 세우는 준비 |
| `Attack_Heavy_Mid` | 검을 머리 위에 세운 준비 자세 유지. Turn 변형은 이 자세에서 방향 전환 |
| `Attack_Heavy2` | 몸을 돌려 검을 크게 들고 앞쪽 아래로 내리는 후속 공격 |
| `Attack_HeavyB` | 몸을 낮추고 검을 아래에서 위로 크게 휘두르는 공격 |
| `Attack_Sprint` | 달려들며 검을 뒤로 뺐다가 앞쪽 아래로 베고 낮은 자세로 정지 |
| `Attack_Airborne` | 무릎을 접은 공중 자세에서 검을 휘두르고 착지 준비 자세로 회수 |
| `Attack_Airborne_Landing` | 검을 뒤로 든 낮은 자세에서 무릎을 펴며 복귀하는 착지 동작 |

## 공격 (27)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Attack_Airborne](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Airborne.gif>) | 0.717 | 43 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Airborne_End](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Airborne_End.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Airborne_Landing](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Airborne_Landing.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Airborne_Mid](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Airborne_Mid.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Airborne_Start](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Airborne_Start.gif>) | 0.317 | 19 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Combo1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Combo1.gif>) | 0.817 | 49 | 0.69 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Combo2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Combo2.gif>) | 0.817 | 49 | 1.29 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Combo3](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Combo3.gif>) | 1.000 | 60 | 1.89 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Combo4](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Combo4.gif>) | 1.017 | 61 | 2.15 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_ComboB1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_ComboB1.gif>) | 0.983 | 59 | 0.69 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_ComboB2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_ComboB2.gif>) | 0.983 | 59 | 1.52 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_ComboB3](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_ComboB3.gif>) | 1.000 | 60 | 0.97 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_ComboB4](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_ComboB4.gif>) | 1.150 | 69 | 1.42 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_ComboB5](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_ComboB5.gif>) | 1.150 | 69 | 1.24 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Dodge](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Dodge.gif>) | 0.950 | 57 | 2.33 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Dodge_to_Combo1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Dodge_to_Combo1.gif>) | 0.817 | 49 | 1.29 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Evade](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Evade.gif>) | 0.983 | 59 | 0.75 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Heavy1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Heavy1.gif>) | 0.750 | 45 | 2.89 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Heavy1_Start](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Heavy1_Start.gif>) | 0.817 | 49 | -0.26 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Heavy2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Heavy2.gif>) | 0.817 | 49 | 0.95 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_HeavyB](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_HeavyB.gif>) | 1.150 | 69 | 2.24 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Heavy_Mid](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Heavy_Mid.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Attack_Heavy_Mid_Turn45L](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Heavy_Mid_Turn45L.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | -45.0 | — |
| [Attack_Heavy_Mid_Turn45R](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Heavy_Mid_Turn45R.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 45.0 | — |
| [Attack_Heavy_Mid_Turn90L](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Heavy_Mid_Turn90L.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | -90.0 | — |
| [Attack_Heavy_Mid_Turn90R](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Heavy_Mid_Turn90R.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 90.0 | — |
| [Attack_Sprint](<GIF/Warrior/ARPG_Warrior_Anim_UE_Attack_Sprint.gif>) | 0.883 | 53 | 2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 처형·처형 피격 (7)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Execute](<GIF/Warrior/ARPG_Warrior_Anim_UE_Execute.gif>) | 1.233 | 74 | 0.60 | 0.00 | 0.00 | 0.00 | 0.0 | RM |
| [Execute_B](<GIF/Warrior/ARPG_Warrior_Anim_UE_Execute_B.gif>) | 1.467 | 88 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Execute_B2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Execute_B2.gif>) | 1.467 | 88 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Executed](<GIF/Warrior/ARPG_Warrior_Anim_UE_Executed.gif>) | 1.617 | 97 | -2.07 | 0.00 | 0.00 | 0.00 | 0.0 | RM |
| [Executed_B](<GIF/Warrior/ARPG_Warrior_Anim_UE_Executed_B.gif>) | 1.900 | 114 | -0.76 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Executed_B_2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Executed_B_2.gif>) | 1.900 | 114 | 0.89 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Executed_Death](<GIF/Warrior/ARPG_Warrior_Anim_UE_Executed_Death.gif>) | 1.383 | 83 | -2.02 | 0.00 | 0.00 | 0.00 | 0.0 | RM |

## 회피 (9)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Dodge](<GIF/Warrior/ARPG_Warrior_Anim_UE_Dodge.gif>) | 0.700 | 42 | -2.97 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Backward](<GIF/Warrior/ARPG_Warrior_Anim_UE_Evade_Backward.gif>) | 0.900 | 54 | -3.96 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Backward_Left](<GIF/Warrior/ARPG_Warrior_Anim_UE_Evade_Backward_Left.gif>) | 0.900 | 54 | -2.97 | -2.97 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Backward_Right](<GIF/Warrior/ARPG_Warrior_Anim_UE_Evade_Backward_Right.gif>) | 0.900 | 54 | -2.97 | 2.97 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Forward](<GIF/Warrior/ARPG_Warrior_Anim_UE_Evade_Forward.gif>) | 0.900 | 54 | 3.96 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Forward_Left](<GIF/Warrior/ARPG_Warrior_Anim_UE_Evade_Forward_Left.gif>) | 0.900 | 54 | 2.97 | -2.97 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Forward_Right](<GIF/Warrior/ARPG_Warrior_Anim_UE_Evade_Forward_Right.gif>) | 0.900 | 54 | 2.97 | 2.97 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Left](<GIF/Warrior/ARPG_Warrior_Anim_UE_Evade_Left.gif>) | 0.900 | 54 | 0.00 | -3.96 | 0.00 | 0.00 | 0.0 | — |
| [Evade_Right](<GIF/Warrior/ARPG_Warrior_Anim_UE_Evade_Right.gif>) | 0.900 | 54 | 0.00 | 3.96 | 0.00 | 0.00 | 0.0 | — |

## 가드·패리 (11)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Guard](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Break](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Break.gif>) | 0.817 | 49 | -0.83 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Hit1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Hit1.gif>) | 0.567 | 34 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Hit2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Hit2.gif>) | 0.567 | 34 | -0.50 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Hit3](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Hit3.gif>) | 0.567 | 34 | -0.50 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Start](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Start.gif>) | 0.117 | 7 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Turn90L](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Turn90L.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | -90.0 | — |
| [Guard_Turn90R](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Turn90R.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 90.0 | — |
| [Guard_Turn_45L](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Turn_45L.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | -45.0 | — |
| [Guard_Turn_45R](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Turn_45R.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 45.0 | — |
| [Parry](<GIF/Warrior/ARPG_Warrior_Anim_UE_Parry.gif>) | 0.500 | 30 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 가드 이동 (32)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Guard_Run_Backward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Backward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Backward_Left_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Backward_Left_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Backward_Left_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Backward_Left_Rootmotion.gif>) | 0.733 | 44 | -1.93 | -1.93 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Backward_Right_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Backward_Right_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Backward_Right_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Backward_Right_Rootmotion.gif>) | 0.733 | 44 | -1.93 | 1.93 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Backward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Backward_Rootmotion.gif>) | 0.733 | 44 | -2.73 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Forward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Forward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Forward_Left_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Forward_Left_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Forward_Left_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Forward_Left_Rootmotion.gif>) | 0.733 | 44 | 1.93 | -1.93 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Forward_Right_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Forward_Right_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Forward_Right_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Forward_Right_Rootmotion.gif>) | 0.733 | 44 | 1.93 | 1.93 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Forward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Forward_Rootmotion.gif>) | 0.733 | 44 | 2.73 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Leftward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Leftward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Leftward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Leftward_Rootmotion.gif>) | 0.733 | 44 | 0.00 | -2.73 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Rightward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Rightward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Run_Rightward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Run_Rightward_Rootmotion.gif>) | 0.733 | 44 | 0.00 | 2.73 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Backward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Backward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Backward_Left_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Backward_Left_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Backward_Left_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Backward_Left_Rootmotion.gif>) | 0.983 | 59 | -1.75 | -1.75 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Backward_Right_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Backward_Right_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Backward_Right_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Backward_Right_Rootmotion.gif>) | 0.983 | 59 | -1.75 | 1.75 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Backward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Backward_Rootmotion.gif>) | 0.983 | 59 | -2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Forward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Forward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Forward_Left_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Forward_Left_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Forward_Left_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Forward_Left_Rootmotion.gif>) | 0.983 | 59 | 1.75 | -1.75 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Forward_Right_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Forward_Right_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Forward_Right_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Forward_Right_Rootmotion.gif>) | 0.983 | 59 | 1.75 | 1.75 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Forward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Forward_Rootmotion.gif>) | 0.983 | 59 | 2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Leftward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Leftward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Leftward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Leftward_Rootmotion.gif>) | 0.983 | 59 | 0.00 | -2.48 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Rightward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Rightward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Guard_Walk_Rightward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Guard_Walk_Rightward_Rootmotion.gif>) | 0.983 | 59 | 0.00 | 2.48 | 0.00 | 0.00 | 0.0 | — |

## 피격·기상·기절 (36)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Collide](<GIF/Warrior/ARPG_Warrior_Anim_UE_Collide.gif>) | 1.067 | 64 | -0.66 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Collide2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Collide2.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Collide_To_Stun](<GIF/Warrior/ARPG_Warrior_Anim_UE_Collide_To_Stun.gif>) | 0.983 | 59 | -0.54 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Getup1_Idle1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Getup1_Idle1.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Getup1_Idle2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Getup1_Idle2.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Getup2_Idle1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Getup2_Idle1.gif>) | 0.650 | 39 | 0.20 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Getup2_Idle2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Getup2_Idle2.gif>) | 0.650 | 39 | 0.20 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit1.gif>) | 0.817 | 49 | -0.50 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit10](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit10.gif>) | 0.817 | 49 | 0.00 | -0.50 | 0.00 | 0.00 | 0.0 | — |
| [Hit11](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit11.gif>) | 1.150 | 69 | 0.99 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit12](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit12.gif>) | 1.150 | 69 | 0.00 | 0.99 | 0.00 | 0.00 | 0.0 | — |
| [Hit13](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit13.gif>) | 1.150 | 69 | 0.00 | -0.99 | 0.00 | 0.00 | 0.0 | — |
| [Hit14](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit14.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit2.gif>) | 0.817 | 49 | -0.74 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit3](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit3.gif>) | 0.817 | 49 | -0.99 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit4](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit4.gif>) | 0.817 | 49 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit5](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit5.gif>) | 0.817 | 49 | 0.32 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit6](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit6.gif>) | 1.150 | 69 | -0.99 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit7](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit7.gif>) | 1.150 | 69 | -0.99 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit8](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit8.gif>) | 1.150 | 69 | -0.99 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit9](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit9.gif>) | 0.817 | 49 | 0.00 | 0.50 | 0.00 | 0.00 | 0.0 | — |
| [Hit_Air](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_Air.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_Air_B](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_Air_B.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_Air_B_Down](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_Air_B_Down.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_Air_B_Up](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_Air_B_Up.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_Air_Down](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_Air_Down.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_Air_Up](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_Air_Up.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_Airborne](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_Airborne.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_knockdown](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_knockdown.gif>) | 1.317 | 79 | -4.06 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_knockdown2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_knockdown2.gif>) | 0.733 | 44 | -2.13 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_knockdown2_End](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_knockdown2_End.gif>) | 0.033 | 1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_knockdown3](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_knockdown3.gif>) | 0.733 | 44 | -3.14 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_knockdown3_End](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_knockdown3_End.gif>) | 0.033 | 1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_knockdown_B](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_knockdown_B.gif>) | 0.733 | 44 | 2.64 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Hit_knockdown_Death](<GIF/Warrior/ARPG_Warrior_Anim_UE_Hit_knockdown_Death.gif>) | 1.567 | 94 | -3.11 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Stun](<GIF/Warrior/ARPG_Warrior_Anim_UE_Stun.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 사망 (5)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Death](<GIF/Warrior/ARPG_Warrior_Anim_UE_Death.gif>) | 0.950 | 57 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Death2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Death2.gif>) | 1.233 | 74 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Death_Airborne_End](<GIF/Warrior/ARPG_Warrior_Anim_UE_Death_Airborne_End.gif>) | 0.233 | 14 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Death_Airborne_Mid](<GIF/Warrior/ARPG_Warrior_Anim_UE_Death_Airborne_Mid.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Death_Airborne_Start](<GIF/Warrior/ARPG_Warrior_Anim_UE_Death_Airborne_Start.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 점프·공중·착지 (16)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Airborne](<GIF/Warrior/ARPG_Warrior_Anim_UE_Airborne.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Jump](<GIF/Warrior/ARPG_Warrior_Anim_UE_Jump.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Jump_Backward](<GIF/Warrior/ARPG_Warrior_Anim_UE_Jump_Backward.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Jump_Backward_Left](<GIF/Warrior/ARPG_Warrior_Anim_UE_Jump_Backward_Left.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Jump_Backward_Right](<GIF/Warrior/ARPG_Warrior_Anim_UE_Jump_Backward_Right.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Jump_Forward](<GIF/Warrior/ARPG_Warrior_Anim_UE_Jump_Forward.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Jump_Forward_Left](<GIF/Warrior/ARPG_Warrior_Anim_UE_Jump_Forward_Left.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Jump_Forward_Right](<GIF/Warrior/ARPG_Warrior_Anim_UE_Jump_Forward_Right.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Jump_Leftward](<GIF/Warrior/ARPG_Warrior_Anim_UE_Jump_Leftward.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Jump_Rightward](<GIF/Warrior/ARPG_Warrior_Anim_UE_Jump_Rightward.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Landing](<GIF/Warrior/ARPG_Warrior_Anim_UE_Landing.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Landing2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Landing2.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Landing2_no_weapon](<GIF/Warrior/ARPG_Warrior_Anim_UE_Landing2_no_weapon.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Landing3](<GIF/Warrior/ARPG_Warrior_Anim_UE_Landing3.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Landing4](<GIF/Warrior/ARPG_Warrior_Anim_UE_Landing4.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Landing_no_weapon](<GIF/Warrior/ARPG_Warrior_Anim_UE_Landing_no_weapon.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 대기·전환 (14)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Buff](<GIF/Warrior/ARPG_Warrior_Anim_UE_Buff.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Equip_Idle1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Equip_Idle1.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Equip_Idle2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Equip_Idle2.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Idle1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Idle1.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Idle1_to_Idle2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Idle1_to_Idle2.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Idle2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Idle2.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Idle2_to_Idle1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Idle2_to_Idle1.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Idle_no_weapon](<GIF/Warrior/ARPG_Warrior_Anim_UE_Idle_no_weapon.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Turn_45L](<GIF/Warrior/ARPG_Warrior_Anim_UE_Turn_45L.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | -45.0 | 고정 |
| [Turn_45R](<GIF/Warrior/ARPG_Warrior_Anim_UE_Turn_45R.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 45.0 | 고정 |
| [Turn_90L](<GIF/Warrior/ARPG_Warrior_Anim_UE_Turn_90L.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | -90.0 | 고정 |
| [Turn_90R](<GIF/Warrior/ARPG_Warrior_Anim_UE_Turn_90R.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 90.0 | 고정 |
| [Unequip_Idle1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Unequip_Idle1.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Unequip_Idle2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Unequip_Idle2.gif>) | 0.500 | 30 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 이동 (56)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Brake](<GIF/Warrior/ARPG_Warrior_Anim_UE_Brake.gif>) | 0.417 | 25 | 0.39 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Brake2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Brake2.gif>) | 0.417 | 25 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Brake2_no_weapon](<GIF/Warrior/ARPG_Warrior_Anim_UE_Brake2_no_weapon.gif>) | 0.417 | 25 | 0.33 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Brake_no_weapon](<GIF/Warrior/ARPG_Warrior_Anim_UE_Brake_no_weapon.gif>) | 0.417 | 25 | 0.33 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Backward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Backward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Backward_Left_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Backward_Left_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Backward_Left_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Backward_Left_Rootmotion.gif>) | 0.733 | 44 | -1.93 | -1.93 | 0.00 | 0.00 | 0.0 | — |
| [Run_Backward_Right_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Backward_Right_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Backward_Right_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Backward_Right_Rootmotion.gif>) | 0.733 | 44 | -1.93 | 1.93 | 0.00 | 0.00 | 0.0 | — |
| [Run_Backward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Backward_Rootmotion.gif>) | 0.733 | 44 | -2.73 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Forward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Forward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Forward_Left_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Forward_Left_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Forward_Left_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Forward_Left_Rootmotion.gif>) | 0.733 | 44 | 1.93 | -1.93 | 0.00 | 0.00 | 0.0 | — |
| [Run_Forward_Right_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Forward_Right_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Forward_Right_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Forward_Right_Rootmotion.gif>) | 0.733 | 44 | 1.93 | 1.93 | 0.00 | 0.00 | 0.0 | — |
| [Run_Forward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Forward_Rootmotion.gif>) | 0.733 | 44 | 2.73 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Leftward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Leftward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Leftward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Leftward_Rootmotion.gif>) | 0.733 | 44 | 0.00 | -2.73 | 0.00 | 0.00 | 0.0 | — |
| [Run_Rightward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Rightward_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_Rightward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_Rightward_Rootmotion.gif>) | 0.733 | 44 | 0.00 | 2.73 | 0.00 | 0.00 | 0.0 | — |
| [Run_no_weapon_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_no_weapon_IPC.gif>) | 0.733 | 44 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Run_no_weapon_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Run_no_weapon_Rootmotion.gif>) | 0.733 | 44 | 2.73 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_IPC.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_Rootmotion.gif>) | 0.650 | 39 | 2.97 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Start_L_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_Start_L_IPC.gif>) | 0.750 | 45 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Start_L_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_Start_L_Rootmotion.gif>) | 0.750 | 45 | 3.42 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Start_R_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_Start_R_IPC.gif>) | 0.750 | 45 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Start_R_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_Start_R_Rootmotion.gif>) | 0.750 | 45 | 3.42 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Start_no_weapon_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_Start_no_weapon_IPC.gif>) | 0.750 | 45 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Start_no_weapon_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_Start_no_weapon_Rootmotion.gif>) | 0.750 | 45 | 3.42 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Turn180_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_Turn180_IPC.gif>) | 0.550 | 33 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Turn180_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_Turn180_Rootmotion.gif>) | 0.550 | 33 | 2.41 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Turn180_no_weapon_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_Turn180_no_weapon_IPC.gif>) | 0.550 | 33 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_Turn180_no_weapon_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_Turn180_no_weapon_Rootmotion.gif>) | 0.550 | 33 | 2.41 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_no_weapon_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_no_weapon_IPC.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Sprint_no_weapon_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Sprint_no_weapon_Rootmotion.gif>) | 0.650 | 39 | 2.97 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk2_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk2_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk2_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk2_Rootmotion.gif>) | 0.983 | 59 | 2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Backward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Backward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Backward_Left_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Backward_Left_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Backward_Left_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Backward_Left_Rootmotion.gif>) | 0.983 | 59 | -1.75 | -1.75 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Backward_Right_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Backward_Right_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Backward_Right_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Backward_Right_Rootmotion.gif>) | 0.983 | 59 | -1.75 | 1.75 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Backward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Backward_Rootmotion.gif>) | 0.983 | 59 | -2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Forward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Forward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Forward_Left_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Forward_Left_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Forward_Left_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Forward_Left_Rootmotion.gif>) | 0.983 | 59 | 1.75 | -1.75 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Forward_Right_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Forward_Right_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Forward_Right_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Forward_Right_Rootmotion.gif>) | 0.983 | 59 | 1.75 | 1.75 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Forward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Forward_Rootmotion.gif>) | 0.983 | 59 | 2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Leftward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Leftward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Leftward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Leftward_Rootmotion.gif>) | 0.983 | 59 | 0.00 | -2.48 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Rightward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Rightward_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_Rightward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_Rightward_Rootmotion.gif>) | 0.983 | 59 | 0.00 | 2.48 | 0.00 | 0.00 | 0.0 | — |
| [Walk_no_weapon_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_no_weapon_IPC.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Walk_no_weapon_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Walk_no_weapon_Rootmotion.gif>) | 0.983 | 59 | 2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 숙이기 (46)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Crouch](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Backward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Backward_IPC.gif>) | 0.817 | 49 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Backward_Left_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Backward_Left_IPC.gif>) | 0.817 | 49 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Backward_Left_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Backward_Left_Rootmotion.gif>) | 0.817 | 49 | -1.75 | -1.75 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Backward_Right_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Backward_Right_IPC.gif>) | 0.817 | 49 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Backward_Right_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Backward_Right_Rootmotion.gif>) | 0.817 | 49 | -1.75 | 1.75 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Backward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Backward_Rootmotion.gif>) | 0.817 | 49 | -2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Forward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Forward_IPC.gif>) | 0.817 | 49 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Forward_Left_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Forward_Left_IPC.gif>) | 0.817 | 49 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Forward_Left_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Forward_Left_Rootmotion.gif>) | 0.817 | 49 | 1.75 | -1.75 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Forward_Right_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Forward_Right_IPC.gif>) | 0.817 | 49 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Forward_Right_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Forward_Right_Rootmotion.gif>) | 0.817 | 49 | 1.75 | 1.75 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Forward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Forward_Rootmotion.gif>) | 0.817 | 49 | 2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Leftward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Leftward_IPC.gif>) | 0.817 | 49 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Leftward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Leftward_Rootmotion.gif>) | 0.817 | 49 | 0.00 | -2.48 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Rightward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Rightward_IPC.gif>) | 0.817 | 49 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_Rightward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_Rightward_Rootmotion.gif>) | 0.817 | 49 | 0.00 | 2.48 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_no_weapon_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_no_weapon_IPC.gif>) | 0.817 | 49 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Run_no_weapon_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Run_no_weapon_Rootmotion.gif>) | 0.817 | 49 | 2.48 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Start](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Start.gif>) | 0.317 | 19 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Start_no_weapon](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Start_no_weapon.gif>) | 0.317 | 19 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_To_Idle1](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_To_Idle1.gif>) | 0.317 | 19 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_To_Idle2](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_To_Idle2.gif>) | 0.317 | 19 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Turn45L](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Turn45L.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | -45.0 | — |
| [Crouch_Turn45R](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Turn45R.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 45.0 | — |
| [Crouch_Turn90L](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Turn90L.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | -90.0 | — |
| [Crouch_Turn90R](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Turn90R.gif>) | 0.650 | 39 | 0.00 | 0.00 | 0.00 | 0.00 | 90.0 | — |
| [Crouch_Walk_Backward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Backward_IPC.gif>) | 1.150 | 69 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Backward_Left_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Backward_Left_IPC.gif>) | 1.150 | 69 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Backward_Left_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Backward_Left_Rootmotion.gif>) | 1.150 | 69 | -1.40 | -1.40 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Backward_Right_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Backward_Right_IPC.gif>) | 1.150 | 69 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Backward_Right_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Backward_Right_Rootmotion.gif>) | 1.150 | 69 | -1.40 | 1.40 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Backward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Backward_Rootmotion.gif>) | 1.150 | 69 | -1.98 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Forward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Forward_IPC.gif>) | 1.150 | 69 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Forward_Left_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Forward_Left_IPC.gif>) | 1.150 | 69 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Forward_Left_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Forward_Left_Rootmotion.gif>) | 1.150 | 69 | 1.40 | -1.40 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Forward_Right_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Forward_Right_IPC.gif>) | 1.150 | 69 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Forward_Right_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Forward_Right_Rootmotion.gif>) | 1.150 | 69 | 1.40 | 1.40 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Forward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Forward_Rootmotion.gif>) | 1.150 | 69 | 1.98 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Leftward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Leftward_IPC.gif>) | 1.150 | 69 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Leftward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Leftward_Rootmotion.gif>) | 1.150 | 69 | 0.00 | -1.98 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Rightward_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Rightward_IPC.gif>) | 1.150 | 69 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_Rightward_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_Rightward_Rootmotion.gif>) | 1.150 | 69 | 0.00 | 1.98 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_no_weapon_IPC](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_no_weapon_IPC.gif>) | 1.150 | 69 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_Walk_no_weapon_Rootmotion](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_Walk_no_weapon_Rootmotion.gif>) | 1.150 | 69 | 1.98 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Crouch_no_weapon](<GIF/Warrior/ARPG_Warrior_Anim_UE_Crouch_no_weapon.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |

## 오르기 (13)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [Climb_Down](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_Down.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Climb_Down_Fast](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_Down_Fast.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Climb_Down_Start](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_Down_Start.gif>) | 0.567 | 34 | -1.06 | 0.00 | -1.81 | 1.81 | 180.0 | — |
| [Climb_Down_To_Idle](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_Down_To_Idle.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Climb_Down_To_Idle_L](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_Down_To_Idle_L.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Climb_Down_To_Idle_R](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_Down_To_Idle_R.gif>) | 0.400 | 24 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Climb_L](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_L.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Climb_L_Hit](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_L_Hit.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Climb_R](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_R.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Climb_R_Hit](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_R_Hit.gif>) | 0.483 | 29 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Climb_Up](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_Up.gif>) | 0.983 | 59 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Climb_Up_Start](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_Up_Start.gif>) | 0.150 | 9 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
| [Climb_Up_To_Idle](<GIF/Warrior/ARPG_Warrior_Anim_UE_Climb_Up_To_Idle.gif>) | 0.567 | 34 | 1.06 | 0.00 | 1.81 | 1.81 | 0.0 | — |

## 기준 자세 (1)

| 모션 / GIF | 길이(초) | 프레임 | 전진(m) | 오른쪽(m) | 높이(m) | 루트 높이 범위(m) | 루트 회전(°) | 설정 |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| [A_Bindpose](<GIF/Warrior/ARPG_Warrior_Anim_UE_A_Bindpose.gif>) | 0.033 | 1 | 0.00 | 0.00 | 0.00 | 0.00 | 0.0 | — |
