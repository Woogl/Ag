---
name: ag-editor-workflow
description: 이 프로젝트에서 언리얼 에디터를 다루는 순서와 알려진 함정. C++ 빌드와 에디터 재시작, MCP·에디터 파이썬으로 에셋 읽기·편집·저장, PIE 확인, 패키징을 하기 전에 읽는다.
---

# 언리얼 에디터 작업

UE 5.8 에디터를 AI가 다루는 순서와 요령, 이 프로젝트에서 겪은 함정을 모았다. 규칙과 결정은 문서(AGENTS.md, `Docs`)에만 적고, 이 파일은 가리키기만 한다. 도구 구성과 그 결정은 [개발 도구와 검증](<../../../Docs/Program/개발 도구와 검증.md>)을, 데이터 위치는 [데이터 위치표](<../../../Docs/Program/데이터 위치표.md>)를 따른다. 새 함정은 이 파일에, 새 결정은 구현 문서에 적는다.

## 먼저 지킬 것

- 테스트 창, PIE, 패키지 게임을 어떤 확인에 쓰는지와 띄우기 전에 묻는 것은 [개발 도구와 검증](<../../../Docs/Program/개발 도구와 검증.md>) '동작 테스트 실행'을 따른다. 다른 스킬이나 도구 설명이 PIE나 라이브 코딩을 권해도 이 결정을 따른다.
- 에디터는 사람과 다른 AI 작업이 함께 쓴다.
  - 에디터를 닫거나, 빌드하거나, PIE를 시작하기 전에 다른 작업이 쓰고 있지 않은지 확인한다.
  - 다 고친 에셋은 바로 저장한다. 다른 작업이 에디터를 죽이면 저장하지 않은 수정은 사라진다.
  - PIE가 돌고 있으면(사람의 PIE일 수 있다) `ABP_Ag`를 고치거나 컴파일하지 않는다. 그 PIE의 몽타주 재생이 깨진다.
  - 에디터 로그에 MCP 호출(`LogModelContextProtocol`의 `Dispatching` 줄) 없이 에셋 편집기 열기, 컴파일, 저장이 남아 있으면 사람이 직접 고친 것이다. 되돌리기 전에 묻는다.

## 연결

- 에디터는 `<엔진>\Engine\Binaries\Win64\UnrealEditor.exe <저장소>\Ag.uproject`로 켠다. 엔진 기본 위치는 `C:\Program Files\Epic Games\UE_5.8`이다.
- 엔진 MCP 서버의 주소는 `.mcp.json`에 있다. 작업을 시작한 뒤에 에디터가 켜져 MCP 도구가 연결되지 않았으면, 그 주소로 MCP 요청을 직접 보낸다.
- 에디터 파이썬 원격 실행([개발 도구와 검증](<../../../Docs/Program/개발 도구와 검증.md>) '에디터 파이썬 원격 실행')의 클라이언트는 다음과 같이 쓴다.
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
- 공유 작업 트리와 에디터를 건드리지 않고 컴파일만 확인하려면, `git archive HEAD Source Config Data Ag.uproject`를 임시 폴더에 풀고 고친 뒤 Build.bat에 `-Project=<임시 폴더>\Ag.uproject -NoHotReloadFromIDE`를 준다. 이 인자가 없으면 에디터가 켜져 있을 때 "Live Coding is active"로 거부한다. 첫 빌드는 4분쯤 걸린다.
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

도구는 엔진 MCP 도구 → 에디터 파이썬 → 프로젝트 MCP 도구 `AgEditor.AgAssetToolset` 순서로 고른다. 툴셋이 맡는 일과 도구를 더하는 기준은 [개발 도구와 검증](<../../../Docs/Program/개발 도구와 검증.md>) '프로젝트 MCP 도구'를 따른다. 파이썬에서는 `unreal.AgAssetToolset.<함수>`로 부르고, 새 도구는 기존 함수처럼 정적 `UFUNCTION(meta = (AICallable))`으로 만든다. 툴셋이 바꾼 에셋은 저장하지 않은 상태로 남으니 직접 저장한다.

### 에디터 없이 읽기

