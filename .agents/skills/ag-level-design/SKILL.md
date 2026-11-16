---
name: ag-level-design
description: 레벨 작업용. 레벨(맵)의 지형·구조물을 만들거나 옮기기, 적 배치, MCP·에디터 파이썬으로 그레이박스 제작, 레벨 기획서와 3D HTML 맞추기를 할 때 읽는다. 캐릭터·전투·UI·C++ 작업에는 쓰지 않는다.
---

# 레벨 디자인 작업

AI와 함께 레벨을 만들 때의 요령과, 지금까지 밟은 함정을 모았다. 에디터를 켜고 빌드하고 저장하는 공통 요령은 [ag-editor-workflow](<../ag-editor-workflow/SKILL.md>)를 따르고, 이 파일에는 레벨 작업에만 해당하는 것을 적는다.

## 작업 순서

바닥, 구멍, 길 찾기, 적 시야를 좌표표로 계산하는 스크립트는 만들지 않고 5번에서 에디터에 묻는다. 만든 3D HTML을 스크린샷으로 찍어 대조하지 않는다.

1. **설계**: 구간표와 좌표표를 만든다. 구간표에는 레벨을 진행 순서대로 나눈 구간(기획서의 '구간'과 같은 단위)마다 크기, 바닥 높이, 적, 재미 요소를 글로 적고, 좌표표에는 액터마다 이름, 위치, 크기, 회전을 적는다. 둘 다 커밋되지 않게 `Saved/LevelDesign/<맵 이름>/`에 둔다. 작업이 끊기면 여기서 이어 간다.
   - 구간표와 기획서의 왼쪽·오른쪽은 진행 방향(+X)을 바라본 플레이어 기준이다. 언리얼 좌표에서 오른쪽은 +Y, 왼쪽은 −Y다.
   - 점프나 낙하로 길을 건너뛰는 경로는 설계에서 계산해 막지 않는다. 눈에 띄는 것만 구간표에 적고(기획서에서는 '미확정 항목'), 플레이로 판단한다.
2. **승인**: 좌표표로 마우스로 돌리고 확대해 볼 수 있는 3D HTML을 만들어 구간표와 함께 보여 주고 승인받는다. 3D HTML에는 단, 계단, 벽, 장애물의 높이 차이가 보여야 한다. 빼기 브러시는 사람이 볼 정보가 아니라 그리지 않는다.
   - 3D HTML(6번의 것도)은 `Docs/Design/Level`의 기존 3D HTML을 틀로 삼아 그 안의 데이터만 바꾼다. 틀이 그리지 못하는 모양이 있으면 그리기 코드를 더한다.
3. **배치**: 새 레벨이면 아래 '새 레벨 만들기'대로 만든다. 좌표표대로 배치한다.
4. **빌드**: 지오메트리와 내비게이션을 다시 빌드하고(아래 '그레이박싱') 저장한다(아래 '저장과 월드 파티션').
5. **검증**: 레벨을 다시 열고 불러오지 않은 액터를 불러온 뒤(아래 '저장과 월드 파티션'), 좌표표와 이름·위치·크기·회전을 짝으로 대조하고 개수도 센다. 이어서 에디터에 묻고, 문제가 있으면 좌표표를 고쳐 레벨을 맞춘 뒤 4번부터 다시 한다.
   - 큰 길을 따라 촘촘히(10cm쯤) 아래로 레이를 쏘아 바닥이 끊긴 곳이 없는지, 문과 구멍에 옆으로 레이를 쏘아 뚫렸는지 본다. 이웃한 두 바닥 브러시 사이의 좁은 틈은 눈으로 보이지 않는다.
   - 구간마다, 그리고 구간 안에서 플레이어가 설 수 있는 높이(단, 발코니)마다 길 찾기가 이어지는지 본다(아래 '그레이박싱'). 구간의 입구와 출구가 이어져도 그 사이의 단이 끊겨 있을 수 있고, 끊긴 단에 선 플레이어에게는 근거리 적이 오지 못한다.
   - 적이 나오는 레벨이면 각 적의 캡슐 중심에서 큰 길 위 플레이어의 캡슐 중심까지 레이를 쏘아, [적 시스템](<../../../Docs/Design/System/적 시스템.md>) '공격 대상과 시야'와 각 적 문서의 시야 거리·정면 각도로 적이 플레이어를 처음 보는 지점을 찾는다.
