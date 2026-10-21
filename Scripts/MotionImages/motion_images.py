# Copyright Woogle. All Rights Reserved.
"""sheets/의 그림 파일로 기획서 모션 그림(Docs/Design/Attachments/<그림 이름>.gif, 칸 그림은 .jpg)을 만듭니다.
쓰는 법은 README.md를 따릅니다.

    python Scripts/MotionImages/motion_images.py probe                          그림 파일의 모션 데이터 뽑기 (1분 정도)
    python Scripts/MotionImages/motion_images.py assemble [image ...]          기존 시퀀스 GIF를 조립
    python Scripts/MotionImages/motion_images.py make [image ...]               GIF는 조립, 칸 그림은 캡처·합성
    python Scripts/MotionImages/motion_images.py compose [image ...] [--out DIR] GIF 조립 또는 캡처한 칸 합성

plan과 capture도 따로 실행할 수 있습니다. image는 .json을 뺀 그림 파일 이름이고, 없으면 모든 그림입니다.
"""
import argparse
import base64
import concurrent.futures
import glob
import json
import math
import os
import shutil
import subprocess
import sys
import time

sys.dont_write_bytecode = True   # no __pycache__ in the repo
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import (ATTACHMENTS, CHARACTERS, FPS, FRAMES, JOBS_FILE, PROBE_FILE, REPO, SAVED, SHEETS,  # noqa: E402
                    add, clip_path, editor, editor_capture_mode, editor_python, engine_root, qconj, qmul, qrot, quat_to_rotator, sub, weapons)

CAMERA_OFFSET = (-430.0, 270.0, 230.0)   # front-right, above; the character faces +Y and its right is -X
CAMERA_HEIGHT = 100.0                     # camera target height above the root
TRAIL_FRAMES = 10                         # a hit frame's trail follows the blade tip over the frames before it

TUNED_WIDTH = 1774                        # capture width the crop size was chosen for; other widths scale it
CROP_W, CROP_H = 600, 660                 # crop around the pelvis, 58% of it above
TILE_W, TILE_H, COLUMNS, GAP = 260, 286, 5, 4
BACKGROUND, TITLE_COLOR, NOTE_COLOR, WHITE = (22, 24, 28), (150, 210, 255), (170, 180, 195), (255, 255, 255)
COLORS = {"yellow": (255, 215, 0), "cyan": (130, 220, 255), "red": (255, 70, 60)}
TRAIL_COLOR = (255, 150, 20)

# 움직이는 그림(GIF)
GIF_FPS = 20
GIF_HEIGHT, GIF_BAR = 420, 30             # 캐릭터를 담는 부분의 높이, 그 위 글자 띠의 높이. 너비는 캐릭터 범위에 맞춥니다.
GIF_HALF_W, GIF_HALF_H = 297, 262         # 골반에서 좌우·위아래로 자르는 거리(TUNED_WIDTH 기준 픽셀). 모든 그림이 같은 틀과 크기입니다.
GIF_HOLD_START, GIF_HOLD_END = 8, 10      # 처음과 끝 자세를 더 보여 줄 프레임 수
GIF_BACKGROUND = (68, 102, 150)           # 하늘 배경을 한 색으로 칠해 파일을 줄입니다
GIF_SKY_TOLERANCE = 12                    # 귀퉁이 하늘색과 채널마다 이만큼 안으로 다른 픽셀을 하늘로 봅니다(하늘의 완만한 그라데이션 포함)
GIF_HIT_SPAN = 0.075                      # 타격 순간 앞뒤로 테두리를 표시할 시간(초)

OBJ, SCENE, ACTOR, APP = ("editor_toolset.toolsets.object.ObjectTools", "editor_toolset.toolsets.scene.SceneTools",
                          "editor_toolset.toolsets.actor.ActorTools", "EditorToolset.EditorAppToolset")
NO_ANNOTATIONS = {"gridSpacing": 0, "gridExtent": 0, "gridHeight": 0, "maxLabelDistance": 0,
                  "classFilter": {"refPath": "/Script/Engine.SkeletalMeshActor"}, "maxLabels": 0}