- `.uasset`의 이름 표를 읽는다. 속성 이름은 값이 기본값과 다를 때만 남으므로, 없으면 기본값이다. `/Game/...` 경로는 그 에셋이 참조하는 에셋이다.
- 레벨 액터는 액터마다 파일이 따로 있다(`Content/__ExternalActors__/Maps/<맵>/`).
- 스켈레탈 메시 컴포넌트의 메시는 `SkinnedAsset`으로 저장된다.

### 저장과 git

- 에디터의 Git 연동은 새로 저장한 에셋(복제 포함)을 자동으로 스테이징한다. 헤드리스 커맨드렛도 같다. 원하지 않은 스테이징은 `git restore --staged <경로>`로 푼다.
- 에디터가 불러온 `.uasset`은 git이 덮어쓰지 못한다('unable to unlink'). `unreal.EditorLoadingAndSavingUtils.unload_packages`로 내린 뒤 `git restore`한다.
- 슬레이트 틱 콜백 안에서는 에셋을 저장하지 않는다. Git 연동의 진행 창이 슬레이트를 다시 틱해 콜백이 되풀이되고, 스택이 넘쳐 에디터가 죽는다. 값은 메모리에서 되돌리고, 저장은 따로 보낸 명령으로 한다.

### 삭제와 이름 변경

- 지우기 전에 `get_referencers`로 참조를 확인한다. 리비전 컨트롤이 꺼진 에디터에서 MCP `AssetTools.delete`는 메모리의 패키지만 지우고 `.uasset`은 남긴다. 파일은 `git rm`으로 지운다.
- 참조를 다른 에셋으로 바꾼 직후 지우려면 먼저 `unreal.SystemLibrary.collect_garbage()`를 부른다. 아니면 강제 삭제가 ensure에 걸린다.
- `AssetTools.rename_assets`는 옛 경로에 리다이렉터를 남기고, 파이썬에는 리다이렉터를 정리하는 함수가 없다. 리다이렉터를 `save_packages`로 저장해 dirty를 없앤 뒤 `git rm -f`로 지운다.
- INI의 소프트 경로가 가리키는 에셋은 이름 변경이 취소된다(확인 창이 무인 실행에서 자동으로 취소된다). 복제하고, INI가 복제본을 가리키게 한 뒤, 에디터를 닫고 옛 파일을 지운다.

### 블루프린트

- MCP `set_properties`로 블루프린트 CDO나 그 컴포넌트를 고쳐도 레벨에 배치된 인스턴스에는 가지 않는다. 인스턴스도 고치거나, 레벨이 dirty가 아닐 때 레벨을 다시 연다.
- 파이썬으로 `Default__BP_X_C`를 고쳤으면 `unreal.BlueprintEditorLibrary.compile_blueprint(bp)` 뒤에 다시 저장한다. 컴파일하지 않으면 스폰된 인스턴스가 네이티브 기본값을 받는다.
- C++ 컴포넌트의 블루프린트 기본값은 `/Game/.../BP_X.Default__BP_X_C:<컴포넌트>`에 `set_properties` → `compile_blueprint` → 저장으로 바꾼다. 스폰되는 액터에만 간다.
- PIE는 에디터 월드를 복제할 때 construction을 다시 실행하지 않는다. 에셋을 저장한 뒤에도 배치된 액터가 옛 데이터를 보이면 레벨을 다시 연다.
- 블루프린트 컴포넌트의 Parent Socket은 `AgAssetToolset.SetComponentParentSocket`으로 바꾼다. 무기를 블루프린트 컴포넌트로 둔 이유는 [애니메이션과 이동](<../../../Docs/Program/애니메이션과 이동.md>) '무기'를 따른다.
  - 블루프린트 컴포넌트는 `SubobjectDataSubsystem.add_new_subobject`로 더하고, 이름은 `rename_subobject_member_variable`로 바꾼다(`rename_subobject`는 조용히 실패한다).
  - 부모 C++ 클래스의 UPROPERTY와 같은 이름의 블루프린트 변수는 컴파일이 실패한다.
