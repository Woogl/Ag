# Copyright Woogle. All Rights Reserved.
"""시퀀스 목록 일괄 촬영과 별도 프로세스 GIF 합성 실행기."""
import argparse
from concurrent.futures import ProcessPoolExecutor
import json
import multiprocessing
from pathlib import Path
import sys
import time
import uuid

sys.dont_write_bytecode = True
from settings import ROOT, output_relative
from compose import compose_sequence

sys.path.insert(0, str(ROOT / "Scripts" / "MotionImages"))
import common

_editor_port = None


def remote(code):
    if _editor_port is None:
        result = common.editor_python(code)
    else:
        sys.path.insert(0, str(Path(common.engine_root()) / "Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python"))
        import remote_execution
        config = remote_execution.RemoteExecutionConfig()
        config.multicast_bind_address = "127.0.0.1"
        config.multicast_group_endpoint = ("239.0.0.1", _editor_port)
        config.command_endpoint = ("127.0.0.1", _editor_port + 1)
        connection = remote_execution.RemoteExecution(config)
        connection.start()
        try:
            deadline = time.monotonic() + 30
            nodes = []
            while not nodes and time.monotonic() < deadline:
                nodes = [node for node in connection.remote_nodes if Path(node.get("project_root", "")).resolve() == ROOT]
                if not nodes:
                    time.sleep(0.2)
            if not nodes:
                raise RuntimeError("지정한 포트에 이 프로젝트의 에디터가 없습니다.")
            connection.open_command_connection(nodes[0])
            reply = connection.run_command(code, unattended=True, exec_mode=remote_execution.MODE_EXEC_FILE, raise_on_failure=False)
            if not reply.get("success"):
                raise RuntimeError(reply.get("result"))
            result = [entry.get("output", "") for entry in reply.get("output", [])]
        finally:
            connection.stop()
    if result is None:
        raise RuntimeError("이 프로젝트를 연 에디터의 Python 원격 실행에 연결할 수 없습니다.")
    return result


def editor_import():
    return ("import sys, importlib; sys.dont_write_bytecode=True; sys.path.insert(0, %r); import editor_capture\n"
            "if editor_capture._active is not None: raise RuntimeError('Animation GIF capture is already running')\n"
            "if editor_capture.unreal.get_editor_subsystem(editor_capture.unreal.UnrealEditorSubsystem).get_game_world() is not None: raise RuntimeError('Stop PIE before capturing')\n"
            "import settings; importlib.reload(settings); importlib.reload(editor_capture)\n") % str(Path(__file__).parent)


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
    global _editor_port
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
    _editor_port = args.editor_port
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
    remote(editor_import() + "editor_capture.atomic_json(editor_capture.Path(%r), editor_capture.inventory())" % str(inventory_file))
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
    lines = remote(common._WEAPON_CODE % (json.dumps({name: value["blueprint"] for name, value in common.CHARACTERS.items()}), common.WEAPON_BONE))
    weapons = json.loads(next(line.split("AG_WEAPONS ", 1)[1] for line in lines if line.startswith("AG_WEAPONS ")))
    manifest = folder / "manifest.json"
    manifest.write_text(json.dumps({"sequences": jobs, "weapons": weapons, "max_pending": max(2, args.workers * 2)}), encoding="utf-8")
    (folder / "heartbeat").touch()
    print("작업 폴더: " + str(folder), flush=True)
    try:
        remote(editor_import() + "editor_capture.start(%r)" % str(manifest))
        summary = monitor(folder, args.workers, args.timeout)
    except BaseException:
        (folder / "stop").touch()
        raise
    print("%d개 GIF 완료, %.2f초" % (len(summary["outputs"]), summary["wall_seconds"]), flush=True)


if __name__ == "__main__":
    multiprocessing.freeze_support()
    main()
