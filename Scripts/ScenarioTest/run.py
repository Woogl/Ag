# Copyright Woogle. All Rights Reserved.
"""테스트 창 하나를 띄워 시나리오를 실행하고, 판정을 출력한 뒤 창을 닫는다. README.md 참고.

    python -I Scripts/ScenarioTest/run.py <시나리오.py> [--hold 초] [--keep]

종료 코드: 0 통과, 1 테스트 실패, 2 실행 실패.
"""
import argparse, ast, base64, json, os, re, subprocess, sys, time

# 출력을 파일로 받아 지켜볼 때도 줄마다 바로 보이게 한다.
sys.stdout.reconfigure(encoding="utf-8", line_buffering=True)
sys.dont_write_bytecode = True

PROJECT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "Ag.uproject"))
WORK = os.path.join(os.path.dirname(PROJECT), "Saved", "ScenarioTest")
MAP = "/Game/Maps/LV_BossStage"


def engine_dir():
    """Ag.uproject의 엔진 버전으로 런처가 설치한 엔진 폴더를 찾는다."""
    version = json.load(open(PROJECT, encoding="utf-8")).get("EngineAssociation", "5.8")
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\EpicGames\Unreal Engine\%s" % version) as key:
            return os.path.join(winreg.QueryValueEx(key, "InstalledDirectory")[0], "Engine")
    except OSError:
        return r"C:\Program Files\Epic Games\UE_%s\Engine" % version


ENGINE = engine_dir()
sys.path.append(ENGINE + r"\Plugins\Experimental\PythonScriptPlugin\Content\Python")
import remote_execution as remote

# 창 자리: 화면 오른쪽 2×2. 자리마다 원격 실행 포트를 따로 써서 다른 창과 섞이지 않게 한다.
SLOTS = {1: (900, 20), 2: (1740, 20), 3: (900, 520), 4: (1740, 520)}


def take_slot():
    """비어 있는 자리를 잠근다. 그 자리 포트를 쓰는 창이 떠 있으면(--keep으로 남긴 창 포함) 건너뛴다.
    잠금 파일에는 이 run.py의 PID를 적고, 그 PID가 끝났으면 빈 자리로 본다."""
    lines = powershell("Get-CimInstance Win32_Process -Filter \"Name='UnrealEditor.exe'\" | ForEach-Object { $_.CommandLine }")
    busy = {int(p) - 6800 for p in re.findall(r"RemoteExecutionMulticastGroupEndpoint=239\.0\.0\.1:(\d+)", lines)}
    for slot in SLOTS:
        if slot in busy:
            continue
        lock = os.path.join(WORK, "slot%d.lock" % slot)
        for _ in range(2):
            try:
                fd = os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
                os.write(fd, str(os.getpid()).encode())
                os.close(fd)
                return slot, lock
            except FileExistsError:
                try:
                    owner = int(open(lock).read().strip() or 0)
                except (OSError, ValueError):
                    owner = 0
                if owner and pid_alive(owner):
                    break
                os.remove(lock)
    return None, None


def wait_for_build():
    """빌드 도구가 도는 동안 기다린다. 빌드 중에 창을 띄우면 DLL을 잡아 빌드가 실패하거나 덜 쓰인 DLL을 읽는다."""
    # 명령줄에 그 이름이 든 셸 명령까지 세지 않도록, 빌드 도구 실행 파일(dotnet으로 도는 UnrealBuildTool.dll)만 본다.
    query = ("@(Get-CimInstance Win32_Process | Where-Object { ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -like '*UnrealBuildTool.dll*') "
             "-or $_.Name -eq 'UnrealBuildTool.exe' }).Count")
    deadline, said = time.time() + 1800, False
    while time.time() < deadline:
        if powershell(query) in ("", "0"):
            return True
        if not said:
            print("빌드가 끝나기를 기다림")
            said = True
        time.sleep(5)
    return False


def unbuilt_sources():
    """게임 모듈 DLL보다 나중에 고친 C++ 파일. 창은 빌드된 DLL을 쓰므로 이 코드는 창에 들어가지 않는다."""
    root = os.path.dirname(PROJECT)
    dll = os.path.join(root, "Binaries", "Win64", "UnrealEditor-Ag.dll")
    if not os.path.exists(dll):
        return []
    built = os.path.getmtime(dll)
    paths = (os.path.join(folder, f) for folder, _, files in os.walk(os.path.join(root, "Source", "Ag")) for f in files)
    return sorted(p for p in paths if p.endswith((".h", ".cpp")) and os.path.getmtime(p) > built)


