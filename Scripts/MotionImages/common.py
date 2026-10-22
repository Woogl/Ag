# Copyright Woogle. All Rights Reserved.
"""motion_images.py가 함께 쓰는 것: 경로, 쿼터니언 계산, 두 캐릭터와 게임 BP에서 재는 무기, 에디터 연결(MCP와 파이썬 원격 실행).

위치는 스켈레톤 컴포넌트 공간의 cm(앞 +Y, 오른쪽 -X, 위 +Z)이고, 쿼터니언은 (x, y, z, w)입니다.
"""
import contextlib
import json
import math
import os
import sys
import time
import urllib.error
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SHEETS = os.path.join(HERE, "sheets")
SAVED = os.path.join(REPO, "Saved", "Tools", "MotionImages")
PROBE_FILE = os.path.join(SAVED, "probe.json")
JOBS_FILE = os.path.join(SAVED, "jobs.json")
FRAMES = os.path.join(SAVED, "frames")
ATTACHMENTS = os.path.join(REPO, "Docs", "Design", "Attachments")
FPS = 60.0


def clip_path(clip):
    """'Samurai/Attack_Combo1' -> the Fab ARPG pack sequence's object path."""
    pack, name = clip.split("/", 1)
    asset = "ARPG_%s_Anim_UE_%s" % (pack, name)
    return "/Game/Fab/ARPG_Pack/ARPG_%s/Animations_UE5/%s.%s" % (pack, asset, asset)


# ------------------------------------------------------------------ math
def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def qconj(q):
    return (-q[0], -q[1], -q[2], q[3])


def qrot(q, v):
    r = qmul(qmul(q, (v[0], v[1], v[2], 0.0)), qconj(q))
    return (r[0], r[1], r[2])


def to_world(tf, v):
    """tf = [x, y, z, qx, qy, qz, qw]; v in tf's local space -> component space."""
    return add(tuple(tf[0:3]), qrot(tuple(tf[3:7]), v))


def quat_to_rotator(q):
    """Unreal's FQuat::Rotator: (pitch, yaw, roll) in degrees."""
    x, y, z, w = q
    sing = z * x - w * y
    yaw_y, yaw_x = 2 * (w * z + x * y), 1 - 2 * (y * y + z * z)
    r2d = 180 / math.pi
    if sing < -0.4999995:
        pitch, yaw = -90.0, math.atan2(yaw_y, yaw_x) * r2d
        roll = -yaw - 2 * math.atan2(x, w) * r2d
    elif sing > 0.4999995:
        pitch, yaw = 90.0, math.atan2(yaw_y, yaw_x) * r2d
        roll = yaw - 2 * math.atan2(x, w) * r2d
    else:
        pitch, yaw = math.asin(2 * sing) * r2d, math.atan2(yaw_y, yaw_x) * r2d
        roll = math.atan2(-2 * (w * x + y * z), 1 - 2 * (x * x + y * y)) * r2d

    def wrap(a):
        a = a % 360
        return a - 360 if a > 180 else a
    return (wrap(pitch), wrap(yaw), wrap(roll))


# ------------------------------------------------------------------ characters and weapons
# 캡처용 임시 캐릭터는 레벨의 물체에서 멀리 떨어진 곳에 둡니다. 무기는 blueprint의 게임 무기를 그대로 씁니다(weapons).
CHARACTERS = {
    "player": {"mesh": "/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple", "blueprint": "/Game/Characters/BP_Player",
               "actor": (0.0, -20000.0, 0.0)},
    "boss": {"mesh": "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple", "blueprint": "/Game/Characters/BP_Boss",
             "actor": (0.0, 20000.0, 0.0)},
}
WEAPON_BONE = "hand_r"   # 무기를 이 본 기준으로 붙입니다. 게임 무기 소켓이 붙은 본입니다.
WEAPON_TIPS = {          # 무기 메시의 날 끝(메시 공간). 메시의 모양이라 붙이는 위치가 바뀌어도 그대로입니다.
    "/Game/Fab/ARPG_Pack/ARPG_Samurai/Demo/Weapon/Mesh/katana.katana": (2.7, -43.2, 0.0),
    "/Game/Fab/ARPG_Pack/ARPG_Halberd/Demo/Weapon/Mesh/Halberd_weapon.Halberd_weapon": (0.0, -127.0, 0.0),
}
WEAPONS_FILE = os.path.join(SAVED, "weapons.json")