- 컴파일 뒤 `bp.get_editor_property("status")`를 확인하고, 오류 상태로 저장하지 않는다.
- 부모 클래스는 `unreal.BlueprintEditorLibrary.reparent_blueprint(bp, cls)` 뒤 컴파일해 바꾼다. 옛 부모에만 있던 속성은 사라지니 먼저 읽어 둔다.
- C++ 속성을 Transient로 바꿔도, 블루프린트에 전에 저장된 그 속성의 값은 블루프린트 CDO에 다시 읽혀 저장할 때 참조(import)로 남는다. 그 참조 에셋을 지울 때는 에디터를 닫고 에셋 파일을 지운 뒤, 에디터를 다시 켜 블루프린트를 다시 저장하고 `.uasset`에 옛 경로가 없는지 `grep -a`로 확인한다. 컴파일하면 레벨에 배치된 인스턴스가 다시 만들어져 레벨이 dirty가 되므로 저장만 한다.
- 테스트용 액터를 에디터 월드에 스폰하면 레벨이 dirty가 된다. 잴 것이 있으면 임시 스폰(`EditorActorSubsystem.spawn_actor_from_class(cls, loc, rot, True)`)으로 띄우고 지운다.

### 애니메이션

- 몽타주 섹션과 트랙 길이는 엔진 MCP와 파이썬으로 바꿀 수 없다. `AgAssetToolset.SetMontageSegments`·`SetMontageSections`를 쓴다. 섹션은 시작 시간 순서로 넘긴다(툴셋이 정렬하지 않는다).
- 복제한 몽타주는 `SetMontageSegments` 뒤에도 옛 섹션의 시퀀스 참조가 남는다. `SetMontageSections`를 다시 부르고 저장한다.
- `SetMontageSegments`로 몽타주가 길어지면 옛 끝에서 끝나던 노티파이 상태를 엔진이 새 끝까지 늘리지만, 끝 링크는 옛 구간에 남는다. 그 노티파이는 지우고(`remove_animation_notify_events_by_track`) 다시 넣는다.
- 몽타주 안의 게임 규칙 시각(판정·보호·워핑·후딜 같은 사건)은 노티파이가 아니라 `Data/MontageTimelines.json`에서 고친다([몽타주 시간표](<../../../Docs/Program/몽타주 시간표.md>)). 몽타주에는 연출과 루트 모션 노티파이만 두며, 프로젝트 노티파이나 엔진 워핑 노티파이를 넣으면 시간표 검증이 오류를 낸다.
- 몽타주 노티파이는 `get_properties`로 읽을 수 없다. `unreal.AnimationLibrary.get_animation_notify_events(montage)`로 읽고, 트랙별로는 `get_animation_notify_events_for_track`으로 읽는다.
- 노티파이 시각을 float 값 그대로 읽으려면 `unreal.AnimationLibrary.get_anim_notify_event_trigger_time`(트리거 오프셋 포함)과 `get_anim_notify_event_duration`을 쓴다. 이벤트의 `trigger_time_offset`과 `Duration`은 파이썬에서 읽을 수 없고, `export_text()`의 값은 소수 6자리다.
- 노티파이는 `unreal.AnimationLibrary.add_animation_notify_event(montage, 트랙 이름, 시각, 노티파이 클래스)`로 더하고, 돌려받은 노티파이에 속성(`gameplay_cue` 등)을 쓴 뒤 저장한다.
- 몽타주 섹션의 시작 시간과 다음 섹션은 파이썬 속성으로 읽을 수 없다(`composite_sections`에 접근하지 못한다). `unreal.AssetExportTask`의 `exporter`에 `unreal.ObjectExporterT3D()`를 주고 `unreal.Exporter.run_asset_export_task`로 T3D를 내보내면 `CompositeSections`(시각 `LinkValue`, `NextSectionName`)와 `Notifies`(시각, 길이, `TriggerTimeOffset`, 끝 링크)가 소수 6자리로 나온다.
- 엔진 노티파이는 정방향 재생에서 '앞 위치 < 노티파이 시각 ≤ 지금 위치'일 때만 울린다. 섹션 시작 시각에 정확히 둔 노티파이는 그 섹션으로 점프한 직후 울리지 않으니 조금 뒤에 둔다. 시간표의 지점은 섹션 이동 직후 그 위치의 지점도 실행한다.
- 몽타주는 섹션 시작 시각에 정확히 둔 노티파이에 −0.0001초 오프셋을 줘 앞 섹션 끝에서 울린다. 반복 섹션 바로 뒤 섹션의 시작에 둔 노티파이 상태는 반복할 때마다 켜졌다 꺼지므로, 시작을 0.0001초 뒤에 둔다.
- 몽타주 시작(0초)에 둔 루트 모션 끄기 노티파이는 엔진이 0.0001초 뒤에 켜고, 몽타주는 한 프레임 안에서 루트 모션을 먼저 뽑은 뒤 분기점 노티파이를 처리한다. 그래서 첫 프레임에 그 앞 조각의 루트 모션이 남고, 그 프레임은 캐릭터 무브먼트가 루트 모션 소스를 쓰지 않고 수평 속도도 덮어쓴다. 몽타주 전체에서 루트 모션을 쓰지 않으려면 노티파이 대신 시퀀스의 루트 모션을 끄고 루트 뼈를 고정한다(`enable_root_motion` False, `force_root_lock` True).
- 루트 모션 끄기 노티파이 구간 안에서 새로 재생한 몽타주 인스턴스(BlendTime 전환)도 첫 프레임의 루트 모션을 먼저 뽑고, 캐릭터 무브먼트는 그 속도를 다음 프레임들에도 이어 간다. 그 구간에 쓰는 시퀀스는 루트 모션을 끈다.
- 몽타주를 이어 재생할 때, `OnBlendOut`은 길이에서 블렌드 아웃 시간을 뺀 때에 온다. 거기서 다음 몽타주를 시작하면 앞 모션이 잘리므로, 앞 몽타주의 `BlendOutTriggerTime`을 0으로 둔다.
- 스크립트로 만든 블렌드 스페이스는 `ResampleData()`가 불리지 않아 기본 자세(A 포즈)를 재생한다. `OpenEditorForAsset`으로 열고 dirty로 만든 뒤 저장한다.
- 애님 그래프 노드의 값이 핀으로 보이면(`bShowPin`) 컴파일할 때 핀 기본값이 덮어쓴다. `BlueprintTools.set_pin_value`로 바꾼다. 핀만 바꾸면 패키지가 dirty가 되지 않아 저장되지 않으니, 노드를 따로 dirty로 만들어 저장한 뒤 파일 시각을 확인한다.
- 포즈 평가(`AnimPoseExtensions.get_anim_pose_at_time`)에 몽타주를 넘기면 엔진이 `check`로 죽는다. 시퀀스를 넘긴다. 모션 측정은 [Samurai 애니메이션 조사](<../../../Docs/Art/Samurai 애니메이션 조사.md>) '측정 방법'과 `Scripts/MotionImages/ue_probe.py`를 따른다.

