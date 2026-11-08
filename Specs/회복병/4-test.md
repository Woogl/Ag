# 회복병 검증 결과

## 1. 테스트 시나리오

- 시나리오 실행 명령: `python -I Scripts/ScenarioTest/run.py <시나리오.py>`
- 시나리오 파일은 임시 파일이라 저장소에 두지 않습니다. `runner.py`의 `KEYS`에 `Input.Potion`(R)을 더했습니다.

| 시나리오 | 맵 | 확인한 것 |
| - | - | - |
| 수치·시점 | `LV_BossStage` | HP 가득 찬 채 마시면 HP 그대로 1회 소비. 350·2회 → 삼키는 프레임에 750·1회. 900·1회 → 1000·0회. 동작 1.2초, 삼키는 시점 0.8초. 빈 병의 R은 시작하지 않음. HUD 3/3, 2/3, 0/3과 빈 병의 흐림 |
| 이동 | `LV_BossStage` | 걷는 속도 유지, 마시는 중 90° 방향 전환, 멈춤, 한 번만 회복. 달리다 R을 누르면 그 프레임에 달리기가 끝나 평소 속도로 걷고, Shift를 누르고 있어도 다시 달리거나 회피하지 않음 |
| 입력·끊김 | `LV_BossStage` | 삼키기 전·후 움찔, R 길게 누르기, 공중·공격·회피·가드 중 R, 이른 공격 입력 버림, 끝나기 0.2초 전 공격의 선입력, 락온, 사망, 병이 마시는 동안 한 개 보이고 끊기면 사라짐, 회복 효과가 삼킨 순간 한 번만 재생되고 삼키기 전 끊기거나 죽으면 재생되지 않음 |
| 실제 공격 | `LV_BossStage` | 보스 DoubleStrike의 첫 타(대미지 133). 삼키기 전: 500 → 367, 횟수 그대로. 삼킨 뒤: 900 → 767, 횟수 유지 |
| 던전 | `LV_DuelCorridor` | 시작 3/3, 두 번 마신 뒤 보스 앞으로 옮겨도 1회 그대로. `RestartLevel`(Restart 버튼이 부르는 것과 같은 함수) 뒤 값과 HUD가 3/3 |
| 시뮬레이션 룸 | `LV_SimulationRoom?Enemies=Boss` | 수치·시점 시나리오와 같은 항목 |
| 상체 재생 | `LV_BossStage` | `UpperBody` 슬롯에서 왼손이 머리 가까이 올라가고, 서 있으면 발이 그대로, 걸으면 발이 이동 모션대로 움직임 |
| 화면 촬영 | `LV_BossStage` | 정지·걷기에서 삼키는 순간 병 입구가 입에 닿음, 회복 효과, 1080p HUD 배치와 조작 안내의 R 줄 |

## 2. 결과

- 빌드: `AgEditor Win64 Development` 성공
- 판정: 위 시나리오 모두 통과
- 로그: 테스트 창의 `LogAg: Error` 없음
- 사람이 확인할 것: 손맛과 모습(모션, 회복 효과의 세기), 타이틀의 조작 안내 팝업, 실제 Restart·Return to Title 버튼

## 3. 회귀 확인

- 확인한 기존 기능과 결과: `player_combo`, `player_dodge`, `player_jump`, `player_guard_reaction`, `player_skill`, `player_execution`, `boss_patterns`, `groggy`, `stage_boss_clear` 통과