def load_json(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def save_json(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, separators=(",", ":"))


def load_sheets(names):
    sheets = [load_json(p) for p in sorted(glob.glob(os.path.join(SHEETS, "*.json")))]
    unknown = set(names) - {s["image"] for s in sheets}
    if unknown:
        raise SystemExit("no sheet for: %s" % ", ".join(sorted(unknown)))
    return [s for s in sheets if not names or s["image"] in names]


# ------------------------------------------------------------------ probe
def probe():
    clips = set()
    for s in load_sheets([]):
        clips.update(f["clip"] for r in s.get("rows", []) for f in r["frames"])
        clips.update(seg["clip"] for seg in s.get("animation", {}).get("segments", []))
    clips = sorted(clips)
    exe = os.path.join(engine_root(), "Engine", "Binaries", "Win64", "UnrealEditor-Cmd.exe")
    if not os.path.exists(exe):
        raise SystemExit("UnrealEditor-Cmd.exe not found at %s; set UE_ROOT to the engine folder." % exe)
    out, request, log = (os.path.join(SAVED, n) for n in ("probe_new.json", "probe_request.json", "probe.log"))
    save_json(request, {"clips": {c: clip_path(c) for c in clips}, "fps": FPS, "out": out})
    print("probing %d clips (log: %s)" % (len(clips), log))
    with open(log, "w", encoding="utf-8", errors="replace") as f:
        subprocess.run([exe, os.path.join(REPO, "Ag.uproject"), "-run=pythonscript",
                        "-script=" + os.path.join(os.path.dirname(os.path.abspath(__file__)), "ue_probe.py"),
                        "-EnablePlugins=PythonScriptPlugin", "-unattended", "-nop4", "-nosplash", "-NullRHI", "-stdout"],
                       stdout=f, stderr=subprocess.STDOUT, env=dict(os.environ, AG_PROBE_REQUEST=request))
    if not os.path.exists(out):
        raise SystemExit("the probe wrote nothing; see %s" % log)
    data = load_json(out)
    os.replace(out, PROBE_FILE)
    errors = {c: r["error"] for c, r in data.items() if "error" in r}
    print("probed %d clips -> %s" % (len(data) - len(errors), PROBE_FILE) + ("; errors: %s" % errors if errors else ""))


# ------------------------------------------------------------------ plan
def rounded(v, digits=3):
    return [round(x, digits) for x in v]


def camera(center):
    ox, oy, oz = CAMERA_OFFSET
    return {"location": {"x": center[0] + ox, "y": center[1] + oy, "z": center[2] + oz},
            "rotation": {"pitch": math.degrees(math.atan2(-oz, math.hypot(ox, oy))), "yaw": math.degrees(math.atan2(-oy, -ox)), "roll": 0},
            "scale": {"x": 1, "y": 1, "z": 1}}


def pose_job(sheet, probe_data, clip, t, trail=False):
    """모션 clip의 t초에 가장 가까운 프레임을 캡처할 값(자세 시간, 루트, 칼끝 궤적, 카메라)입니다."""
    base = CHARACTERS[sheet["character"]]["actor"]
    rec = probe_data.get(clip)
    if not rec or "error" in rec:
        raise SystemExit("%s has no probe data; run the probe command" % clip)
    ts = rec["t"]
    i = min(range(len(ts)), key=lambda k: abs(ts[k] - t))
    bones = rec["bones"]
    root, tips = bones["root"][i], []
    if trail:   # 칼끝 궤적은 게임 무기 위치로 계산합니다. 캡처의 무기는 손에 붙어 자세를 따라갑니다(spawn).
        weapon = weapons()[sheet["character"]]
        hand = bones[weapon.bone]
        tips = [rounded(add(base, weapon.tip(hand[k]))) for k in range(max(0, i - TRAIL_FRAMES), i + 1)]
    return {
        "image": sheet["image"], "character": sheet["character"], "anim": clip_path(clip), "t": ts[i],
        "root": rounded(root[:3]) + root[3:7], "root0": rounded(bones["root"][0][:3]) + bones["root"][0][3:7],
        "root_lock": rec["root_lock"] if rec["root_motion"] else "",
        "pelvis": rounded(add(base, bones["pelvis"][i][:3])),
        "trail": tips,
        "cam": camera((base[0] + root[0], base[1] + root[1], CAMERA_HEIGHT)),
    }


def animation_jobs(sheet, probe_data):
    """움직이는 그림의 프레임마다 캡처할 값입니다. 구간을 몽타주처럼 이어 패턴 재생 속도로 재생하고, 프레임마다 예고와
    타격, 구간 표시를 정해 둡니다."""
    anim = sheet["animation"]
    rate = anim.get("play_rate", 1.0)
    segments = []   # (모션, 시작, 끝, 모션 재생 속도(멈춤은 0), 실제 길이)
    for s in anim["segments"]:
        if "hold" in s:   # 자세를 그대로 두는 시간(초). 패턴 재생 속도와 관계없습니다.
            segments.append((s["clip"], s["from"], s["from"], 0.0, s["hold"]))
        else:
            r = s.get("rate", 1.0)
            segments.append((s["clip"], s["from"], s["to"], r, (s["to"] - s["from"]) / r / rate))
    starts, total = [], 0.0
    for seg in segments:
        starts.append(total)
        total += seg[4]

    def real_time(pos, first):
        """모션 위치 {"clip", "t"}가 first번째 구간부터 처음 나오는 곳의 시각. 같은 모션이 두 번 나와도 됩니다."""
        for n in range(first, len(segments)):
            clip, a, b, r, _ = segments[n]
            if clip == pos["clip"] and a - 1e-6 <= pos["t"] <= b + 1e-6:
                return n, starts[n] + ((pos["t"] - a) / r / rate if r else 0.0)
        raise SystemExit("%s: %s %.3f is not in the animation segments" % (sheet["image"], pos["clip"], pos["t"]))

    marks, first = [], 0
    for mark in anim.get("marks", []):
        first, at = real_time(mark, first)
        marks.append((at, mark))
    spans = []
    for span in anim.get("spans", []):
        n, begin = real_time(span["from"], 0)
        spans.append((begin, real_time(span["to"], n)[1], span))
    jobs = []
    eps = 1e-6   # 구간(spans) 경계가 프레임 시각과 같을 때 소수점 오차로 한 프레임 밀리지 않게 합니다
    for k in range(-GIF_HOLD_START, int(round(total * GIF_FPS)) + 1 + GIF_HOLD_END):
        time = min(max(k / GIF_FPS, 0.0), total)
        n = max(m for m in range(len(segments)) if starts[m] <= time + 1e-9)
        clip, a, b, r, _ = segments[n]
        job = pose_job(sheet, probe_data, clip, min(a + (time - starts[n]) * rate * r, b))
        label, color, border = "", "", False
        for begin, end, span in spans:
            if begin - eps <= time <= end + eps:
                label, color, border = span["label"], span.get("color", "yellow"), span.get("border", False)
        for at, mark in marks:
            mark_color = "red" if mark.get("red") else mark.get("color", "yellow")
            if at - mark.get("lead", 0.0) <= time < at:
                label, color = mark.get("lead_label", ""), mark_color
            if abs(time - at) <= GIF_HIT_SPAN:
                label, color, border = mark["label"], mark_color, True
        job.update({"gif": True, "idx": len(jobs), "time": "%.2f초" % time, "label": label, "color": color, "border": border})
        jobs.append(job)
    return jobs


def plan(names):
    """그림 파일의 칸마다 자세 시간, 무기 위치, 칼끝 궤적, 카메라를 정합니다. 움직이는 그림이 있으면 그 프레임도 정합니다."""
    if not os.path.exists(PROBE_FILE):
        raise SystemExit("no probe data; run: python Scripts/MotionImages/motion_images.py probe")
    probe_data = load_json(PROBE_FILE)
    sheets = load_sheets(names)
    alive = {s["image"] for s in load_sheets([])}   # 그림 파일을 지운 그림의 작업은 버립니다
    jobs = [j for j in (load_json(JOBS_FILE) if os.path.exists(JOBS_FILE) else [])
            if j["image"] in alive and j["image"] not in {s["image"] for s in sheets}]
    for sheet in sheets:
        for row_index, row in enumerate(sheet.get("rows", [])):
            for idx, frame in enumerate(row["frames"]):
                hit = frame.get("hit", False)
                job = pose_job(sheet, probe_data, frame["clip"], frame["t"], frame.get("trail", hit))
                job.update({"row": row_index, "idx": idx, "title": row["title"], "note": row.get("note", ""),
                            "time": frame["time"], "label": frame["label"], "hit": hit, "red": frame.get("red", False),
                            "banner": frame.get("banner", [])})
                jobs.append(job)
        if "animation" in sheet:
            jobs.extend(animation_jobs(sheet, probe_data))
    save_json(JOBS_FILE, jobs)
    print("planned %d sheets -> %s" % (len(sheets), JOBS_FILE))
    return [s["image"] for s in sheets]


def jobs_for(names):
    if not os.path.exists(JOBS_FILE):
        raise SystemExit("no jobs; run the plan command first")
    jobs = load_json(JOBS_FILE)
    missing = set(names) - {j["image"] for j in jobs}
    if missing:
        raise SystemExit("not planned yet: %s" % ", ".join(sorted(missing)))
    return [j for j in jobs if not names or j["image"] in names]


def frame_path(job):
    name = "gif_%03d.png" % job["idx"] if job.get("gif") else "%d_%02d.png" % (job["row"], job["idx"])
    return os.path.join(FRAMES, job["image"], name)


# ------------------------------------------------------------------ capture
def lock_offset(job):
    """With root motion on, the editor preview draws the root locked; this is the transform to give the actor instead
    so the body stands where the root is."""
    p, q = tuple(job["root"][:3]), tuple(job["root"][3:])
    if job["root_lock"] in ("REF_POSE", "ZERO"):
        return p, q
    if job["root_lock"] == "ANIM_FIRST_FRAME":
        q = qmul(q, qconj(tuple(job["root0"][3:])))
        return sub(p, qrot(q, tuple(job["root0"][:3]))), q
    return (0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0)


def xyz(v):
    return {"x": v[0], "y": v[1], "z": v[2]}


def rotator(q):
    pitch, yaw, roll = quat_to_rotator(q)
    return {"pitch": pitch, "yaw": yaw, "roll": roll}


# 임시 마네킹의 무기를 손 본에 게임과 같은 위치로 붙입니다. 원격 실행에 보내는 코드라 '.py'를 쓰지 않습니다.
_ATTACH_CODE = """
import unreal
actor = unreal.find_object(None, %r)
weapon = [c for c in actor.get_components_by_class(unreal.SceneComponent) if c.get_name() == "Weapon"][0]
rule = unreal.AttachmentRule.KEEP_RELATIVE
weapon.attach_to_component(actor.get_editor_property("skeletal_mesh_component"), %r, rule, rule, rule, False)
weapon.set_relative_transform(unreal.Transform(unreal.Vector(*%r), unreal.Quat(*%r).rotator(), unreal.Vector(*%r)), False, False)
"""


def spawn(character):
    """임시 마네킹을 놓고 게임과 같은 무기를 손에 붙입니다. 무기는 자세를 따라 움직이므로 프레임마다 옮기지 않습니다."""
    info, weapon = CHARACTERS[character], weapons()[character]
    actor = editor(SCENE, "add_to_scene_from_asset", {"asset_path": info["mesh"], "name": "TMP_MotionImage_" + character,
                                                      "xform": {"location": xyz(info["actor"]), "rotation": rotator((0, 0, 0, 1)),
                                                                "scale": xyz((1, 1, 1))}})["refPath"]
    try:
        editor(ACTOR, "add_component", {"owner": {"refPath": actor}, "component_type": {"refPath": weapon.component}, "name": "Weapon"})
        editor(OBJ, "set_properties", {"instance": {"refPath": actor + ".Weapon"}, "values": json.dumps({weapon.mesh_property: weapon.mesh})})
        if editor_python(_ATTACH_CODE % (actor, weapon.bone, list(weapon.loc), list(weapon.q), list(weapon.scale))) is None:
            raise SystemExit("The editor's Python remote execution is not reachable; check that the project's editor is open.")
    except BaseException:
        editor(SCENE, "remove_from_scene", {"actor": {"refPath": actor}})
        raise
    return actor


def capture(names):
    """프레임마다 임시 마네킹에 자세를 주고 에디터 뷰포트를 캡처합니다. 자세와 카메라가 같은 프레임(처음·끝 멈춤, 자세 유지)은
    한 번만 찍어 복사합니다. 캡처하는 동안은 에디터를 캡처 모드(스로틀 끔, 구름 숨김)로 둡니다. 끝나면 임시 마네킹을 지우며,
    액터마다 파일을 두는 레벨(LV_BossStage)에는 바뀐 것이 남지 않습니다."""
    jobs = jobs_for(names)
    actors, shots, start = {}, {}, time.time()
    with editor_capture_mode():
        try:
            for n, job in enumerate(jobs):
                path = frame_path(job)
                os.makedirs(os.path.dirname(path), exist_ok=True)
                key = json.dumps([job[k] for k in ("character", "anim", "t", "root", "root0", "root_lock", "cam")])
                if key in shots:
                    shutil.copyfile(shots[key], path)
                else:
                    if job["character"] not in actors:
                        actors[job["character"]] = spawn(job["character"])
                    lp, lq = lock_offset(job)
                    editor(OBJ, "set_properties", {"instance": {"refPath": actors[job["character"]] + ".SkeletalMeshComponent0"}, "values": json.dumps({
                        "animationMode": "AnimationSingleNode",
                        "animationData": {"animToPlay": job["anim"], "bSavedLooping": False, "bSavedPlaying": False,
                                          "savedPosition": job["t"], "savedPlayRate": 0},
                        "relativeLocation": xyz(add(CHARACTERS[job["character"]]["actor"], lp)), "relativeRotation": rotator(lq)})})
                    shot = editor(APP, "CaptureViewport", {"captureTransform": job["cam"], "annotations": NO_ANNOTATIONS})
                    with open(path, "wb") as f:
                        f.write(base64.b64decode(shot["image"]["data"]))
                    shots[key] = path
                print("\rcaptured %d/%d (%d shots, %.0fs)" % (n + 1, len(jobs), len(shots), time.time() - start), end="", flush=True)
            print()
        finally:
            for actor in actors.values():
                editor(SCENE, "remove_from_scene", {"actor": {"refPath": actor}})


# ------------------------------------------------------------------ compose
def project(p, cam, width, height):
    """World point -> pixel in a capture taken from cam (horizontal field of view 90°)."""
    loc = (cam["location"]["x"], cam["location"]["y"], cam["location"]["z"])
    pitch, yaw = math.radians(cam["rotation"]["pitch"]), math.radians(cam["rotation"]["yaw"])
    fwd = (math.cos(pitch) * math.cos(yaw), math.cos(pitch) * math.sin(yaw), math.sin(pitch))
    right = (-math.sin(yaw), math.cos(yaw), 0.0)
    up = (-math.sin(pitch) * math.cos(yaw), -math.sin(pitch) * math.sin(yaw), math.cos(pitch))
    v = sub(p, loc)
    z = sum(v[i] * fwd[i] for i in range(3))
    return (width / 2 + sum(v[i] * right[i] for i in range(3)) / z * (width / 2),
            height / 2 - sum(v[i] * up[i] for i in range(3)) / z * (width / 2))


def tile(job, fonts):
    from PIL import Image, ImageDraw
    im = Image.open(frame_path(job)).convert("RGB")
    width, height = im.size
    s = width / TUNED_WIDTH
    cx, cy = project(job["pelvis"], job["cam"], width, height)
    if job["trail"]:
        draw = ImageDraw.Draw(im, "RGBA")
        pts = [project(p, job["cam"], width, height) for p in job["trail"]]
        for q in range(1, len(pts)):
            draw.line([pts[q - 1], pts[q]], fill=TRAIL_COLOR + (int(60 + 195 * q / (len(pts) - 1)),), width=max(1, round(7 * s)))
    im = im.crop((int(cx - CROP_W * s / 2), int(cy - CROP_H * s * 0.58), int(cx + CROP_W * s / 2), int(cy + CROP_H * s * 0.42)))
    im = im.resize((TILE_W, TILE_H), Image.LANCZOS)
    draw = ImageDraw.Draw(im)
    hit_color = COLORS["red"] if job["red"] else COLORS["yellow"]
    text = job["time"] + ("  " + job["label"] if job["label"] else "")
    font = fonts["label_hit"] if job["hit"] else fonts["label"]
    draw.rectangle((0, 0, draw.textlength(text, font=font) + 10, 24), fill=(0, 0, 0))
    draw.text((5, 1), text, font=font, fill=hit_color if job["hit"] else WHITE)
    if job["banner"]:
        overlay = ImageDraw.Draw(im, "RGBA")
        h = 20 * len(job["banner"]) + 8
        overlay.rectangle((0, TILE_H - h, TILE_W, TILE_H), fill=(0, 0, 0, 200))
        for n, (line, color) in enumerate(job["banner"]):
            overlay.text((7, TILE_H - h + 3 + n * 20), line, font=fonts["banner"], fill=COLORS[color])
    if job["hit"]:
        ImageDraw.Draw(im).rectangle((0, 0, TILE_W - 1, TILE_H - 1), outline=hit_color, width=4)
    return im


def compose_gif(image, jobs, fonts, out_dir):
    """움직이는 그림을 합성합니다. 골반(몸 가운데)을 가운데 두고 모든 그림을 같은 틀로 자르고, 하늘 배경을 한 색으로
    칠하고 프레임마다 팔레트를 잡아 파일을 작게 만듭니다. 크게 휘두른 무기 끝은 잠깐 잘릴 수 있습니다."""
    from PIL import Image, ImageChops, ImageDraw
    for job in jobs:
        if not os.path.exists(frame_path(job)):
            raise SystemExit("missing frame %s; run the capture command" % frame_path(job))
    left, top, right, bottom = -GIF_HALF_W, -GIF_HALF_H, GIF_HALF_W, GIF_HALF_H
    out_w = int(round(GIF_HEIGHT * (right - left) / (bottom - top)))
    frames = []
    for job in jobs:
        im = Image.open(frame_path(job)).convert("RGB")
        width, height = im.size
        s = width / TUNED_WIDTH
        cx, cy = project(job["pelvis"], job["cam"], width, height)
        im = im.crop((int(cx + left * s), int(cy + top * s), int(cx + right * s), int(cy + bottom * s))).resize((out_w, GIF_HEIGHT), Image.LANCZOS)
        # 하늘: 네 귀퉁이 하늘색(가운데 값이라 무기가 한 귀퉁이에 걸려도 됨)에 가까운 픽셀. 하늘을 비추는 칼날이나 몸의
        # 푸른 음영은 하늘색과 달라 남습니다.
        corners = [im.getpixel(p) for p in ((2, 2), (out_w - 3, 2), (2, GIF_HEIGHT - 3), (out_w - 3, GIF_HEIGHT - 3))]
        sky_color = tuple(sorted(c[i] for c in corners)[1] for i in range(3))
        r, g, b = ImageChops.difference(im, Image.new("RGB", im.size, sky_color)).split()
        sky = ImageChops.lighter(ImageChops.lighter(r, g), b)
        im.paste(GIF_BACKGROUND, mask=sky.point(lambda v: 255 if v <= GIF_SKY_TOLERANCE else 0))
        frame = Image.new("RGB", (out_w, GIF_BAR + GIF_HEIGHT), (0, 0, 0))
        frame.paste(im, (0, GIF_BAR))
        draw = ImageDraw.Draw(frame, "RGBA")
        color = COLORS.get(job["color"], WHITE)
        name = job["anim"].rsplit(".", 1)[-1]   # 지금 재생 중인 모션의 에셋 이름
        name_y = GIF_BAR + GIF_HEIGHT - 6   # 이름 띠의 아래 끝. 타격 순간의 테두리(6px) 안쪽입니다.
        draw.rectangle((6, name_y - 24, draw.textlength(name, font=fonts["gif_asset"]) + 18, name_y), fill=(0, 0, 0, 170))
        draw.text((12, name_y - 22), name, font=fonts["gif_asset"], fill=WHITE)
        draw.text((7, 5), job["time"], font=fonts["label"], fill=WHITE)
        if job["label"]:
            draw.text((80, 3), job["label"], font=fonts["gif_label"], fill=color)
        if job["border"]:
            draw.rectangle((0, 0, out_w - 1, GIF_BAR + GIF_HEIGHT - 1), outline=color, width=6)
        frames.append(frame.quantize(colors=64, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE))
    path = os.path.join(out_dir, image + ".gif")
    frames[0].save(path, save_all=True, append_images=frames[1:], duration=1000 // GIF_FPS, loop=0, optimize=True)
    print(path)


def compose(names, out_dir=ATTACHMENTS):
    from PIL import Image, ImageDraw, ImageFont
    fonts = {key: ImageFont.truetype("C:/Windows/Fonts/" + name, size) for key, name, size in (
        ("title", "malgunbd.ttf", 20), ("note", "malgun.ttf", 17), ("label", "malgun.ttf", 16),
        ("label_hit", "malgunbd.ttf", 16), ("banner", "malgunbd.ttf", 15), ("gif_label", "malgunbd.ttf", 19),
        ("gif_asset", "malgun.ttf", 14))}
    jobs = jobs_for(names)
    os.makedirs(out_dir, exist_ok=True)
    for image in sorted({j["image"] for j in jobs}):
        gif = sorted((j for j in jobs if j["image"] == image and j.get("gif")), key=lambda j: j["idx"])
        if gif:
            compose_gif(image, gif, fonts, out_dir)
        rows = []
        for job in sorted((j for j in jobs if j["image"] == image and not j.get("gif")), key=lambda j: (j["row"], j["idx"])):
            if not os.path.exists(frame_path(job)):
                raise SystemExit("missing frame %s; run the capture command" % frame_path(job))
            if not rows or rows[-1][0]["row"] != job["row"]:
                rows.append([])
            rows[-1].append(job)
        if not rows:   # 칸 그림 없이 움직이는 그림만 있는 그림 파일
            continue
        sheet = Image.new("RGB", (min(COLUMNS, max(len(r) for r in rows)) * (TILE_W + GAP) + GAP,
                                  8 + sum(34 + math.ceil(len(r) / COLUMNS) * (TILE_H + GAP) for r in rows)), BACKGROUND)
        draw = ImageDraw.Draw(sheet)
        y = 6
        for row in rows:
            draw.text((8, y), row[0]["title"], font=fonts["title"], fill=TITLE_COLOR)
            if row[0]["note"]:
                x = 8 + draw.textlength(row[0]["title"], font=fonts["title"]) + 14
                draw.text((x, y + 2), "· " + row[0]["note"], font=fonts["note"], fill=NOTE_COLOR)
            y += 34
            for k, job in enumerate(row):
                sheet.paste(tile(job, fonts), (GAP + (k % COLUMNS) * (TILE_W + GAP), y + (k // COLUMNS) * (TILE_H + GAP)))
            y += math.ceil(len(row) / COLUMNS) * (TILE_H + GAP)
        path = os.path.join(out_dir, image + ".jpg")
        sheet.save(path, quality=88, optimize=True)
        print(path)


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["probe", "plan", "capture", "compose", "make", "assemble"])
    parser.add_argument("images", nargs="*")
    parser.add_argument("--out", default=ATTACHMENTS, help="compose/make: folder for the images (default Docs/Design/Attachments)")
    args = parser.parse_args()
    if args.command in ("make", "compose", "assemble"):
        from assemble import assemble_sheet
        sheets = load_sheets(args.images)
        if args.command == "assemble" or not any(sheet.get("rows") for sheet in sheets):
            for sheet in sheets:
                if "animation" in sheet:
                    assemble_sheet(sheet, args.out, GIF_FPS, GIF_HOLD_START, GIF_HOLD_END)
            return
    if args.command == "probe":
        probe()
    elif args.command == "plan":
        plan(args.images)
    elif args.command == "capture":
        capture(args.images)
    elif args.command == "compose":
        compose(args.images, args.out)
    else:
        # 그림마다 캡처가 끝나는 대로 합성해 앞 그림부터 결과가 나오고, 합성은 다음 그림을 캡처하는 동안 함께 진행합니다.
        with concurrent.futures.ThreadPoolExecutor() as pool:
            composing = []
            for image in plan(args.images):
                capture([image])
                composing.append(pool.submit(compose, [image], args.out))
            for job in composing:
                job.result()


if __name__ == "__main__":
    main()