### GAS와 StateTree

- GameplayCue를 복제하면 태그를 바꿀 때까지 "Multiple gameplay cues ... Skipping" 경고가 나고 원본만 재생된다. 복제본의 CDO에 `set_editor_property('gameplay_cue_tag', tag)`로 태그를 주면 바로 다시 등록된다. 엔진 `GameplayCueToolset.AddCueTag`는 태그 ini에 쓰므로 쓰지 않는다(AGENTS.md 코딩 규칙 3).
- 에디터를 켤 때 부모 클래스를 찾지 못한 GameplayCue 블루프린트는 경고 없이 Cue 목록에서 빠진다. 켜 둔 에디터에서 부모를 바꿔 저장해도 목록은 다시 만들어지지 않아, 그 세션의 PIE에서는 그 Cue가 나오지 않는다. 부모를 바꿨으면 에디터를 다시 켠 뒤 확인한다. 등록 여부는 `GameplayCueManager_0`의 `GlobalGameplayCueSet`에서 `GameplayCueData`의 태그로 본다(파이썬에 `GameplayCueSet` 타입이 없어 `ObjectIterator(unreal.Object)`에서 클래스 이름으로 찾는다).
- StateTree는 `AgAssetToolset`의 StateTree 함수로 고치고, `CompileStateTree`(빈 문자열이면 오류 없음)로 컴파일한 뒤 저장한다. 읽기와 스키마 속성은 [개발 도구와 검증](<../../../Docs/Program/개발 도구와 검증.md>) '프로젝트 MCP 도구'를 따른다.
- 파이썬으로 상태 이름을 바꾸려면 `unreal.ObjectIterator(unreal.StateTreeState)`를 `get_outermost().get_name() == '/Game/AI/ST_Boss'`로 걸러 `set_editor_property('name', ...)`로 바꾸고, 컴파일하고 저장한다. 전환은 ID로 이어져 있어 그대로 남는다.

