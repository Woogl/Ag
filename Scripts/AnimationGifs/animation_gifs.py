# Copyright Woogle. All Rights Reserved.
"""시퀀스 목록 일괄 촬영과 별도 프로세스 GIF 합성 실행기."""
import argparse
from concurrent.futures import ProcessPoolExecutor
import json
import multiprocessing
import os
from pathlib import Path
import sys
import time
import uuid

sys.dont_write_bytecode = True
from settings import CHARACTERS, ROOT, WEAPON_BONE, output_relative
from compose import compose_sequence

# 에디터에서 게임 BP를 잠깐 스폰해, 태그가 Weapon인 컴포넌트(게임이 무기로 찾는 컴포넌트)가 WEAPON_BONE에 대해 어디에
# 붙는지 잽니다. 소켓, 컴포넌트 오프셋, 메시를 바꿔도 게임과 같은 자리가 나옵니다.
_WEAPON_CODE = """
import json, unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
out = {}
for name, path in json.loads(%r).items():
    actor = eas.spawn_actor_from_class(unreal.EditorAssetLibrary.load_blueprint_class(path), unreal.Vector(0, 0, -10000), unreal.Rotator(0, 0, 0), True)
    try:
        weapon = actor.get_components_by_tag(unreal.SceneComponent, "Weapon")[0]
        bone = actor.get_editor_property("mesh").get_socket_transform(%r, unreal.RelativeTransformSpace.RTS_WORLD)
        rel = weapon.get_world_transform().make_relative(bone)
        static = isinstance(weapon, unreal.StaticMeshComponent)
        mesh = weapon.get_editor_property("static_mesh" if static else "skeletal_mesh_asset")
        t, q, s = rel.translation, rel.rotation, rel.scale3d
        out[name] = {"mesh": mesh.get_path_name(), "loc": [t.x, t.y, t.z], "q": [q.x, q.y, q.z, q.w], "scale": [s.x, s.y, s.z]}
    finally:
        eas.destroy_actor(actor)
print("AG_WEAPONS " + json.dumps(out))
"""


def engine_root():
    """Ag.uproject의 엔진 버전으로 런처가 설치한 엔진 폴더를 찾습니다."""
    version = json.loads((ROOT / "Ag.uproject").read_text(encoding="utf-8"))["EngineAssociation"]
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\EpicGames\Unreal Engine\%s" % version) as key:
            return Path(winreg.QueryValueEx(key, "InstalledDirectory")[0])
    except OSError:
        return Path(r"C:\Program Files\Epic Games\UE_%s" % version)


def remote(code, port):
    """이 프로젝트를 연 에디터에서 code를 실행하고 출력 줄들을 돌려줍니다. port를 주면 그 포트로 띄운 촬영 전용 에디터에 보냅니다.
    파일 실행 방식이라 code의 첫 '.py' 바로 뒤가 공백이나 끝이면 그 앞이 파일 경로로 읽히므로, code에 '.py'를 쓰지 않습니다."""
    engine_python = str(engine_root() / "Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python")
    if engine_python not in sys.path:
        sys.path.append(engine_python)
    import remote_execution
    config = remote_execution.RemoteExecutionConfig()
    config.multicast_bind_address = "127.0.0.1"
    if port is not None:
        config.multicast_group_endpoint = ("239.0.0.1", port)
        config.command_endpoint = ("127.0.0.1", port + 1)
    connection = remote_execution.RemoteExecution(config)
    connection.start()
    try:
        deadline = time.monotonic() + 30
        nodes = []
        while not nodes and time.monotonic() < deadline:
            nodes = [node for node in connection.remote_nodes
                     if os.path.normcase(os.path.normpath(node.get("project_root", ""))) == os.path.normcase(ROOT)]
            if not nodes:
                time.sleep(0.2)
        if not nodes:
            raise RuntimeError("지정한 포트에 이 프로젝트의 에디터가 없습니다." if port is not None
                               else "이 프로젝트를 연 에디터의 Python 원격 실행에 연결할 수 없습니다.")
        connection.open_command_connection(nodes[0])
        reply = connection.run_command(code, unattended=True, exec_mode=remote_execution.MODE_EXEC_FILE, raise_on_failure=False)
    finally:
        connection.stop()
    if not reply.get("success"):
        raise RuntimeError("에디터 파이썬 실행 실패: %s" % reply.get("result"))
    return [entry.get("output", "") for entry in reply.get("output", [])]


def editor_import():
    # 촬영 중인지는 reload 앞에서 확인합니다. reload가 editor_capture._active를 지웁니다.
    folder = str(Path(__file__).parent)
    return ("import sys, importlib; sys.dont_write_bytecode=True\n"
            "if %r not in sys.path: sys.path.insert(0, %r)\n"
            "import settings, editor_capture\n"
            "if editor_capture._active is not None: raise RuntimeError('Animation GIF capture is already running')\n"
            "if editor_capture.unreal.get_editor_subsystem(editor_capture.unreal.UnrealEditorSubsystem).get_game_world() is not None: raise RuntimeError('Stop PIE before capturing')\n"
            "importlib.reload(settings); importlib.reload(editor_capture)\n") % (folder, folder)


def measure_weapons(port):
    blueprints = json.dumps({name: info["blueprint"] for name, info in CHARACTERS.items()})
    lines = remote(_WEAPON_CODE % (blueprints, WEAPON_BONE), port)
    return json.loads(next(line.split("AG_WEAPONS ", 1)[1] for line in lines if line.startswith("AG_WEAPONS ")))