6. **반영**: 확인을 마쳐 정본에 반영할 때는 `Docs/Design/Level/`에 기획서와 레벨을 읽어 만든 3D HTML을 나란히 두고, 3D HTML은 기획서에서 링크한다. 새 기획서는 [문서 인덱스](<../../../Docs/인덱스.md>)에도 넣는다. 높이는 기획서 본문에 적는다. 기획서에 적는 거리와 "누가 어디서 보이는지" 같은 문장은 5번처럼 에디터에 다시 물어 맞춘다. 다 맞췄으면 `Saved/LevelDesign/<맵 이름>/`을 지운다. 에디터에서 레벨을 직접 고친 뒤에도 이렇게 맞춘다.
   - 새 스테이지면 [게임 플로우](<../../../Docs/Design/System/게임 플로우.md>) '스테이지 선택'의 표에도 넣는다. 스테이지 선택 목록에는 `/Game/Maps`의 레벨이 저절로 나온다(같은 절).
   - 월드 파티션 미니맵을 다시 굽는다(아래 '월드 파티션 미니맵').

## 새 레벨 만들기

새 레벨은 월드 파티션 레벨로 만든다([기본 방침](<../../../Docs/Architecture/기본 방침.md>) '레벨 파일').

- 에디터 파이썬 `LevelEditorSubsystem.new_level("/Game/Maps/<맵 이름>", True)`로 만든다(두 번째 인자를 빼면 월드 파티션이 아니다). 이 함수는 열린 레벨을 저장하지 않고 닫으니, 부르기 전에 `EditorLoadingAndSavingUtils.get_dirty_map_packages()`로 저장하지 않은 변경이 있는지 보고, 있으면 묻는다.
  - `new_level`은 만든 맵을 바로 디스크에 저장한다. 시험으로 만든 맵은 `EditorAssetLibrary.delete_asset`으로 맵과 HLOD 레이어 에셋 둘을 지우고, `__ExternalActors__/Maps/<맵 이름>`과 (있으면) `__ExternalObjects__/Maps/<맵 이름>` 폴더도 지운다.
- 새 레벨에는 조명이 없다. 조명·하늘·구름·안개와 레벨 전체 포스트 프로세스를 담은 `/Game/Maps/BP_StageEnvironment`를 원점에 놓는다(`EditorActorSubsystem.spawn_actor_from_class`). 값을 고치면 이 블루프린트를 쓰는 모든 레벨에 반영되니, 한 레벨만 다르게 하려면 그 레벨에 놓인 액터의 값을 바꾼다.
- 내비 범위는 `EditorActorSubsystem.spawn_actor_from_object(unreal.NavMeshBoundsVolume.static_class(), 위치)`로 놓고 `SetBrushSettings`로 레벨 크기에 맞춘다.
- 월드 파티션 미니맵의 범위는 `WorldPartitionMiniMapVolume`으로 정한다. 볼륨이 없으면 엔진이 BSP 브러시를 빼고 범위를 잡아 지형 대부분이 잘린다. 내비 범위처럼 놓고, 바깥 벽까지 덮게 `SetBrushSettings`로 크기를 맞춘다.
- 월드 세팅의 `default_game_mode`는 다른 스테이지 레벨과 같게(`BP_StageGameMode`) 둔다.

## 그레이박싱

그레이박스의 지형과 구조물은 [기본 방침](<../../../Docs/Architecture/기본 방침.md>) '그레이박스'대로 BSP 브러시로 만든다.

- 프로젝트 MCP 도구 `AgEditor.AgLevelToolset`의 `AddBrush`로 놓고, 크기는 `SetBrushSettings`로 바꾼다. 에디터 파이썬에서는 `unreal.AgLevelToolset.add_brush(...)`처럼 부른다. 모양(`/Script/UnrealEd.<모양>Builder`)과 브러시 세팅 이름, 모양마다 `location`이 가리키는 자리는 도구 설명에 있다.
- 새 브러시 면에는 콘텐츠 브라우저에서 선택된 재질이 들어간다. 기본 재질(`WorldGridMaterial`)로 놓으려면 선택된 에셋이 없는지 먼저 보고, 다른 재질로 놓으려면 `EditorAppToolset.SelectAssets`로 그 재질을 고른 뒤 놓는다.
- 맵의 내비 메시는 정적이라 저장된 것이 게임에서 쓰인다. 지형을 바꾸면 `RebuildNavigation`으로 내비게이션을 다시 빌드하고, 바뀐 곳마다 길 찾기(`NavigationSystemV1.find_path_to_location_synchronously`, 결과의 `is_partial()`)가 이어지는지 확인한 뒤 레벨을 저장한다(`RecastNavMesh` 액터 포함).
  - 지오메트리를 다시 빌드한 바로 뒤에 보낸 `RebuildNavigation`은 로그에 "navigation build is locked"를 남기고 무시될 수 있다. 그 사이 내비 메시는 비어 있으니, 다음 명령에서 다시 보내고 로그의 `UNavigationSystemV1::Build total execution time` 줄을 확인한다.