### 구조체와 배열

- MCP `set_properties`는 배열 크기와 요소를 한 번에 바꾸지 못한다. 먼저 남길 요소를 `get_properties`가 돌려준 그대로(객체 참조는 `{"refPath": ...}`) 넘겨 줄이고, 같은 크기에서 새 값을 넣는다.
- 인스턴스 하위 객체 배열에 요소를 더할 때는 남길 요소를 그대로 두고 새 자리에 클래스 경로(`{"refPath": "/Script/Ag.<클래스>"}`)를 넣는다. 그 클래스의 새 하위 객체가 생긴다.
- 구조체 안의 EditDefaultsOnly 필드는 `set_editor_property`로 쓸 수 없다. `struct.import_text('(...)')`로 채운 뒤 구조체 전체를 CDO에 쓴다.
- 머티리얼 Custom 노드의 입력은 `set_properties` 한 번에 하나씩, 요소를 통째로(`inputName`과 `input`) 넣어 늘린다.

### UI

- 위젯 애니메이션은 MCP와 파이썬으로 만들 수 없다. `AgAssetToolset.SetFadeAnimation`·`RemoveWidgetAnimation`을 쓴다.
- UMG를 MCP로 다룰 때:
  - `OverlaySlot`은 기본이 왼쪽 위 정렬이라 배경 이미지는 채우기로 바꾼다.
  - `ProgressBar`의 기본 스타일은 엔진 텍스처를 쓴다. 단색 막대는 `ResourceName`을 "None", `ImageType`을 NoImage로 둔다.
  - `GetWidgets`는 부모 클래스의 BindWidget 이름도 `widget: None`으로 보여 준다. 이름이 이미 쓰였는지는 `widget`이 있는지로 판단한다.
- 슬레이트는 틴트 알파가 0인 박스를 그리지 않는다. 채움이 투명한 둥근 박스는 외곽선도 보이지 않는다.
- `FKey::GetDisplayName()`은 에디터 언어를 따른다(한국어 에디터에서 '왼쪽 Shift').

## PIE

PIE를 쓰는 경우는 [개발 도구와 검증](<../../../Docs/Program/개발 도구와 검증.md>) '동작 테스트 실행'을 따른다.