def read_json(path):
    for attempt in range(10):
        try:
            return json.loads(path.read_text(encoding="utf-8"))
        except PermissionError:
            if attempt == 9:
                raise
            time.sleep(0.02)


def monitor(folder, workers, timeout):
    submitted, results, futures = set(), [], {}
    started = time.perf_counter()
    last_report = 0
    try:
        with ProcessPoolExecutor(max_workers=workers) as pool:
            while True:
                (folder / "heartbeat").touch()
                status_path = folder / "status.json"
                status = read_json(status_path) if status_path.exists() else {"state": "starting"}
                if status["state"] in ("failed", "cancelled"):
                    raise RuntimeError(status.get("error") or status["state"])
                for ready in sorted(folder.glob("*/ready.json")):
                    if ready not in submitted:
                        submitted.add(ready)
                        futures[pool.submit(compose_sequence, str(ready.parent))] = ready
                for future in list(futures):
                    if future.done():
                        result = future.result()
                        results.append(result)
                        print("완료: " + result["output"], flush=True)
                        del futures[future]
                if status["state"] == "complete" and not futures:
                    summary = {"wall_seconds": time.perf_counter() - started, "capture": status, "outputs": results}
                    (folder / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
                    return summary
                if time.perf_counter() - started > timeout:
                    raise TimeoutError("촬영 제한 시간을 초과했습니다.")
                if time.perf_counter() - last_report > 10:
                    print("촬영 %s프레임 / GIF %d개 완료" % (status.get("frames", 0), len(results)), flush=True)
                    last_report = time.perf_counter()
                time.sleep(0.2)
    except BaseException:
        (folder / "stop").touch()
        # 실행기 종료 전에 에디터가 콜백과 임시 액터를 정리할 기회를 줍니다.
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            path = folder / "status.json"
            if path.exists() and read_json(path)["state"] != "running":
                break
            time.sleep(0.2)
        raise


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sequences", nargs="*", help="/Game 경로 또는 정확한 에셋 이름. 생략하면 전체")
    parser.add_argument("--sequence-list", type=Path, help="시퀀스 경로 문자열 배열을 담은 JSON 파일")
    parser.add_argument("--editor-port", type=int, help="촬영 전용 에디터의 Python multicast 포트. 명령 연결은 다음 포트")
    parser.add_argument("--out", type=Path, default=ROOT / "Docs" / "Art" / "GIF")
    parser.add_argument("--overwrite", action="store_true")
    parser.add_argument("--workers", type=int, default=2)
    parser.add_argument("--timeout", type=float, default=14400)
    parser.add_argument("--compose", type=Path, help="기존 실행 폴더의 프레임만 다시 합성")
    args = parser.parse_args()
    if args.editor_port is not None and not 1024 <= args.editor_port <= 65534:
        parser.error("editor-port는 1024~65534여야 합니다.")
    requested_sequences = list(args.sequences)
    if args.sequence_list:
        listed = read_json(args.sequence_list)
        if not isinstance(listed, list) or not all(isinstance(path, str) for path in listed):
            parser.error("sequence-list는 시퀀스 경로 문자열 배열이어야 합니다.")
        requested_sequences.extend(listed)
        if not requested_sequences:
            parser.error("sequence-list가 비어 있습니다.")
    if args.workers < 1 or args.timeout <= 0:
        parser.error("workers와 timeout은 양수여야 합니다.")
    if args.compose:
        with ProcessPoolExecutor(max_workers=args.workers) as pool:
            for result in pool.map(compose_sequence, [str(p.parent) for p in sorted(args.compose.glob("*/ready.json"))]):
                print(result["output"])
        return
    folder = ROOT / "Saved" / "Tools" / "AnimationGifs" / (time.strftime("%Y%m%d-%H%M%S") + "-" + uuid.uuid4().hex[:8])
    folder.mkdir(parents=True)
    inventory_file = folder / "inventory.json"
    remote(editor_import() + "editor_capture.atomic_json(editor_capture.Path(%r), editor_capture.inventory())" % str(inventory_file), args.editor_port)
    paths = read_json(inventory_file)
    selected = []
    for requested in requested_sequences:
        matches = [path for path in paths if path == requested or path.rsplit("/", 1)[-1] == requested]
        if len(matches) != 1:
            raise ValueError("시퀀스를 하나로 찾을 수 없습니다: " + requested)
        if matches[0] not in selected:
            selected.append(matches[0])
    selected = selected if requested_sequences else paths
    jobs = [{"path": path, "output": str((args.out / output_relative(path)).resolve())} for path in selected
            if args.overwrite or not (args.out / output_relative(path)).exists()]
    if not jobs:
        print("촬영할 시퀀스가 없습니다. 기존 결과를 갱신하려면 --overwrite를 사용하세요.")
        return
    # 게임 BP의 무기 기본값을 한 번만 측정합니다. 프레임 루프에는 원격 호출이 없습니다.
    weapons = measure_weapons(args.editor_port)
    manifest = folder / "manifest.json"
    manifest.write_text(json.dumps({"sequences": jobs, "weapons": weapons, "max_pending": max(2, args.workers * 2)}), encoding="utf-8")
    (folder / "heartbeat").touch()
    print("작업 폴더: " + str(folder), flush=True)
    try:
        remote(editor_import() + "editor_capture.start(%r)" % str(manifest), args.editor_port)
        summary = monitor(folder, args.workers, args.timeout)
    except BaseException:
        (folder / "stop").touch()
        raise
    print("%d개 GIF 완료, %.2f초" % (len(summary["outputs"]), summary["wall_seconds"]), flush=True)


if __name__ == "__main__":
    multiprocessing.freeze_support()
    main()
