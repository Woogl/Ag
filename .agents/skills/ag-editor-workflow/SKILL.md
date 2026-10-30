---
name: ag-editor-workflow
description: 이 프로젝트에서 언리얼 에디터를 다루는 순서와 알려진 함정. C++ 빌드와 에디터 재시작, MCP·에디터 파이썬으로 에셋 읽기·편집·저장, PIE 확인, 패키징을 하기 전에 읽는다.
---

# 언리얼 에디터 작업

UE 5.8 에디터를 AI가 다루는 순서와 요령, 이 프로젝트에서 겪은 함정을 모았다. 규칙과 결정은 문서(AGENTS.md, `Docs`)에만 적고, 이 파일은 가리키기만 한다. 도구 구성과 그 결정은 [개발 도구와 검증](<../../../Docs/Architecture/개발 도구와 검증.md>)을, 데이터 위치는 [데이터 로딩과 스키마](<../../../Docs/Architecture/데이터 로딩과 스키마.md>)를 따른다. 새 함정은 이 파일이나 해당 참고 파일에, 새 결정은 `Docs/Architecture`의 해당 공통 문서에 적는다.

## 먼저 지킬 것

- 테스트 창, PIE, 패키지 게임을 어떤 확인에 쓰는지는 [개발 도구와 검증](<../../../Docs/Architecture/개발 도구와 검증.md>) '동작 테스트 실행'을, 띄우기 전에 묻는 것은 [AI 워크플로우](<../../../Docs/AI 워크플로우.md>) '테스트 작업 수칙'을 따른다. 다른 스킬이나 도구 설명이 PIE나 라이브 코딩을 권해도 이 결정을 따른다.
- 에디터는 사람과 다른 AI 작업이 함께 쓴다.
  - 에디터를 닫거나, 빌드하거나, PIE를 시작하기 전에 다른 작업이 쓰고 있지 않은지 확인한다.
  - 다 고친 에셋은 바로 저장한다. 다른 작업이 에디터를 죽이면 저장하지 않은 수정은 사라진다.
  - PIE가 돌고 있으면(사람의 PIE일 수 있다) `ABP_Character`를 고치거나 컴파일하지 않는다. 그 PIE의 몽타주 재생이 깨진다.
  - 에디터 로그에 MCP 호출(`LogModelContextProtocol`의 `Dispatching` 줄) 없이 에셋 편집기 열기, 컴파일, 저장이 남아 있으면 사람이 직접 고친 것이다. 되돌리기 전에 묻는다.

## 연결

- 에디터는 `<엔진>\Engine\Binaries\Win64\UnrealEditor.exe <저장소>\Ag.uproject`로 켠다. 엔진 기본 위치는 `C:\Program Files\Epic Games\UE_5.8`이다.
- 엔진 MCP 서버의 주소는 `.mcp.json`에 있다. 작업을 시작한 뒤에 에디터가 켜져 MCP 도구가 연결되지 않았으면, 그 주소로 MCP 요청을 직접 보낸다.
- 에디터 파이썬 원격 실행([개발 도구와 검증](<../../../Docs/Architecture/개발 도구와 검증.md>) '에디터 파이썬 원격 실행')의 클라이언트는 다음과 같이 쓴다.
  - 엔진의 `Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python`을 `sys.path`에 넣고 `import remote_execution as remote`.
  - `config = remote.RemoteExecutionConfig()`, `config.multicast_bind_address = "127.0.0.1"`, `rx = remote.RemoteExecution(config)`, `rx.start()`.
  - `rx.remote_nodes`가 차면 `rx.open_command_connection(node)`, 코드는 `rx.run_command(code, unattended=True, exec_mode=remote.MODE_EXEC_FILE, raise_on_failure=False)`로 보내고, 끝나면 `rx.stop()`.
  - `MODE_EXEC_FILE`은 보낸 코드에 `.py`라는 글자가 있으면(주석이라도) 파일 경로로 여겨 실패한다.
  - 보낸 코드의 전역 변수는 다음 명령에도 남는다. 에셋(노티파이 구조체 등)을 담은 전역 변수가 남아 있으면 그 에셋을 지울 때 '사용 중'으로 실패하므로, 에셋은 함수 안에서만 잡거나 지우기 전에 전역 변수를 비운다.
  - 다른 프로젝트의 에디터도 켜져 있으면 첫 노드가 이 프로젝트가 아닐 수 있다. `unreal.Paths.get_project_file_path()`로 먼저 확인한다.
  - 테스트 창도 원격 실행을 쓴다. 창마다 포트를 나누는 방법은 `Scripts/ScenarioTest`의 코드를 따른다.