- 뒤에 있는 에디터는 PIE를 초당 3프레임쯤으로 돌려 타이밍 확인이 모두 틀린다. `/Script/UnrealEd.Default__EditorPerformanceSettings`의 `bThrottleCPUWhenNotForeground`를 끄고(MCP `set_properties`나 파이썬 `unreal.find_object`), 끝나면 되돌린다. 최소화된 창은 이 설정을 꺼도 느리므로, 포커스를 가져오지 않고 창만 복원한다.
- `/Script/UnrealEd.Default__LevelEditorPlaySettings`를 고치면 사용자의 `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`에 바로 써진다. 고친 값은 모두 되돌린다.
- PIE는 StopPIE 뒤에도 남을 수 있다. 그동안 에셋 도구는 "Asset does not exist"를 돌려준다. 에셋을 조회하거나 에디터를 닫기 전에 PIE가 돌고 있는지 확인한다.
- 슬레이트 틱 콜백으로 PIE를 조종하는 스크립트는 끝날 때와 오류가 났을 때 모두 `editor_request_end_play()`를 부른다. 아니면 확인용 입력이 눌린 채 PIE가 남는다.
- 키 입력은 확인용 입력([개발 도구와 검증](<../../../Docs/Program/개발 도구와 검증.md>) '확인용 입력')으로 넣는다. 회피 방향(`DevMoveInput`)은 회피 키보다 한 프레임 이상 먼저 넣는다. `DevPressInput`은 다음 원격 호출 전에 끝나므로 몽타주 확인은 `DevMashInput`으로 한다.
- 보스는 `boss.get_controller().get_component_by_class(unreal.BrainComponent).stop_logic("test")`로 멈춘다. 멈추지 않으면 StateTree가 파이썬으로 실행한 패턴을 끊는다.
- 행동(보스 패턴 포함)은 모두 같은 클래스 `UAgGameplayAbility`의 스펙이라 태그로 실행되지 않는다. 행동 태그는 스펙 태그에 있고, 엔진 `TryActivateAbilitiesByTag`·`FindAllAbilitiesWithTags`는 어빌리티의 에셋 태그만 본다. `asc.get_all_abilities()`의 핸들은 `Characters.json`에서 그 캐릭터의 `Abilities` 순서이므로, 그 순서로 고른 핸들로 `asc.try_activate_ability(handle, True)`를 부른다.
- `Data` JSON은 저장하면 PIE 중에도 바로 다시 읽힌다(`UAgDataReloadSubsystem`). 테스트 때문에 고칠 때는 '동작 테스트 실행'을 따르고, 끝나면 원래 바이트로 되돌린다.
- 엔진 자동화 테스트를 돌린 뒤에는 남은 컴파일 오류 창이 PIE 시작을 막을 수 있다. 에디터를 다시 켜면 풀린다.
- 에디터를 다시 켠 직후에는 첫 MCP 호출이 두 번 처리될 수 있다. StartPIE가 'A play session is already running'으로 답하면 방금 보낸 첫 호출이 먼저 처리된 것일 수 있으니 로그의 `Dispatching` 줄부터 본다.
- CommonUI는 에디터에서 PIE 뷰포트가 포커스를 가질 때만 입력 설정과 첫 포커스를 적용한다. 뒤에 있는 PIE에서 버튼 선택이 보이지 않으면 버그로 보기 전에 포커스부터 확인한다. 사람의 마우스가 떠 있는 PIE 창을 지나가도 버튼이 선택되니, 선택이 이상하면 `is_hovered()`와 커서 위치를 본다.
- CommonUI 버튼은 `<화면>.WidgetTree_0.<버튼>.WidgetTree_0.InternalRootButtonBase`의 `on_clicked.broadcast()`로 누른다. 화면은 `is_activated()`로 고른다. 이전 월드의 화면이 GameInstance에 남아 있다.
- 캡처: `CaptureEditorImage`는 화면을 반 크기로 찍고, 0.1 ~ 0.3초짜리 효과는 찍히지 않는다. UI 스크린샷은 떠 있는 PIE 창에서 콘솔 `slomo 0.02` 뒤 `shot showui`로 찍는다(`Saved/Screenshots/WindowsEditor`). OS 화면 캡처(PowerShell 등)는 쓰지 않는다. 다른 창이 에디터를 가려 사용자의 화면이 찍힌다.
- 소리가 났는지는 콘솔 `au.DumpActiveSounds`를 0.1초마다 실행해 `LogAudio` 줄로 확인한다.
- 모션 시간(후딜, 무적 구간 등)은 실제 시간이 아니라 몽타주 재생 위치(`anim_instance.montage_get_position(montage)`)로 잰다. 히트스톱 동안 몽타주가 멈춘다. 시간표의 구간과 사건은 그 시각을 지난 프레임에 실행되므로, 바뀐 태그나 다음 행동은 시각보다 한 프레임 안쪽 늦게 보인다. 미리 누른 키는 선입력 시간이 히트스톱 동안에도 흐르니, 행동 시점을 잴 때는 키를 계속 눌러 둔다.
- 로그로 확인할 때:
  - 엔진 ensure와 일부 경고(Nanite 머티리얼 경고 등)는 프로세스마다 처음 한 번만 찍힌다. 고쳤는지는 새로 켠 에디터나 게임에서, 실패하던 경우를 먼저 실행해 확인한다.
  - 에디터를 켤 때마다 나오는 `LogAutomationTest: Error: Condition failed` 줄들은 엔진 자체 테스트의 잡음이다.

## 에디터 파이썬 이름

