# Copyright Woogle. All Rights Reserved.
"""Makes the spec motion images (Docs/Design/Attachments/<image>.jpg) from the sheet files in sheets/. See README.md.

    python Scripts/MotionImages/motion_images.py probe                          sample the sheets' motions (about a minute)
    python Scripts/MotionImages/motion_images.py make [image ...]               plan, capture and compose
    python Scripts/MotionImages/motion_images.py compose [image ...] [--out DIR] recompose from the saved frames

plan and capture also run on their own. An image is a sheet file name without .json; none means every sheet.
"""
import argparse
import base64
import glob
import json
import math
import os
import subprocess
import sys

sys.dont_write_bytecode = True   # no __pycache__ in the repo
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import (ATTACHMENTS, CHARACTERS, FPS, FRAMES, JOBS_FILE, PROBE_FILE, REPO, SAVED, SHEETS,  # noqa: E402
                    add, clip_path, editor, qconj, qmul, qrot, quat_to_rotator, sub)

CAMERA_OFFSET = (-430.0, 270.0, 230.0)   # front-right, above; the character faces +Y and its right is -X
CAMERA_HEIGHT = 100.0                     # camera target height above the root
TRAIL_FRAMES = 10                         # a hit frame's trail follows the blade tip over the frames before it

TUNED_WIDTH = 1774                        # capture width the crop size was chosen for; other widths scale it
CROP_W, CROP_H = 600, 660                 # crop around the pelvis, 58% of it above
TILE_W, TILE_H, COLUMNS, GAP = 260, 286, 5, 4
BACKGROUND, TITLE_COLOR, NOTE_COLOR, WHITE = (22, 24, 28), (150, 210, 255), (170, 180, 195), (255, 255, 255)
COLORS = {"yellow": (255, 215, 0), "cyan": (130, 220, 255), "red": (255, 70, 60)}
TRAIL_COLOR = (255, 150, 20)

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
    clips = sorted({f["clip"] for s in load_sheets([]) for r in s["rows"] for f in r["frames"]})
    root = os.environ.get("UE_ROOT")
    if not root:
        root = "C:/Program Files/Epic Games/UE_%s" % load_json(os.path.join(REPO, "Ag.uproject")).get("EngineAssociation", "5.8")
    exe = os.path.join(root, "Engine", "Binaries", "Win64", "UnrealEditor-Cmd.exe")
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


def plan(names):
    """Pose time, weapon placement, blade trail and camera for every frame of the sheets."""
    if not os.path.exists(PROBE_FILE):
        raise SystemExit("no probe data; run: python Scripts/MotionImages/motion_images.py probe")
    probe_data = load_json(PROBE_FILE)
    sheets = load_sheets(names)
    jobs = [j for j in (load_json(JOBS_FILE) if os.path.exists(JOBS_FILE) else []) if j["image"] not in {s["image"] for s in sheets}]
    for sheet in sheets:
        character = CHARACTERS[sheet["character"]]
        weapon, base = character["weapon"], character["actor"]
        for row_index, row in enumerate(sheet["rows"]):
            for idx, frame in enumerate(row["frames"]):
                rec = probe_data.get(frame["clip"])
                if not rec or "error" in rec:
                    raise SystemExit("%s has no probe data; run the probe command" % frame["clip"])
                ts = rec["t"]
                i = min(range(len(ts)), key=lambda k: abs(ts[k] - frame["t"]))
                bones = rec["bones"]
                root, wtf = bones["root"][i], bones[weapon.bone]
                wloc, wq = weapon.transform(wtf[i])
                hit = frame.get("hit", False)
                jobs.append({
                    "image": sheet["image"], "character": sheet["character"], "row": row_index, "idx": idx,
                    "title": row["title"], "note": row.get("note", ""), "anim": clip_path(frame["clip"]), "t": ts[i],
                    "time": frame["time"], "label": frame["label"], "hit": hit, "red": frame.get("red", False),
                    "banner": frame.get("banner", []),
                    "root": rounded(root[:3]) + root[3:7], "root0": rounded(bones["root"][0][:3]) + bones["root"][0][3:7],
                    "root_lock": rec["root_lock"] if rec["root_motion"] else "",
                    "pelvis": rounded(add(base, bones["pelvis"][i][:3])),
                    "wloc": rounded(wloc), "wq": [round(x, 6) for x in wq],
                    "trail": [rounded(add(base, weapon.tip(wtf[k]))) for k in range(max(0, i - TRAIL_FRAMES), i + 1)] if frame.get("trail", hit) else [],
                    "cam": camera((base[0] + root[0], base[1] + root[1], CAMERA_HEIGHT)),
                })
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
    return os.path.join(FRAMES, job["image"], "%d_%02d.png" % (job["row"], job["idx"]))


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


