# 애니메이션 시퀀스 GIF

`/Game`의 AnimSequence를 시퀀스마다 GIF 하나로 만듭니다. 촬영은 아래 준비·실행 절을 따릅니다. 출력 사양을 확인할 때는 [캡처 조건](<../../Docs/Art/GIF/README.md#캡처-조건>), 도구를 수정할 때는 [구현 문서](<../../Docs/Program/애니메이션 GIF 촬영.md#구현>)를 참고합니다. 기획서의 여러 모션을 조합하는 그림은 `Scripts/MotionImages`를 사용합니다.

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

- 촬영·합성 구조와 화질 설정의 이유는 [구현 문서](<../../Docs/Program/애니메이션 GIF 촬영.md>)를 따릅니다. 변환 대기가 많으면 촬영이 잠시 기다립니다.
- `--compose`할 실행 폴더에 재합성에 필요한 불투명도 마스크가 없으면 재촬영합니다.
- `Ctrl+C` 또는 해당 실행 폴더에 `stop` 파일을 만들면 중단합니다. 실행기가 강제 종료되면 에디터는 마지막 heartbeat에서 60초 뒤 자동으로 정리합니다. 에디터 자체가 멈추거나 종료되면 정리 완료를 확인할 수 없으며, 결과 파일과 상태를 확인해야 합니다.
- 실행 폴더는 `Saved/Tools/AnimationGifs/<실행 ID>`입니다. `status.json`은 촬영 상태, `summary.json`은 완료 시간과 출력 목록입니다. 각 시퀀스 폴더의 `ready.json`은 촬영 완료, `composed.json`은 GIF 저장 완료를 나타냅니다. 프레임은 재합성을 위해 남습니다.
- 무기·샘플링의 출력 기준은 [GIF 출력 조건](<../../Docs/Art/GIF/README.md>)을 따르며, 설정은 도구의 `settings.py`와 게임 BP 기본값을 사용합니다.