| 하려는 것 | 쓰는 것 |
| - | - |
| 어빌리티 시스템 함수 | `unreal.AbilitySystemLibrary` (`AbilitySystemBlueprintLibrary`는 없다) |
| 위젯 함수 | `unreal.WidgetLibrary` (`WidgetBlueprintLibrary`는 없다) |
| 태그 | `t = unreal.GameplayTag()` 뒤 `t.import_text('(TagName="Input.Attack")')`, 컨테이너는 `unreal.GameplayTagLibrary` |
| 캐릭터의 ASC와 어트리뷰트 세트 | `unreal.find_object(None, actor.get_path_name() + '.AbilitySystem')`, `'.AttributeSet'` |
| 어트리뷰트 값 쓰기 | 값 구조체에 `set_editor_property("base_value", x)`와 `"current_value"`를 쓴 뒤 세트에 `set_editor_property("HP", v)`. 최대값부터 쓴다. |
| 게임플레이 이벤트 | `unreal.AbilitySystemLibrary.send_gameplay_event_to_actor` |
| GameplayCue 실행 | `unreal.GameplayCueFunctionLibrary.execute_gameplay_cue_on_actor` |
| GameplayEffect 적용 | 프로젝트 효과는 `Effects.json`에서 만든 객체라, 클래스를 받는 `asc.make_outgoing_spec(cls, ...)`로는 스펙을 만들 수 없다. 자원·스탯은 '어트리뷰트 값 쓰기'로 바꾼다. 엔진 GE 클래스는 `spec = asc.make_outgoing_spec(cls, 1.0, asc.make_effect_context())`, `unreal.AbilitySystemLibrary.assign_tag_set_by_caller_magnitude`·`set_duration`·`add_granted_tag`(각각 핸들을 돌려준다), `asc.apply_gameplay_effect_spec_to_self(spec)` |
| 캐릭터 무브먼트 | `char.get_editor_property("character_movement")` |
| PIE 월드 | `unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()` |
| 콘솔 명령 | `unreal.SystemLibrary.execute_console_command(world, cmd, unreal.GameplayStatics.get_player_controller(world, 0))` |
| JSON 데이터 객체 | 에디터의 에셋 매니저는 `/Engine/Transient.UnrealEdEngine_0:AgAssetManager_0`이다. 하위 객체를 `unreal.find_object(None, <매니저 경로> + '.Effects')`처럼 이름(`CombatRules`, `CameraData`, `Effects`, `Projectiles`, `MontageTimelines`, `TargetingPresets`, `Abilities`)으로 찾는다. 파이썬 타입이 없는 클래스(`UAgEffectSet` 등)도 이렇게 찾는다. `ObjectIterator`로 찾으면 매니저 CDO의 같은 이름 하위 객체(`/Script/Ag.Default__AgAssetManager:<이름>`)와 읽을 때 만든 임시 객체도 함께 나온다. |
| 파이썬 타입이 없는 클래스 | CDO를 `unreal.find_object(None, '/Script/<모듈>.Default__<클래스>')`로 찾는다 |
| 속성 이름 | `get_editor_property`·`set_editor_property`는 네이티브 C++ 이름(`bThrottleCPUWhenNotForeground`, `MaxHP`)도 받는다 |

## 패키징

- 패키지 게임을 띄우기 전의 물음은 [개발 도구와 검증](<../../../Docs/Program/개발 도구와 검증.md>) '동작 테스트 실행'을 따른다.
- `Saved/StagedBuilds` 아래의 `Ag.exe`가 아직 돌고 있으면 스테이지 단계가 실패한다(ExitCode=102). 먼저 끈다.
- 다른 작업이 에디터를 빌드하려는 중에는 패키징하지 않는다. 쿡이 에디터 DLL을 불러온다.
- 명령: `RunUAT.bat BuildCookRun -project=<저장소>\Ag.uproject -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -nocompileeditor -unattended -utf8output`. `-nocompileeditor`는 켜져 있는 에디터의 DLL을 건드리지 않게 한다. 결과는 `Saved/StagedBuilds/Windows`에 생긴다.
- 확인: `Ag.exe /Game/Maps/LV_BossStage -windowed -ResX=1280 -ResY=720 -nosound`를 30초쯤 띄우고(가만히 두면 보스에게 쓰러져 결과 UI가 뜬다) `Saved/StagedBuilds/Windows/Ag/Saved/Logs/Ag.log`에 `LogAg: Error`가 없는지 본다. 창 닫기 요청으로는 닫히지 않으니 프로세스를 끈다.
