# Copyright Woogle. All Rights Reserved.
"""Shared pieces of motion_images.py: paths, quaternion math, the two characters and their weapons, and the editor's
MCP connection.

Positions are centimeters in the skeleton's component space (forward +Y, right -X, up +Z); quaternions are (x, y, z, w).
"""
import json
import math
import os
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


def scale(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def norm(a):
    n = math.sqrt(dot(a, a))
    return (a[0] / n, a[1] / n, a[2] / n)


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


def mat_to_quat(x_axis, y_axis, z_axis):
    """Quaternion of the rotation whose columns are the images of the basis vectors."""
    m00, m01, m02 = x_axis[0], y_axis[0], z_axis[0]
    m10, m11, m12 = x_axis[1], y_axis[1], z_axis[1]
    m20, m21, m22 = x_axis[2], y_axis[2], z_axis[2]
    tr = m00 + m11 + m22
    if tr > 0:
        s = math.sqrt(tr + 1.0) * 2
        return ((m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s, 0.25 * s)
    if m00 > m11 and m00 > m22:
        s = math.sqrt(1.0 + m00 - m11 - m22) * 2
        return (0.25 * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s)
    if m11 > m22:
        s = math.sqrt(1.0 + m11 - m00 - m22) * 2
        return ((m01 + m10) / s, 0.25 * s, (m12 + m21) / s, (m02 - m20) / s)
    s = math.sqrt(1.0 + m22 - m00 - m11) * 2
    return ((m02 + m20) / s, (m12 + m21) / s, 0.25 * s, (m10 - m01) / s)


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
class Katana:
    """플레이어 카타나. 게임과 같이 hand_r의 소켓 KatanaSocket에 붙입니다(Samurai 애니메이션 조사 '무기')."""
    bone = "hand_r"
    mesh = "/Game/Fab/ARPG_Pack/ARPG_Samurai/Demo/Weapon/Mesh/katana.katana"
    component = "/Script/Engine.StaticMeshComponent"
    mesh_property = "staticMesh"
    SOCKET_LOC = (-26.83, -26.36, 40.79)   # SK_Mannequin KatanaSocket 위치(hand_r 기준)
    SOCKET_Q = (-0.48580840364876876, -0.315088429192808, -0.014223640429174067, 0.8151730888504507)   # 회전(Pitch 31.84, Yaw 19.46, Roll 67.18)
    TIP = (2.7, -43.2, 0.0)                 # 메시의 칼끝

    @classmethod
    def transform(cls, hr):
        """hand_r 변환에서 메시 피벗(컴포넌트 공간)과 회전을 구합니다."""
        q = tuple(hr[3:7])
        return add(tuple(hr[0:3]), qrot(q, cls.SOCKET_LOC)), qmul(q, cls.SOCKET_Q)

    @classmethod
    def tip(cls, wr):
        pivot, q = cls.transform(wr)
        return add(pivot, qrot(q, cls.TIP))


class Halberd:
    """The boss's halberd on hand_r, on the grip line fitted to the hand (할버드 애니메이션 조사 '무기 부착')."""
    bone = "hand_r"
    mesh = "/Game/Fab/ARPG_Pack/ARPG_Halberd/Demo/Weapon/Mesh/Halberd_weapon.Halberd_weapon"
    component = "/Script/Engine.SkeletalMeshComponent"
    mesh_property = "skeletalMeshAsset"
    # Shaft line in hand_r space: C + K * s, head end at s = HEAD. E: the edge side (wrist to middle knuckle).
    K = (-0.19139550334029393, -0.12210349639582654, 0.9738883393228557)
    C = (-11.185258469870991, 0.855104053233005, 0.6643688625870006)
    E = (-0.9760767334009931, -0.08060814069045838, -0.20193201372658562)
    HEAD = 127.0

    @classmethod
    def transform(cls, hr):
        y_axis = scale(cls.K, -1)                     # mesh head at -Y
        z_axis = scale(cls.E, -1)                     # mesh edge at -Z
        z_axis = norm(sub(z_axis, scale(y_axis, dot(z_axis, y_axis))))
        grip = mat_to_quat(cross(y_axis, z_axis), y_axis, z_axis)
        return to_world(hr, cls.C), qmul(tuple(hr[3:7]), grip)

    @classmethod
    def tip(cls, hr):
        return to_world(hr, add(cls.C, scale(cls.K, cls.HEAD)))


# The temporary capture actor of each character stands far away from the level's content.
CHARACTERS = {
    "player": {"mesh": "/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple", "weapon": Katana, "actor": (0.0, -20000.0, 0.0)},
    "boss": {"mesh": "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple", "weapon": Halberd, "actor": (0.0, 20000.0, 0.0)},
}


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
