# 락온 대상 전환 검증 결과

## 1. 테스트 시나리오

시나리오는 회귀용으로 `Saved/ScenarioTest/Scenarios`에 둡니다. 샌드백 셋은 시뮬레이션 룸 배치 자리 2·3·4에 놓은 뒤 시나리오가 플레이어 앞으로 옮깁니다. 마우스 튕기기는 확인용 입력 `DevLookInput`(가로값 × 2.5 = 자유 시점 프레임당 각도)으로 넣고, 락온 대상은 락온 마커가 보이는 샌드백으로 읽습니다.

```
python -I Scripts/ScenarioTest/run.py Saved/ScenarioTest/Scenarios/lockon_switch.py --map "/Game/Maps/LV_SimulationRoom?Enemies=,Sandbag,Sandbag,Sandbag"
python -I Scripts/ScenarioTest/run.py Saved/ScenarioTest/Scenarios/lockon_flick_shape.py --map "/Game/Maps/LV_SimulationRoom?Enemies=,Sandbag,Sandbag,Sandbag"
python -I Scripts/ScenarioTest/run.py Saved/ScenarioTest/Scenarios/lockon_death.py --map "/Game/Maps/LV_SimulationRoom?Enemies=,Sandbag,Sandbag,Sandbag"
python -I Scripts/ScenarioTest/run.py Saved/ScenarioTest/Scenarios/lockon_cinematic.py --map "/Game/Maps/LV_SimulationRoom?Enemies=,Sandbag,Sandbag,Sandbag"
python -I Scripts/ScenarioTest/run.py Saved/ScenarioTest/Scenarios/lockon_boss.py
```

| 시나리오 | 확인하는 완료 조건 |
| - | - |
| `lockon_switch.py` | 좌우 전환과 카메라 회전, 오른쪽 끝에서 멈춤, 0.4초 동안 세게 튕겨도 한 칸, 천천히·세로 이동 무시, 기본 공격·가드·움찔 중 전환과 다음 타의 방향, 20m 밖·화면 밖 적 제외, 다른 적이 있어도 30m 밖이면 해제 |
| `lockon_flick_shape.py` | 빠르게 → 느리게 → 빠르게 출렁이는 튕김(최근 0.1초 합이 30° 아래로 내려갔다가 다시 넘음)도 한 칸, 오른쪽으로 튕긴 뒤 멈추지 않고 왼쪽으로 튕기면 되돌아감 |
| `lockon_death.py` | 처치하면 화면 중앙에 가까운 적으로 옮김과 카메라 회전, 차례로 처치, 남은 적이 없으면 해제 |
| `lockon_cinematic.py` | 처형 중 튕겨도 그대로, 처형으로 처치하면 처형 카메라 밖의 적이라도 락온 카메라 화면 기준으로 옮김, 처형 카메라가 끝난 뒤 새 대상을 향함, 궁극기 컷신 중 튕겨도 그대로 |
| `lockon_boss.py` | 보스만 있을 때 튕겨도 보스 락온 유지, 보스 사망 뒤 해제 |

## 2. 결과

- 빌드: 임시 복사본 컴파일 성공(경고 없음), 공유 트리 `AgEditor Win64 Development` 빌드 성공 뒤 에디터 재시작. 에디터와 테스트 창 로그에 `LogAg: Error` 없음.
- 판정: 지금 코드(판정 각도 30°, 다음 전환 5°, 플레이어가 락온 어빌리티의 대상을 직접 바꿈)로 5개 모두 통과(최저 44fps).

| 시나리오 | 결과 | 주요 값 |
| - | - | - |
| `lockon_switch.py` | 통과 (`slot1_20261107-050830.json`) | 전환 뒤 카메라 Yaw 오차 0.0~0.2°, 공격 중 바꾼 뒤 다음 타(10.73초) 몸 방향 0.0° |
| `lockon_flick_shape.py` | 통과 (`slot1_20261107-050750.json`) | 출렁이는 튕김 뒤 L → C(R 없음), 반대 튕김 C → R → C |
| `lockon_death.py` | 통과 (`slot1_20261107-050923.json`) | L 사망 2.53초 → C 2.56초, 1.1초 뒤 카메라 0.1°, 그동안 락온 유지 |
| `lockon_cinematic.py` | 통과 (`slot1_20261107-051004.json`) | C 처형 사망 2.81초 → R 2.85초, 그 순간 R은 처형 카메라 화면 밖, 처형 카메라 뒤 카메라 0.1°, 컷신 6.51~8.44초 중 6.73초에 튕겨도 카메라 최대 0.2° |
| `lockon_boss.py` | 통과 (`slot1_20261107-051051.json`) | |

- 플레이 확인에서 바꾼 것: 판정 각도 15° → 30°(너무 민감), 다시 전환을 받는 기준을 판정 각도 아래 → 5° 아래 또는 반대 방향(한 번 튕겼는데 두 칸 넘어감), 전환 요청을 게임플레이 이벤트에서 직접 호출로.
- 처음 실행에서 고친 시나리오 문제(코드 변경 없음):
  - `lockon_switch.py` S12~S14 실패: 기본 공격 5타와 움찔로 플레이어가 샌드백 쪽으로 움직여, 시작 위치 기준 21.8m에 둔 R이 실제로는 20m 안이었습니다. 이 구간 전에 플레이어를 시작 위치로 되돌리게 고쳤습니다.
  - `lockon_death.py` S2 실패: 판정 코드가 3.6초 행을 3.6초까지의 범위에서만 찾아 카메라 각도를 읽지 못했습니다(동작은 통과 기준 안).
  - `lockon_cinematic.py` 참고 항목: 처형 카메라 화면 확인의 투영 호출 반환 형식을 고쳤습니다(판정 항목 아님).

## 3. 회귀 확인

판정 각도 15°, 이벤트 방식 빌드에서 돌렸습니다. 그 뒤 바뀐 것은 튕김 판정과 전환 호출뿐이라 아래 시나리오가 보는 동작(락온 켜고 끄기, 처형, 적 락온)은 바뀌지 않았습니다.

| 시나리오 | 결과 |
| - | - |
| `look_at_sandbags.py` (`Enemies=Sandbag,Sandbag`) | 통과. 락온 중 락온 대상을 보고, 해제 뒤 가까운 적을 봄 |
| `player_execution.py` | 통과. 보스 처형, 처형 카메라 3.53~7.45초, 처형 중 사망 모션 |
| `enemy_turn_order.py` (`Enemies=Cutter,Cutter,Cutter`) | 통과. 적 락온(같은 Task)과 동시 공격 순서 |