## 저장과 월드 파티션

- 새로 만든 액터는 `SceneTools.save_actor`로 저장되지 않는다("Asset does not exist"). MCP `AssetTools.save_assets`에 빈 목록을 주거나 에디터 파이썬 `EditorLoadingAndSavingUtils.save_dirty_packages(True, True)`로 저장한다. 둘 다 저장하지 않은 것을 모두 저장하므로, 먼저 `get_dirty_map_packages()`와 `get_dirty_content_packages()`로 이번 작업이 아닌 패키지가 섞여 있지 않은지 본다.
- 월드 파티션 레벨에서는 불러온 액터만 `get_all_level_actors`에 나온다. 레벨을 새로 열면 적, 성문 같은 액터는 불러오지 않은 상태다(시작 위치, 조명, 브러시는 불러온다). `WorldPartitionBlueprintLibrary.get_actor_descs()`로 전체 목록을 읽고, 항목의 `guid`를 `load_actors`에 넘겨 불러온다. 불러와도 레벨은 저장할 변경으로 잡히지 않는다.

### 월드 파티션 미니맵

- 에디터의 빌드 → Build Minimap은 저장 확인 창을 띄우고, 열린 레벨을 내리고, 열린 에셋 편집기를 모두 닫은 뒤 커맨드렛을 돌린다. 함께 쓰는 에디터에서는 이 메뉴를 쓰지 않고, 그 레벨이 에디터에 열려 있지 않을 때 같은 커맨드렛을 셸에서 돌린다(레벨 하나에 15초쯤 걸린다). 열려 있으면 에디터가 미니맵 파일을 잡고 있어 커맨드렛이 저장하다 죽는다(로그의 `Error Code 32`).
  - `<엔진>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe <저장소>\Ag.uproject /Game/Maps/<맵 이름> -run=WorldPartitionBuilderCommandlet -Builder=WorldPartitionMiniMapBuilder -AllowCommandletRendering -Unattended`
  - PowerShell에서 돌린다. Git Bash는 `/Game/Maps/...` 인자를 윈도 경로로 바꿔, 커맨드렛이 맵을 못 찾고 바로 끝난다.
- 결과는 맵의 `WorldPartitionMiniMap` 액터 파일에 저장되고 git에 스테이징된다. 로그의 `WorldBounds:` 줄이 미니맵 볼륨의 범위와 같은지 보고, 인자에 `-DebugCapture`를 더하면 `Intermediate/Minimap/<맵 이름>-Minimap.bmp`로 그림을 볼 수 있다.

### 지운 액터가 되살아나는 문제

BSP 브러시를 뺀 액터(볼륨 포함)는 액터 파일(`Content/__ExternalActors__/Maps/<맵 이름>/...`)로 저장되고, 지운 액터의 파일은 저장할 때 지워진다. 아래 경우에는 파일이 남아, 레벨을 다시 열거나 에디터를 다시 켜면 지운 액터가 다시 나온다.

- 지운 뒤 저장하기 전에 실행 취소 기록이 비워진 경우. 콘솔 `MAP REBUILD`, 빌드 메뉴의 지오메트리·경로·전체 빌드, 조명 빌드가 기록을 비운다(`RebuildGeometry`·`SetBrushSettings`와 콘솔 `RebuildNavigation`은 비우지 않는다). 그래서 지오메트리는 `RebuildGeometry`로 다시 빌드하고, 액터를 지웠으면 이런 빌드를 하기 전에 저장한다.
- 저장할 때 Git 연동의 `git rm`이 실패한 경우(로그에 `SCC failed to open the selected files for deletion`). 같은 저장소에서 다른 git 명령이 돌거나 `.git/index.lock`이 남아 있으면 실패하니, 저장하는 동안 git 명령을 함께 돌리지 않는다.

지우기 전에 지울 액터의 패키지 경로(`actor.get_package().get_name()`)를 적어 두고, 저장 뒤 그 파일이 디스크에 없는지 본다. 남았으면 보고하고, 에디터를 다시 켠 뒤 그 액터를 다시 지우고 바로 저장한다.
