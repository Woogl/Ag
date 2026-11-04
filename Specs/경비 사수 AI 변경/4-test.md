# 경비 사수 AI 변경 검증 결과

## 1. 테스트 시나리오

시나리오는 `Saved/ScenarioTest/Scenarios`에 두었습니다. 통과 기준은 각 파일 맨 위에 실행 전에 적었습니다.

| 시나리오 | 실행 | 확인하는 Spec 3절 항목 |
| - | - | - |
| `gunner_sight_range.py` | `python -I Scripts/ScenarioTest/run.py Saved/ScenarioTest/Scenarios/gunner_sight_range.py --map "/Game/Maps/LV_SimulationRoom?Enemies=Gunner"` | 1, 2 |
| `gunner_retreat_ledge.py` | 같은 맵 (`Enemies=Gunner`) | 4, 8 |
| `gunner_predict_vanish.py` | 같은 맵 (`Enemies=Gunner,Sandbag`) | 5, 7 |
| `boss_swordwave_predict.py` | `python -I Scripts/ScenarioTest/run.py Saved/ScenarioTest/Scenarios/boss_swordwave_predict.py` | 6 |
| `gunner_aim_cancel.py` | `--map "/Game/Maps/LV_SimulationRoom?Enemies=Gunner"` | 9 (조준 취소) |
| `boss_swordwave_hit_reflect.py` | 기본 맵 | 9 (검기 관통·반사) |
| `enemy_patterns.py` | `--map "/Game/Maps/LV_SimulationRoom?Enemies=Cutter,Gunner,AttackSandbag"` | 9 (일반 적 패턴) |

## 2. 결과

- 빌드: 임시 폴더 컴파일을 먼저 확인했습니다(다른 세션의 변경 포함). 그 뒤 공유 트리 `Build.bat`도 성공했습니다. 에디터를 다시 켠 뒤 `LogAg: Error`는 0건입니다.
- 판정: 아래 시나리오가 모두 통과했습니다.

| 시나리오 | 판정 | 측정값 |
| - | - | - |
| `gunner_sight_range` | 통과 | 정면 80°·10m는 봄, 100°·10m는 못 봄, 정면 28m는 봄. 28m에서 다가가지 않고 사격(이동 0cm) |
| `gunner_retreat_ledge` | 통과 | 5m에서 최대 8.05m까지 물러난 뒤 0cm/s로 서 있음, 15m에서 0cm/s, 높이 1m 단상 끝에서 92cm 물러나 가장자리에서 멈춤(높이 변화 0cm), 넉다운 뒤에도 높이 변화 0cm |
| `gunner_predict_vanish` | 통과 | A 옆으로 걷기(10m) 발사 0.59초 뒤 적중, B 발사 직후 반대로 걷기 빗나감, C 서 있기 적중, D 적중 뒤 0.1초 안에 소멸·3m 뒤 샌드백 안 맞음, E 가드 뒤 소멸·샌드백 안 맞음, F 무적이면 지나가 샌드백 적중, G 패리 반사 뒤 사수 적중·소멸 |
| `boss_swordwave_predict` | 통과 | 옆으로 걷는 플레이어가 첫 검기 발사 0.74초 뒤 적중, 검기 높이 변화 0.00cm |
| `gunner_aim_cancel` | 통과 | 등 뒤로 옮기면 조준 0.45초에, 총구 앞에 벽을 세우면 0.42초에 Fire 없이 취소 |

- 로그: 결과 JSON은 `Saved/ScenarioTest/slot<자리>_20261105-02*.json`에 있습니다.
- 시나리오를 고친 이유:
  - `gunner_predict_vanish`의 첫 실행은 앞 사격 패턴(4초)이 끝나기 전에 다음 사격을 시켜서, 발사 7번 중 3번이 거부됐습니다. 간격을 늘렸습니다.
  - 두 번째 실행에서는 플레이어 8m 뒤의 샌드백에 닿기 전에 탄이 바닥에 닿았습니다. 탄이 총구 높이에서 캡슐 중심 쪽으로 내려가며 날아가기 때문입니다. 샌드백을 3m 뒤로 옮기고, 사격 명령이 사수를 고르도록 락온을 더했습니다.

## 3. 회귀 확인

- 검기 관통·패리 반사(`boss_swordwave_hit_reflect`): 통과했습니다. 검기 3회 적중, 맞은 뒤에도 검기가 남았고, 반사된 검기가 보스에게 맞았습니다.
- 일반 적 패턴(`enemy_patterns`, 시뮬레이션 룸): 통과했습니다. 절단자 3패턴, PistolShot(Aim→Fire→Reload), 공격 샌드백 모두 정상입니다.
  - 기본 맵(`LV_BossStage`)에서는 절단자가 배치돼 있지 않아 `KeyError: 'Cutter'`로 실행에 실패했습니다. 맵 배치 문제이며 이번 변경과는 관계없습니다.
- 확인하지 못한 것은 사용자 플레이 확인으로 넘깁니다.
  - 30m 밖의 플레이어를 발견하지 못하는지, 탄환이 100m에서 사라지는지: 아레나 지름이 약 33m라 재지 못했습니다. 데이터 값만 확인했습니다(`SightDistance` 3000, `Range` 10000).
  - 극한 회피 때 탄이 지나가는지: 무적과 같은 코드 경로(결과 없음 → 소멸 호출 없음)이고, 무적으로만 확인했습니다.
  - 절단자의 공격 이동이 낭떠러지에서 멈추는지, 벽에 비스듬히 닿으면 미끄러지는지: 같은 엔진 설정이라 따로 재지 않았습니다. 사수의 걷기와 넉다운으로만 확인했습니다.