- `python -I`로 돌리는 스크립트는 `PYTHONIOENCODING`을 무시한다. 한글을 출력하려면 `sys.stdout.reconfigure(encoding="utf-8")`를 먼저 부른다.
- 저장소의 파이썬 모듈(`Scripts/MotionImages/common.py` 등)을 가져다 쓰는 임시 스크립트는 `sys.dont_write_bytecode = True`를 먼저 둔다. 아니면 저장소에 `__pycache__`가 생긴다.

## 빌드와 에디터 재시작

- 헤더나 리플렉션(UCLASS, UPROPERTY, UFUNCTION)을 바꾸면 전체 빌드가 필요하다.
  1. 저장하지 않은 에셋과 맵을 확인해(`unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()`, `get_dirty_map_packages()`) 저장하거나 되돌린다. 남아 있으면 에디터를 닫을 때 '콘텐츠 저장' 창이 떠서 에디터가 닫히지 않고 MCP도 막힌다. 그 창이 떴으면 그 창에 창 닫기를 보내 종료를 취소하고, dirty 에셋을 정리한 뒤 다시 닫는다.
  2. 테스트 창이 떠 있으면 닫는다(`Scripts/ScenarioTest` README).
  3. 에디터에 창 닫기를 요청한다(PowerShell `CloseMainWindow()`). 강제로 끄지 않는다.
  4. PowerShell에서 빌드한다: `& '<엔진>\Engine\Build\BatchFiles\Build.bat' AgEditor Win64 Development '-Project=<저장소>\Ag.uproject' -WaitMutex`. Git Bash에서 `cmd //c`로 부르면 따옴표가 깨져 시작하지 않는다.
  5. 빌드가 성공했을 때만 에디터를 다시 켠다.
- `.cpp`만 바꿨으면 라이브 코딩을 먼저 써도 된다. 테스트 창을 띄우기 전의 빌드는 `Scripts/ScenarioTest` README의 '준비'를 따른다.
- 공유 작업 트리와 에디터를 건드리지 않고 컴파일만 확인하려면, `git archive HEAD Source Config DesignData Ag.uproject`를 임시 폴더에 풀고 고친 뒤 Build.bat에 `-Project=<임시 폴더>\Ag.uproject -NoHotReloadFromIDE`를 준다. 이 인자가 없으면 에디터가 켜져 있을 때 "Live Coding is active"로 거부한다. 첫 빌드는 4분쯤 걸린다.
- 다른 빌드가 돌고 있으면 UBT 뮤텍스가 겹친다. RunUAT가 1초 만에 "ExitCode=10 (Error_SDKNotFound)"로 끝나면 이 경우다. 그 빌드가 끝난 뒤 다시 한다.
- 컴파일 함정:
  - 에디터 모듈 `AgEditor`는 .cpp가 적어 PCH에 기대지 못한다. 파일마다 쓰는 헤더를 직접 include한다. `ToolsetDefinition.h` 앞에는 `UObject/Object.h`가 필요하다.
  - `UEditorSubsystem`은 `EditorSubsystem` 모듈에 있다(UnrealEd가 아니다).
  - 경고는 오류로 처리된다. 위젯 클래스에서 `Visibility`처럼 UWidget 멤버와 같은 이름의 지역 변수를 쓰면 C4458로 빌드가 실패한다.