def spawn(character):
    info = CHARACTERS[character]
    weapon = info["weapon"]
    actor = editor(SCENE, "add_to_scene_from_asset", {"asset_path": info["mesh"], "name": "TMP_MotionImage_" + character,
                                                      "xform": {"location": xyz(info["actor"]), "rotation": rotator((0, 0, 0, 1)),
                                                                "scale": xyz((1, 1, 1))}})["refPath"]
    editor(ACTOR, "add_component", {"owner": {"refPath": actor}, "component_type": {"refPath": weapon.component}, "name": "Weapon"})
    editor(OBJ, "set_properties", {"instance": {"refPath": actor + ".Weapon"}, "values": json.dumps({weapon.mesh_property: weapon.mesh})})
    return actor


def capture(names):
    """Poses a temporary mannequin with its weapon for each frame and captures the editor viewport. The actors are
    removed afterwards; in a level with one file per actor (LV_BossStage) nothing is left modified."""
    jobs = jobs_for(names)
    actors = {}
    try:
        for n, job in enumerate(jobs):
            if job["character"] not in actors:
                actors[job["character"]] = spawn(job["character"])
            actor = actors[job["character"]]
            lp, lq = lock_offset(job)
            editor(OBJ, "set_properties", {"instance": {"refPath": actor + ".SkeletalMeshComponent0"}, "values": json.dumps({
                "animationMode": "AnimationSingleNode",
                "animationData": {"animToPlay": job["anim"], "bSavedLooping": False, "bSavedPlaying": False,
                                  "savedPosition": job["t"], "savedPlayRate": 0},
                "relativeLocation": xyz(add(CHARACTERS[job["character"]]["actor"], lp)), "relativeRotation": rotator(lq)})})
            editor(OBJ, "set_properties", {"instance": {"refPath": actor + ".Weapon"}, "values": json.dumps({
                "relativeLocation": xyz(qrot(qconj(lq), sub(job["wloc"], lp))), "relativeRotation": rotator(qmul(qconj(lq), tuple(job["wq"])))})})
            shot = editor(APP, "CaptureViewport", {"captureTransform": job["cam"], "annotations": NO_ANNOTATIONS})
            os.makedirs(os.path.dirname(frame_path(job)), exist_ok=True)
            with open(frame_path(job), "wb") as f:
                f.write(base64.b64decode(shot["image"]["data"]))
            print("\rcaptured %d/%d" % (n + 1, len(jobs)), end="", flush=True)
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


def compose(names, out_dir=ATTACHMENTS):
    from PIL import Image, ImageDraw, ImageFont
    fonts = {key: ImageFont.truetype("C:/Windows/Fonts/" + name, size) for key, name, size in (
        ("title", "malgunbd.ttf", 20), ("note", "malgun.ttf", 17), ("label", "malgun.ttf", 16),
        ("label_hit", "malgunbd.ttf", 16), ("banner", "malgunbd.ttf", 15))}
    jobs = jobs_for(names)
    os.makedirs(out_dir, exist_ok=True)
    for image in sorted({j["image"] for j in jobs}):
        rows = []
        for job in sorted((j for j in jobs if j["image"] == image), key=lambda j: (j["row"], j["idx"])):
            if not os.path.exists(frame_path(job)):
                raise SystemExit("missing frame %s; run the capture command" % frame_path(job))
            if not rows or rows[-1][0]["row"] != job["row"]:
                rows.append([])
            rows[-1].append(job)
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
    parser.add_argument("command", choices=["probe", "plan", "capture", "compose", "make"])
    parser.add_argument("images", nargs="*")
    parser.add_argument("--out", default=ATTACHMENTS, help="compose/make: folder for the images (default Docs/Design/Attachments)")
    args = parser.parse_args()
    if args.command == "probe":
        probe()
    elif args.command == "plan":
        plan(args.images)
    elif args.command == "capture":
        capture(args.images)
    elif args.command == "compose":
        compose(args.images, args.out)
    else:
        images = plan(args.images)
        capture(images)
        compose(images, args.out)


if __name__ == "__main__":
    main()
