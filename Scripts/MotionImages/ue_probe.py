# Copyright Woogle. All Rights Reserved.
"""Runs inside UnrealEditor-Cmd (pythonscript commandlet); motion_images.py probe starts it.

Samples the requested animation sequences at every frame and writes the bone transforms as JSON. It only reads assets,
so it is safe while the editor is open.
"""
import json
import os

import unreal

BONES = ["root", "pelvis", "hand_r"]


def main():
    with open(os.environ["AG_PROBE_REQUEST"], encoding="utf-8") as f:
        request = json.load(f)
    options = unreal.AnimPoseEvaluationOptions()
    options.evaluation_type = unreal.AnimDataEvalType.RAW
    options.extract_root_motion = False

    out = {}
    for clip, path in request["clips"].items():
        anim = unreal.load_asset(path)
        if anim is None:
            out[clip] = {"error": "missing asset " + path}
            continue
        length = unreal.AnimationLibrary.get_sequence_length(anim)
        frames = unreal.AnimationLibrary.get_num_frames(anim)
        rec = {"len": round(length, 4),
               "root_motion": bool(anim.get_editor_property("enable_root_motion")),
               "root_lock": getattr(anim.get_editor_property("root_motion_root_lock"), "name", ""),
               "t": [], "bones": {b: [] for b in BONES}}
        for f in range(frames + 1):
            t = min(f / request["fps"], length)
            pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(anim, t, options)
            rec["t"].append(round(t, 4))
            for b in BONES:
                tr = unreal.AnimPoseExtensions.get_bone_pose(pose, b, unreal.AnimPoseSpaces.WORLD)  # component space
                v, q = tr.translation, tr.rotation
                rec["bones"][b].append([round(v.x, 2), round(v.y, 2), round(v.z, 2), round(q.x, 5), round(q.y, 5), round(q.z, 5), round(q.w, 5)])
        out[clip] = rec

    with open(request["out"], "w", encoding="utf-8") as f:
        json.dump(out, f, separators=(",", ":"))
    unreal.log("[AgMotionImages] probed %d clips" % len(out))


main()