- C++ 클래스나 속성 이름을 바꿀 때는 다음 순서를 따른다.
  1. `Config/DefaultEngine.ini`에 임시 `[CoreRedirects]`(`+ClassRedirects`, `+PropertyRedirects`)를 넣고 빌드한다.
  2. 에셋 이름을 바꾸고, 참조하는 에셋을 모두 다시 저장한다.
  3. 저장된 `.uasset`에 옛 이름이 없는지 확인한다(`grep -a`).
  4. 임시 리다이렉트를 지운다.

## 에셋 다루기

도구는 엔진 MCP 도구 → 에디터 파이썬 → 프로젝트 MCP 도구 `AgEditor.AgAssetToolset` 순서로 고른다. 툴셋이 맡는 일과 도구를 더하는 기준은 [개발 도구와 검증](<../../../Docs/Architecture/개발 도구와 검증.md>) '프로젝트 MCP 도구'를 따른다. 파이썬에서는 `unreal.AgAssetToolset.<함수>`로 부르고, 새 도구는 기존 함수처럼 정적 `UFUNCTION(meta = (AICallable))`으로 만든다. 툴셋이 바꾼 에셋은 저장하지 않은 상태로 남으니 직접 저장한다.

에셋 종류별 요령은 해당 작업을 할 때만 이 폴더의 참고 파일에서 읽는다.

| 작업 | 참고 파일 |
| - | - |
| 에디터 없이 `.uasset` 읽기, 파이썬으로 못 읽는 속성 읽기(T3D), 에셋 삭제·이름 변경, 블루프린트, 메시 소켓, 구조체·배열 속성, 머티리얼 노드, GameplayCue, StateTree | [에셋 편집](<에셋 편집.md>) |
| 몽타주, 노티파이, 루트 모션, 블렌드 스페이스, 애님 그래프, 레벨 시퀀스, PIE에서 모션 시간 재기 | [애니메이션](<애니메이션.md>) |
| 나이아가라 시스템·이미터, 효과 미리 보기 | [나이아가라](<나이아가라.md>) |
| 위젯, UMG, 슬레이트, PIE에서 CommonUI 버튼 확인·누르기 | [UI](<UI.md>) |
| 에디터 파이썬에서 쓸 클래스·함수 이름(GAS, 태그, 위젯, JSON 데이터 객체) | [에디터 파이썬 이름](<에디터 파이썬 이름.md>) |
| 패키지 빌드와 실행 확인 | [패키징](<패키징.md>) |

### 저장과 git

- 에디터의 Git 연동은 새로 저장한 에셋(복제 포함)을 자동으로 스테이징한다. 헤드리스 커맨드렛도 같다. 원하지 않은 스테이징은 `git restore --staged <경로>`로 푼다.
- 에디터가 불러온 `.uasset`은 git이 덮어쓰지 못한다('unable to unlink'). `unreal.EditorLoadingAndSavingUtils.unload_packages`로 내린 뒤 `git restore`한다.
- 슬레이트 틱 콜백 안에서는 에셋을 저장하지 않는다. Git 연동의 진행 창이 슬레이트를 다시 틱해 콜백이 되풀이되고, 스택이 넘쳐 에디터가 죽는다. 값은 메모리에서 되돌리고, 저장은 따로 보낸 명령으로 한다.

## PIE

PIE를 쓰는 경우는 [개발 도구와 검증](<../../../Docs/Architecture/개발 도구와 검증.md>) '동작 테스트 실행'을 따른다.

