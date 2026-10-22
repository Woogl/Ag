# VFX·사운드 리소스 조사

> 갱신일 2026-10-23 · 출처: 프로젝트·엔진 콘텐츠 폴더, 사운드 길이는 SoundWave 에셋에 든 WAV 헤더

프로젝트에서 쓸 수 있는 VFX와 사운드의 출처, 지금 쓰는 에셋을 고른 근거를 정리합니다. 어떤 상황에 무엇을 재생하는지는 [아트 표현 사양](<아트 표현 사양.md>)의 '효과와 UI 리소스', 에셋을 지정하는 곳은 [데이터 위치표](<../Program/데이터 위치표.md>)를 따릅니다.

## 출처

| 출처 | VFX | 사운드 |
| - | - | - |
| 템플릿 에셋 | Niagara 시스템 3개 ('VFX') | 없음 |
| Fab 애니메이션 팩 5종 (ARPG 4종, Dodge and Evade) | 없음 | 없음 |
| 엔진 콘텐츠 | 없음 | 에디터 조작음뿐 ('엔진 사운드') |
| 엔진 플러그인 NetworkPredictionExtras | 없음 | 게임용 착지·발사·충돌·통증 소리 ('NetworkPredictionExtras 사운드') |
| 직접 제작 | `NS_Telegraph`, `NS_WeaponTrail` ([에셋 목록](<../Program/에셋 목록.md>) 참고) | 없음 |

Starter Content는 설치되어 있지 않습니다.

## VFX

| 에셋 | 위치 | 지금 쓰는 곳 |
| - | - | - |
| `NS_Damage` | `/Game/Variant_Combat/VFX` | 피격 VFX(피격·가드), 패리, 궁극기 마무리 |
| `NS_Jump_Trail` | `/Game/Variant_Platforming/VFX` | 없음. 무기 궤적(`NS_WeaponTrail`)은 이 리본을 복제해 고친 것입니다. |
| `NS_JumpPad` | `/Game/LevelPrototyping/Interactable/JumpPad/Assets` | 없음 |

- 타격에 쓸 만한 템플릿 VFX가 `NS_Damage` 하나뿐이라, 피격·패리·궁극기 마무리는 같은 VFX를 크기로 구분합니다.
- 검기와 궁극기 베기 선은 Niagara가 아니라 템플릿의 프로토타입 메시와 `M_SimpleGlow` 계열 머티리얼을 조합한 것입니다.

## 엔진 사운드

| 폴더 | 에셋 수 | 내용 |
| - | - | - |
| `/Engine/VREditor/Sounds` | 78 (SoundCue 포함) | VR 에디터의 클릭·열기·닫기·잡기·텔레포트 소리 |
| `/Engine/EngineSounds` | 44 | 사운드 클래스·리버브·서브믹스 설정이 대부분이고, 소리는 사인파와 화이트 노이즈뿐 |
| `/Engine/EditorSounds` | 17 | PIE 시작·종료, 컴파일 성공·실패 알림 |
| `/Engine/Tutorial/Audio` | 4 | 튜토리얼 음성 |

지금 쓰는 엔진 사운드는 모두 `/Engine/VREditor/Sounds`에 있습니다.

| 에셋 | 길이 | 지금 쓰는 곳 |
| - | - | - |
| `VR_negative` | 1.23초 | 가드 불가 예고 |
| `UI/Floating_UI_Open-002` | 1.46초 | 패리 예고 |
| `VR_click2` | 0.38초 | 패리 (금속음에 겹침) |
| `UI/Click_on_Button`, `UI/Selection_Changes` | | 버튼 실행, 버튼 위에 마우스를 올림 (`ButtonStyle_Default`) |

- 엔진 사운드는 UI 조작음 성격이라 타격음으로 쓰지 않습니다. 예고와 버튼처럼 알림에 가까운 소리에만 씁니다.
- 패키징에서 `-SkipCookingEditorContent`를 쓰면 `/Engine/VREditor`가 쿡에서 빠져 이 사운드들도 빠집니다.

## NetworkPredictionExtras 사운드

### 가져오는 방법

플러그인은 켜지 않고 사운드만 가져옵니다. SoundWave 에셋(`.uasset`) 안에 원본 WAV가 그대로 들어 있으므로, 파일에서 `RIFF`를 찾아 그 뒤의 크기 값 + 8바이트만큼 잘라 `.wav`로 저장한 뒤 `/Game/Audio`로 가져옵니다. SoundCue는 WAV가 없으므로 그것이 가리키는 SoundWave를 가져옵니다.

### 목록

위치는 엔진의 `Plugins/Runtime/NetworkPredictionExtras/Content/Audio`입니다.

| 에셋 | 길이 | 채널 | 지금 쓰는 곳 |
| - | - | - | - |
| `Footstep_Rock_Land01` | 0.30초 | 모노 | 피격 |
| `Footstep_Metal_Catwalk_Land01` | 0.50초 | 모노 | 가드 |
| `Footstep_Metal_Catwalk_Land03` | 0.49초 | 모노 | |
| `Footstep_Metal_Catwalk_Land05` | 0.51초 | 모노 | 패리 |
| `Pain/HoffmanEfforts_PainMedium_01` | 0.67초 | 모노 | |
| `Pain/HoffmanEfforts_PainMedium_02` | 0.72초 | 모노 | |
| `Pain/HoffmanEfforts_PainMedium_03` | 0.58초 | 모노 | |
| `AssaultHeavy_Fire_LowAmmo_01` | 0.56초 | 스테레오 | |
| `GrenadeLauncher/GrenadeLauncher_Fire_INT_JScottV2_01` | 0.69초 | 스테레오 | |
| `GrenadeLauncher/GrenadeLauncher_Fire_INT_JScottV3_01` | 0.59초 | 스테레오 | |
| `GrenadeLauncher/GrenadeLauncher_Fire_INT_JScottV5_01` | 0.65초 | 스테레오 | |
| `MortarFirePlayer01`~`03` | 3.00~3.04초 | 스테레오 | |
| `SciFiLaserShot_14`, `_16`, `_17` | 1.06~1.33초 | 스테레오 | |
| `Smash_CarImpactBridge` | 2.41초 | 모노 | |
| `Smash_MetalTowerFall_Spark` | 1.60초 | 모노 | |
| `steam_burst04` | 1.24초 | 모노 | |
| `RespawnPad/VendingMachine_Fail` | 0.96초 | 스테레오 | |
| `SFX_Missle_Ignite_01` | 2.09초 | 스테레오 | |
| `SFX_Flame_01` | 3.51초 | 스테레오 | |
| `Brumak_RocketInAir02` | 6.39초 | 모노 | |
| 반복 재생용: `SFX_Missle_Loop_01`, `Steam_Loop01`~`03`, `Tank_Loop1`~`3` | 0.61~2.97초 | | |
| SoundCue: `SciFiLaserShot_Cue`, `SFX_PhysicsAttract_Cue`, `SFX_Ship_HitGround_01_Cue` | | | |

- 피격·가드·패리는 짧게 끊기는 착지음에서 골랐습니다. 피격은 가장 짧은 바위 착지음, 가드는 금속 착지음이고, 패리는 다른 금속 착지음에 `VR_click2`를 겹쳐 가드와 구분합니다.

## 갱신 방법

에셋 수와 목록은 위 폴더의 `.uasset` 파일로 다시 셉니다. 사운드 길이는 에셋 안 WAV의 `fmt `·`data` 청크(데이터 크기 ÷ 초당 바이트 수)로 계산합니다. 에셋이 바뀌면 이 문서를 고치고 머리의 갱신일을 바꿉니다.
