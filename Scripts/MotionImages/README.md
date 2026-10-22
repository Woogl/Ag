# 기획서 모션 그림

플레이어·보스 사양 문서의 조합 GIF를 만듭니다. `sheets/<그림 이름>.json`의 모션 구간을 [이미 촬영한 GIF](<../../Docs/Art/GIF/README.md>)에서 가져와 순서대로 연결합니다. Python 3와 Pillow가 필요하며, 에디터 연결이나 재촬영은 필요하지 않습니다.

## 실행

```powershell
# 전체 조합 GIF를 Docs/Design/Attachments에 갱신합니다.
python Scripts/MotionImages/motion_images.py assemble

# 지정한 조합만 갱신합니다.
python Scripts/MotionImages/motion_images.py assemble "플레이어 모션 평타"

# 별도 폴더에서 먼저 확인합니다.
python Scripts/MotionImages/motion_images.py assemble --out Saved/Tools/MotionImages/Preview
```

GIF만 정의한 그림은 `make`·`compose`도 `assemble`과 같은 조립 경로를 사용합니다. 구간·배속을 바꾼 뒤 다시 실행하면 됩니다. 없는 원본이나 원본 길이를 벗어난 구간은 오류로 알립니다. 원본 모션이 바뀌었으면 [시퀀스 촬영 도구](<../AnimationGifs/README.md>)로 해당 원본을 갱신한 뒤 조립합니다.

## 조합 정의

```json
{
  "image": "보스 모션 예시",
  "character": "boss",
  "animation": {
    "play_rate": 0.8,
    "segments": [
      {"clip": "Halberd/Attack_Combo1", "from": 0.0, "to": 0.136, "rate": 0.3523},
      {"clip": "Halberd/Attack_Combo1", "from": 0.136, "to": 0.27},
      {"clip": "Halberd/Attack_Combo2", "from": 0.0, "hold": 0.35}
    ]
  }
}
```

| 항목 | 뜻 |
| - | - |
| `image` | 출력 파일 이름(확장자 제외) |
| `character` | 에디터 캡처로 만드는 칸 그림(`rows`)용 캐릭터 지정. GIF 조립에서는 원본의 캐릭터와 무기를 그대로 사용합니다. |
| `play_rate` | 전체 재생 속도. 기본 1이며 실제 기획 배속은 사양 문서를 따릅니다. |
| `segments` | 재생 순서. 반복하려면 같은 구간을 필요한 횟수만큼 나열합니다. |
| `clip` | `팩/모션 이름`. 예: `Samurai/Attack_Combo_B1`. ARPG 팩이 아니면 에셋 이름을 그대로 씁니다. 예: `Dodge_and_Evade/Dodge_NoHandSpin_Left` |
| `from`, `to` | 원본 시퀀스의 시작·끝 시각(초) |
| `rate` | 해당 구간의 추가 배속. 전체 배속과 곱하며 기본 1입니다. |
| `hold` | `to` 대신 지정하면 `from` 자세를 해당 초만큼 유지합니다. 배속의 영향을 받지 않습니다. |

원본 상단의 이름·시각 띠를 포함한 전체 프레임을 사용합니다. 띠의 시각과 1x 표시는 원본 시퀀스 기준이며 조합의 배속·누적 시간을 나타내지 않습니다. 주요 프레임 테두리·예고·판정 문구를 추가하지 않습니다. 구간 설명은 사양 문서 본문에 둡니다. 원본 GIF의 처음·끝 감상용 멈춤은 제거하고, 조합 전체에만 시작·끝 멈춤을 적용합니다. 원본에서 가장 가까운 프레임을 선택하므로 재촬영과 달리 자세 시간 해상도는 원본 프레임 간격에 제한됩니다.

GIF를 팩 사이에서 이어 붙이면 캐릭터와 무기가 바뀔 수 있습니다. 조립 시 이를 바꾸거나 게임의 부착 전환·이펙트를 재현하지 않습니다. 지금 크기(480×457) 원본과 이전 크기(640×574, 480×431) 원본을 모두 읽습니다. 원본 파일은 유지하고, 너비가 480보다 큰 원본은 480으로 Lanczos 축소합니다. 정보 띠가 낮은 이전 원본은 세로로 늘리지 않고 위를 정보 띠 색으로 채워 480×457에 맞춥니다. 축소 후 원본 프레임의 팔레트로 색을 매핑하고 최종 GIF를 한 번 저장합니다.

## 칸 그림(JPG)

`rows`로 정의하는 JPG 칸 그림은 에디터 캡처로 만듭니다. 이 경우 `probe` → `make`로 생성하며 에디터 연결이 필요합니다. 사양 문서의 조합은 모두 `animation`만 사용하는 GIF입니다. GIF만 갱신할 때는 `assemble`을 사용합니다.

중간 캡처·모션 측정 자료는 `Saved/Tools/MotionImages`에 저장됩니다. GIF 조립은 이 캐시에 의존하지 않습니다.