- 뒤에 있는 에디터는 PIE를 초당 3프레임쯤으로 돌려 타이밍 확인이 모두 틀린다. `/Script/UnrealEd.Default__EditorPerformanceSettings`의 `bThrottleCPUWhenNotForeground`를 끄고(MCP `set_properties`나 파이썬 `unreal.find_object`), 끝나면 되돌린다. 최소화된 창은 이 설정을 꺼도 느리므로, 포커스를 가져오지 않고 창만 복원한다.
- `/Script/UnrealEd.Default__LevelEditorPlaySettings`를 고치면 사용자의 `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`에 바로 써진다. 고친 값은 모두 되돌린다.
- PIE는 StopPIE 뒤에도 남을 수 있다. 그동안 에셋 도구는 "Asset does not exist"를 돌려준다. 에셋을 조회하거나 에디터를 닫기 전에 PIE가 돌고 있는지 확인한다.
- 슬레이트 틱 콜백으로 PIE를 조종하는 스크립트는 끝날 때와 오류가 났을 때 모두 `editor_request_end_play()`를 부른다. 아니면 확인용 입력이 눌린 채 PIE가 남는다.
- 키 입력은 확인용 입력([개발 도구와 검증](<../../../Docs/Architecture/개발 도구와 검증.md>) '확인용 입력')으로 넣는다. 회피 방향(`DevMoveInput`)은 회피 키보다 한 프레임 이상 먼저 넣는다. `DevPressInput`은 다음 원격 호출 전에 끝나므로 몽타주 확인은 `DevMashInput`으로 한다.
- 그로기, 적 패턴, 적 HP, 자원, 무적, 적 AI 멈춤은 [개발 도구와 검증](<../../../Docs/Architecture/개발 도구와 검증.md>) '테스트용 콘솔 명령'으로 만든다. 파이썬에서는 `unreal.SystemLibrary.execute_console_command(PIE 월드, 명령, 플레이어 컨트롤러)`로 부른다. 패턴을 확인할 때는 `AgStopAI`를 먼저 부른다. 적 AI가 돌고 있으면 StateTree가 따로 실행한 패턴을 끊는다.
- 콘솔 명령은 확인할 동작의 전제만 만든다. 그로기에 빠지는 과정이나 패턴 쿨다운처럼 명령이 건너뛰는 동작을 확인할 때는 쓰지 않는다.
- 행동(보스 패턴 포함)은 모두 같은 클래스 `UAgGameplayAbility`의 스펙이라 엔진 `TryActivateAbilitiesByTag`·`FindAllAbilitiesWithTags`로 찾지 못한다. 행동 태그는 스펙 태그에 있고, 엔진은 어빌리티의 에셋 태그만 본다.
- `DesignData` JSON은 저장하면 PIE 중에도 바로 다시 읽힌다(`UAgDataReloadSubsystem`). 테스트 때문에 고칠 때는 [AI 워크플로우](<../../../Docs/AI 워크플로우.md>) '테스트 작업 수칙'을 따르고, 끝나면 원래 바이트로 되돌린다.
- 엔진 자동화 테스트를 돌린 뒤에는 남은 컴파일 오류 창이 PIE 시작을 막을 수 있다. 에디터를 다시 켜면 풀린다.
- 에디터를 다시 켠 직후에는 첫 MCP 호출이 두 번 처리될 수 있다. StartPIE가 'A play session is already running'으로 답하면 방금 보낸 첫 호출이 먼저 처리된 것일 수 있으니 로그의 `Dispatching` 줄부터 본다.
- 캡처: `CaptureEditorImage`는 화면을 반 크기로 찍고, 0.1 ~ 0.3초짜리 효과는 찍히지 않는다. UI 스크린샷은 떠 있는 PIE 창에서 콘솔 `slomo 0.02` 뒤 `shot showui`로 찍는다(`Saved/Screenshots/WindowsEditor`). OS 화면 캡처(PowerShell 등)는 쓰지 않는다. 다른 창이 에디터를 가려 사용자의 화면이 찍힌다.
- 소리가 났는지는 콘솔 `au.DumpActiveSounds`를 0.1초마다 실행해 `LogAudio` 줄로 확인한다.
- 로그로 확인할 때:
  - 엔진 ensure와 일부 경고(Nanite 머티리얼 경고 등)는 프로세스마다 처음 한 번만 찍힌다. 고쳤는지는 새로 켠 에디터나 게임에서, 실패하던 경우를 먼저 실행해 확인한다.
  - 에디터를 켤 때마다 나오는 `LogAutomationTest: Error: Condition failed` 줄들은 엔진 자체 테스트의 잡음이다.

