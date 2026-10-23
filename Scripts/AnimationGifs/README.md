# 애니메이션 시퀀스 GIF

`/Game`의 AnimSequence를 시퀀스마다 GIF 하나로 만듭니다. 촬영은 아래 준비·실행 절을 따릅니다. 출력 사양을 확인할 때는 [캡처 조건](<../../Docs/Art/GIF/README.md#캡처-조건>)을 참고합니다. 기획서의 여러 모션을 조합하는 그림은 `Scripts/MotionImages`를 사용합니다.

## 준비

- Python 3와 Pillow, 프로젝트를 연 UE 에디터가 필요합니다. 에디터의 Python 원격 실행을 사용합니다. Python이 PATH에 없으면 실행 파일의 절대 경로를 지정합니다.
- 현재 레벨의 조명을 사용하므로 지금 있는 GIF와 같은 밝은 레벨에서 실행합니다. PIE를 종료하고 실행하며 촬영 중에는 맵을 바꾸거나 에디터를 종료하지 않습니다.
- 촬영은 임시 액터와 Render Target을 사용합니다. 에셋 저장·게임 실행·에디터 빌드는 하지 않습니다.

새 촬영의 기본 출력 크기는 [GIF 출력 조건](<../../Docs/Art/GIF/README.md>)을 따릅니다. 기본 크기를 바꿔도 이미 있는 GIF를 자동으로 재촬영하거나 축소하지 않습니다.

## 실행

프로젝트 루트에서 실행합니다.

```powershell
# 특정 시퀀스. 정확한 에셋 이름 또는 /Game 패키지 경로를 나열합니다.
python Scripts/AnimationGifs/animation_gifs.py ARPG_Samurai_Anim_UE_Attack_Combo1 ARPG_Halberd_Anim_UE_Attack_Combo1

# 전체 목록 중 결과가 없는 시퀀스만 촬영합니다.
python Scripts/AnimationGifs/animation_gifs.py

# JSON 문자열 배열로 지정한 목록을 다시 촬영합니다.
python Scripts/AnimationGifs/animation_gifs.py --sequence-list Saved/Tools/AnimationGifs/sequences.json --overwrite

# 검토용 폴더에 다시 촬영합니다. 기본 출력은 Docs/Art/GIF입니다.
python Scripts/AnimationGifs/animation_gifs.py ARPG_Samurai_Anim_UE_Attack_Combo1 --out Saved/Tools/AnimationGifs/Preview --overwrite

# 캡처 없이 기존 실행 폴더의 완료된 시퀀스를 다시 합성합니다.
python Scripts/AnimationGifs/animation_gifs.py --compose Saved/Tools/AnimationGifs/<실행 폴더>
```

기존 결과를 바꾸려면 `--overwrite`를 지정합니다. `--workers`는 GIF 변환 프로세스 수이며 기본 2개입니다. `--timeout`은 촬영·변환 감시 제한 시간(초)이며 기본 4시간입니다.

공유 에디터의 PIE와 분리하려면 별도의 촬영 에디터를 `-RenderOffscreen -NoSplash -NoSound -unattended -NoSaveConfig`로 실행합니다. MCP는 `-ModelContextProtocolPort=8009`, Python은 `-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:RemoteExecutionMulticastGroupEndpoint=239.0.0.1:6901`처럼 사용 중이지 않은 포트로 분리하고, 실행기에 `--editor-port 6901`을 전달합니다. 명령 연결에는 바로 다음 포트인 6902도 사용합니다. 같은 프로젝트와 촬영용 레벨을 지정하고, 완료 후 직접 띄운 촬영 에디터만 종료합니다. 실행기가 별도 에디터를 자동으로 열거나 닫지는 않습니다.

## 처리와 중단

- `--compose`할 실행 폴더에 재합성에 필요한 불투명도 마스크가 없으면 재촬영합니다.
- `Ctrl+C` 또는 해당 실행 폴더에 `stop` 파일을 만들면 중단합니다. 실행기가 강제 종료되면 에디터는 마지막 heartbeat에서 60초 뒤 자동으로 정리합니다. 에디터 자체가 멈추거나 종료되면 정리 완료를 확인할 수 없으며, 결과 파일과 상태를 확인해야 합니다.
- 실행 폴더는 `Saved/Tools/AnimationGifs/<실행 ID>`입니다. `status.json`은 촬영 상태, `summary.json`은 완료 시간과 출력 목록입니다. 각 시퀀스 폴더의 `ready.json`은 촬영 완료, `composed.json`은 GIF 저장 완료를 나타냅니다. 프레임은 재합성을 위해 남습니다.
- 무기·샘플링의 출력 기준은 [GIF 출력 조건](<../../Docs/Art/GIF/README.md>)을 따르며, 설정은 도구의 `settings.py`와 게임 BP 기본값을 사용합니다.

## 도구 구현 구조

- 외부 실행기가 시퀀스 목록과 게임 BP의 무기 기본값을 조회하고, 목록과 출력 경로를 실행별 작업 파일에 기록해 에디터에 한 번 전달합니다. 에디터 파이썬의 틱 콜백이 목록을 순서대로 처리합니다.
- 임시 스켈레탈 메시와 SceneCapture2D, Render Target을 재사용합니다. 윤곽의 계단 현상을 줄이기 위해 이미지 영역의 가로·세로 2배로 렌더링하고 외부에서 Lanczos 축소·제목 띠 합성을 합니다. 처음·끝 멈춤은 GIF 표시 시간으로 처리합니다.
- 자세를 지정한 뒤 에디터가 뼈와 부착물을 갱신할 틱을 거쳐 촬영합니다. 시퀀스의 루트 잠금은 원본 에셋을 바꾸지 않고 임시 컴포넌트의 변환으로 보정합니다. 노티파이는 실행하지 않습니다.
- 에디터는 PNG 프레임과 시퀀스 완료 표식을 출력합니다. 별도 프로세스가 완료 표식을 받아 배경 정리·라벨 합성·팔레트 변환·GIF 저장을 수행합니다. GIF는 임시 파일로 완성한 뒤 교체합니다.
- 같은 자세·카메라로 색상과 불투명도 마스크를 각각 캡처합니다. 마스크는 SceneColor HDR의 역불투명도 알파를 사용하므로, 몸통의 검은 부분이 배경으로 지워지지 않습니다. 마스크로 배경을 합성한 뒤 축소하며, 팔레트는 시퀀스 전체 구간에서 뽑은 256색을 공유해 프레임 간 색 변화도 줄입니다.
- 첫 프레임 전에 같은 자세로 준비 렌더링을 해 노출·반사를 안정화합니다. SceneCapture는 고정 수동 노출을 사용하고 시간 누적 안티앨리어싱과 모션 블러를 끕니다. 원래 뷰포트와 조명·윤곽이 픽셀 단위로 같지는 않습니다. 구도는 [GIF 안내](<../../Docs/Art/GIF/README.md#캡처-조건>)를 따릅니다.
- 실행별 작업 폴더와 에디터 내 중복 실행 방지로 작업 충돌을 막습니다. 취소·실패 시 콜백과 임시 액터를 제거하고 백그라운드 CPU 제한을 복원합니다. 사용자 에셋과 레벨은 저장하지 않습니다.
- 완료된 GIF만 건너뛰며, 재생성은 명시적인 옵션으로 요청합니다. 중간 프레임은 실행 폴더에 남겨 실패한 변환을 다시 실행할 수 있습니다.
- 이는 에디터용 제작 도구이며 게임 로직, PIE, 게임 창을 사용하지 않습니다.