def start_watchdog(game_pid, slot):
    """run.py가 어떤 이유로든 끝나면 창을 닫는 감시 프로세스를 띄운다. WMI로 띄워 run.py와 함께 끊기지 않게 한다.
    창 PID가 다른 프로세스에 다시 쓰였을 수 있으니, 이 자리의 포트를 쓰는 테스트 창인지 확인하고 닫는다."""
    script = ("Wait-Process -Id %d -ErrorAction SilentlyContinue; "
              "$p = Get-CimInstance Win32_Process -Filter 'ProcessId=%d'; "
              "if ($p -and $p.CommandLine -like '*RemoteExecutionMulticastGroupEndpoint=239.0.0.1:%d*') "
              "{ Stop-Process -Id %d -Force }") % (os.getpid(), game_pid, 6800 + slot, game_pid)
    encoded = base64.b64encode(script.encode("utf-16-le")).decode()
    cmd = "powershell -NoProfile -NonInteractive -WindowStyle Hidden -EncodedCommand " + encoded
    powershell("Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{ CommandLine = '%s' } | Out-Null" % cmd)


def pid_alive(pid):
    out = subprocess.run(["tasklist", "/FI", "PID eq %d" % pid, "/NH"], capture_output=True, text=True).stdout
    return str(pid) in out


def powershell(script):
    # 한글(창 제목)이 시스템 코드페이지로 나오지 않게 출력을 UTF-8로 고정한다.
    script = "[Console]::OutputEncoding = [Text.Encoding]::UTF8\n" + script
    encoded = base64.b64encode(script.encode("utf-16-le")).decode()
    return subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-EncodedCommand", encoded],
                          capture_output=True, text=True, encoding="utf-8", errors="replace").stdout.strip()


def launch(slot, title):
    """게임 창을 WMI로 띄운다. 이 셸의 자식 프로세스가 아니어서 사용자가 쓰는 창의 포커스를 가져가지 않는다."""
    x, y = SLOTS[slot]
    args = [
        '"%s\\Binaries\\Win64\\UnrealEditor.exe"' % ENGINE, '"%s"' % PROJECT, MAP, "-game", "-windowed",
        "-ResX=800", "-ResY=450", "-WinX=%d" % x, "-WinY=%d" % y, "-nosound", "-NoSplash", "-unattended", "-FORCELOGFLUSH",
        # 낮은 그래픽 품질과 60fps 제한: 여러 창과 에디터가 GPU를 나눠 써도 프레임이 일정해야 타이밍 확인을 믿을 수 있다.
        '"-ExecCmds=scalability 0, t.MaxFPS 60"',
        '"-abslog=%s"' % os.path.join(WORK, "slot%d.log" % slot),
        "-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:RemoteExecutionMulticastGroupEndpoint=239.0.0.1:%d" % (6800 + slot),
        '"-ini:Game:[/Script/EngineSettings.GeneralProjectSettings]:ProjectDisplayedTitle=테스트 %d - %s"' % (slot, title.replace('"', "'")),
        '"-ini:Game:[/Script/EngineSettings.GeneralProjectSettings]:ProjectDebugTitleInfo= "',
    ]
    cmd = " ".join(args).replace("'", "''")
    out = powershell("(Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{ CommandLine = '%s' }).ProcessId" % cmd)
    return int(out) if out.isdigit() else 0


def send(slot, code):
    config = remote.RemoteExecutionConfig()
    config.multicast_bind_address = "127.0.0.1"
    config.multicast_group_endpoint = ("239.0.0.1", 6800 + slot)
    config.command_endpoint = ("127.0.0.1", 6820 + slot)
    rx = remote.RemoteExecution(config)
    rx.start()
    try:
        deadline = time.time() + 180
        while not rx.remote_nodes and time.time() < deadline:
            time.sleep(0.5)
        if not rx.remote_nodes:
            return "창이 원격 실행에 응답하지 않음"
        rx.open_command_connection(rx.remote_nodes[0])
        res = rx.run_command(code, unattended=True, exec_mode=remote.MODE_EXEC_FILE, raise_on_failure=False)
        return None if res.get("success") else "시나리오 실행 실패: %s" % res.get("result")
    finally:
        rx.stop()