# 에디터에서 게임 BP를 잠깐 스폰해, 태그가 Weapon인 컴포넌트(게임이 무기로 찾는 컴포넌트)가 WEAPON_BONE에 대해 어디에
# 붙는지 잽니다. 소켓, 컴포넌트 오프셋, 메시를 바꿔도 게임과 같은 자리가 나옵니다. 원격 실행에 보내는 코드라 '.py'를 쓰지 않습니다.
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
        out[name] = {"mesh": mesh.get_path_name(), "component": weapon.get_class().get_path_name(),
                     "mesh_property": "staticMesh" if static else "skeletalMeshAsset",
                     "loc": [t.x, t.y, t.z], "q": [q.x, q.y, q.z, q.w], "scale": [s.x, s.y, s.z]}
    finally:
        eas.destroy_actor(actor)
print("AG_WEAPONS " + json.dumps(out))
"""


class Weapon:
    """게임 BP에서 잰 무기: 메시, 컴포넌트 종류, WEAPON_BONE 기준 위치·회전·크기."""
    bone = WEAPON_BONE

    def __init__(self, info):
        self.mesh, self.component, self.mesh_property = info["mesh"], info["component"], info["mesh_property"]
        self.loc, self.q, self.scale = tuple(info["loc"]), tuple(info["q"]), tuple(info["scale"])
        if self.mesh not in WEAPON_TIPS:
            raise SystemExit("no blade tip for %s; add it to WEAPON_TIPS in common.py" % self.mesh)
        tip = WEAPON_TIPS[self.mesh]
        self.tip_local = (tip[0] * self.scale[0], tip[1] * self.scale[1], tip[2] * self.scale[2])

    def transform(self, bone):
        """본 변환(컴포넌트 공간)에서 메시 피벗과 회전을 구합니다."""
        return to_world(bone, self.loc), qmul(tuple(bone[3:7]), self.q)

    def tip(self, bone):
        pivot, q = self.transform(bone)
        return add(pivot, qrot(q, self.tip_local))


_weapons = None


def weapons():
    """캐릭터별 Weapon. 처음 부를 때 열린 에디터에서 재고 WEAPONS_FILE에 남깁니다. 에디터에 닿지 못하면 남겨 둔 값을 씁니다."""
    global _weapons
    if _weapons is None:
        blueprints = json.dumps({name: c["blueprint"] for name, c in CHARACTERS.items()})
        lines = editor_python(_WEAPON_CODE % (blueprints, WEAPON_BONE))
        if lines is None:
            if not os.path.exists(WEAPONS_FILE):
                raise SystemExit("The editor is not reachable for the weapon placement; open the project in the editor first.")
            print("editor not reachable; using the weapon placement measured earlier (%s)" % WEAPONS_FILE)
            with open(WEAPONS_FILE, encoding="utf-8") as f:
                data = json.load(f)
        else:
            data = json.loads(next(line for line in lines if line.startswith("AG_WEAPONS "))[len("AG_WEAPONS "):])
            os.makedirs(SAVED, exist_ok=True)
            with open(WEAPONS_FILE, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=1)
        _weapons = {name: Weapon(info) for name, info in data.items()}
    return _weapons


# ------------------------------------------------------------------ editor (unreal-mcp over HTTP)
MCP_URL = "http://127.0.0.1:8000/mcp"
_session_id = None


def _post(payload, session_id=None, timeout=900):
    request = urllib.request.Request(MCP_URL, data=json.dumps(payload).encode("utf-8"), method="POST")
    request.add_header("Content-Type", "application/json")
    request.add_header("Accept", "application/json, text/event-stream")
    if session_id:
        request.add_header("Mcp-Session-Id", session_id)
    with urllib.request.urlopen(request, timeout=timeout) as response:
        body = response.read().decode("utf-8").strip()
        if not body.startswith("{"):   # server-sent events: the last data line holds the reply
            lines = [line[5:].strip() for line in body.splitlines() if line.startswith("data:")]
            body = lines[-1] if lines else "{}"
        return response.headers.get("Mcp-Session-Id"), json.loads(body)


def editor(toolset, tool, args):
    """Calls an editor toolset tool and returns its decoded return value. Raises RuntimeError when it fails."""
    global _session_id
    if _session_id is None:
        try:
            _session_id, _ = _post({"jsonrpc": "2.0", "id": 1, "method": "initialize",
                                    "params": {"protocolVersion": "2025-03-26", "capabilities": {},
                                               "clientInfo": {"name": "ag-motion-images", "version": "1.0"}}}, timeout=30)
        except urllib.error.URLError:
            raise SystemExit("The editor's MCP server is not reachable at %s; open the project in the editor first." % MCP_URL)
        _post({"jsonrpc": "2.0", "method": "notifications/initialized"}, _session_id, timeout=30)
    _, reply = _post({"jsonrpc": "2.0", "id": 2, "method": "tools/call", "params": {
        "name": "call_tool", "arguments": {"toolset_name": toolset, "tool_name": tool, "arguments": args}}}, _session_id)
    result = reply.get("result", {})
    texts = [c.get("text", "") for c in result.get("content", []) if c.get("type") == "text"]
    value = texts[0] if texts else None
    try:
        value = json.loads(value)
        value = value.get("returnValue", value) if isinstance(value, dict) else value
    except (TypeError, ValueError):
        pass
    if "error" in reply or result.get("isError") or (isinstance(value, dict) and "TOOL_ERROR" in value):
        raise RuntimeError("%s.%s failed: %s" % (toolset, tool, reply.get("error") or value))
    return value


# ------------------------------------------------------------------ 에디터 (파이썬 원격 실행)
def engine_root():
    """언리얼 엔진 폴더: UE_ROOT 환경 변수, 없으면 Ag.uproject의 엔진 버전으로 기본 설치 위치."""
    root = os.environ.get("UE_ROOT")
    if not root:
        with open(os.path.join(REPO, "Ag.uproject"), encoding="utf-8") as f:
            root = "C:/Program Files/Epic Games/UE_%s" % json.load(f).get("EngineAssociation", "5.8")
    return root


def editor_python(code, timeout=10.0):
    """이 프로젝트를 연 에디터에서 code를 실행하고 출력 줄들을 돌려줍니다. 에디터에 닿지 못하면 None입니다.
    파일 실행 방식이라 code에 '.py'가 있으면 파일 경로로 읽히므로 쓰지 않습니다."""
    sys.path.append(os.path.join(engine_root(), "Engine", "Plugins", "Experimental", "PythonScriptPlugin", "Content", "Python"))
    import remote_execution as remote
    config = remote.RemoteExecutionConfig()
    config.multicast_bind_address = "127.0.0.1"
    rx = remote.RemoteExecution(config)
    rx.start()
    try:
        deadline, nodes = time.time() + timeout, []
        while not nodes and time.time() < deadline:
            time.sleep(0.2)
            nodes = [n for n in rx.remote_nodes if os.path.normcase(os.path.normpath(n.get("project_root", ""))) == os.path.normcase(REPO)]
        if not nodes:
            return None
        rx.open_command_connection(nodes[0])
        result = rx.run_command(code, unattended=True, exec_mode=remote.MODE_EXEC_FILE, raise_on_failure=False)
    finally:
        rx.stop()
    if not result.get("success"):
        raise RuntimeError("editor Python failed: %s" % result.get("result"))
    return [entry.get("output", "") for entry in result.get("output", [])]


# 캡처하는 동안 에디터를 캡처에 맞춥니다. 창이 뒤에 있어도 느려지지 않게 백그라운드 스로틀('Use Less CPU when in Background',
# 켜져 있으면 초당 3틱 정도로 느려져 요청마다 틱을 기다립니다)을 끄고, 시간에 따라 움직이는 구름(VolumetricCloud)을 숨겨 찍을
# 때마다 하늘과 빛이 같게 합니다. 둘 다 메모리에서만 바꾸고(구름은 아웃라이너의 눈 아이콘과 같은 임시 숨김) 끝나면 되돌립니다.
_CAPTURE_MODE_CODE = """
import json, unreal
want = json.loads(%r)
s = unreal.get_default_object(unreal.load_class(None, "/Script/UnrealEd.EditorPerformanceSettings"))
clouds = [a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if isinstance(a, unreal.VolumetricCloud)]
print("AG_STATE " + json.dumps({"throttle": s.get_editor_property("bThrottleCPUWhenNotForeground"),
                                "clouds": {a.get_path_name(): a.is_temporarily_hidden_in_editor() for a in clouds}}))
s.set_editor_property("bThrottleCPUWhenNotForeground", want["throttle"], unreal.PropertyAccessChangeNotifyMode.NEVER)
for a in clouds:
    a.set_is_temporarily_hidden_in_editor(want["clouds"].get(a.get_path_name(), True))
"""


@contextlib.contextmanager
def editor_capture_mode():
    """이 안에서는 에디터의 백그라운드 스로틀을 끄고 구름을 숨깁니다. 끝나면 원래 상태로 되돌립니다."""
    lines = editor_python(_CAPTURE_MODE_CODE % json.dumps({"throttle": False, "clouds": {}})) or []
    state = next((json.loads(line[len("AG_STATE "):]) for line in lines if line.startswith("AG_STATE ")), None)
    try:
        yield
    finally:
        if state is not None:
            editor_python(_CAPTURE_MODE_CODE % json.dumps(state))