def top_level_names(source, with_imports):
    """모듈 맨 위의 def, class, 대입(과 import)이 묶는 이름."""
    names = set()
    for node in ast.parse(source).body:
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
            names.add(node.name)
        elif isinstance(node, (ast.Assign, ast.AnnAssign, ast.AugAssign)):
            targets = node.targets if isinstance(node, ast.Assign) else [node.target]
            names.update(n.id for t in targets for n in ast.walk(t) if isinstance(n, ast.Name) and isinstance(n.ctx, ast.Store))
        elif with_imports and isinstance(node, (ast.Import, ast.ImportFrom)):
            names.update((alias.asname or alias.name).split(".")[0] for alias in node.names)
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("scenario")
    ap.add_argument("--hold", type=float, default=10, help="판정을 띄운 채 창을 두는 시간(초)")
    ap.add_argument("--keep", action="store_true", help="창을 닫지 않음. 떠 있는 동안 Build.bat이 실패한다.")
    a = ap.parse_args()

    scenario = open(a.scenario, encoding="utf-8").read()
    runner = open(os.path.join(os.path.dirname(__file__), "runner.py"), encoding="utf-8").read()
    # 시나리오는 실행기와 한 이름 공간에서 돌아, 같은 이름은 오류 없이 실행기 것으로 바뀐다.
    clash = sorted(top_level_names(scenario, False) & (top_level_names(runner, True) | {"OUT"}))
    if clash:
        print("시나리오의 이름이 실행기와 겹침(이름을 바꿀 것): %s" % ", ".join(clash))
        return 2
    title = {}
    exec(compile("".join(l for l in scenario.splitlines(True) if l.startswith("TITLE")), a.scenario, "exec"), title)
    os.makedirs(WORK, exist_ok=True)
    if not wait_for_build():
        print("빌드가 30분 넘게 끝나지 않음")
        return 2
    stale = unbuilt_sources()
    if stale:
        names = ", ".join(os.path.basename(p) for p in stale[:5]) + (" 외 %d개" % (len(stale) - 5) if len(stale) > 5 else "")
        print("빌드 뒤에 고친 C++ 파일은 창에 들어가지 않음: %s" % names)
    slot, lock = take_slot()
    if not slot:
        print("빈 창 자리가 없음(최대 %d개)" % len(SLOTS))
        return 2
    out = os.path.join(WORK, "slot%d_%s.json" % (slot, time.strftime("%Y%m%d-%H%M%S")))
    pid = 0
    try:
        pid = launch(slot, title.get("TITLE", os.path.basename(a.scenario)))
        if not pid:
            print("창을 띄우지 못함")
            return 2
        if not a.keep:
            start_watchdog(pid, slot)
        print("테스트 %d 창 (PID %d): %s" % (slot, pid, title.get("TITLE")))
        error = send(slot, scenario + "\nOUT = r'%s'\n" % out + runner)
        if error:
            print(error)
            return 2
        deadline = time.time() + 600
        while not os.path.exists(out) and time.time() < deadline and pid_alive(pid):
            time.sleep(0.5)
        if not os.path.exists(out):
            print("결과 없음(창이 닫혔거나 시간 초과)")
            return 2
        time.sleep(0.3)
        r = json.load(open(out, encoding="utf-8"))
        print("%s → %s" % (r["title"], r["verdict"]))
        for line in r["lines"]:
            print("  " + line)
        print("  플레이어:", " → ".join(m.replace("AM_Player_", "") for m in r["player_montages"]))
        print("  보스:", " → ".join(m.replace("AM_Boss_", "") for m in r["boss_montages"]))
        print("  %.1f초, 최저 %d fps | %s | 기록: %s" % (r["seconds"], r["fps_min"], " / ".join(r["notes"]), out))
        # 게임 코드의 오류 로그(데이터 파일을 읽지 못함 등)는 판정을 믿을 수 없게 하므로 함께 보여 준다.
        log = open(os.path.join(WORK, "slot%d.log" % slot), encoding="utf-8", errors="replace").read()
        for line in sorted(set(re.findall(r"LogAg: Error: [^\r\n]*", log))):
            print("  게임 로그 오류: " + line)
        if a.keep:
            print("창을 열어 둠(PID %d). 다 보면 닫아야 빌드가 됩니다." % pid)
        else:
            time.sleep(a.hold)
        return {"통과": 0, "테스트 실패": 1, "실행 실패": 2}.get(r["verdict"], 2)
    finally:
        if pid and not a.keep:
            subprocess.run(["taskkill", "/PID", str(pid), "/F"], capture_output=True)
        os.remove(lock)


if __name__ == "__main__":
    try:
        code = main()
    except Exception as e:
        print("실행 실패: %s: %s" % (type(e).__name__, e))
        code = 2
    sys.exit(code)
