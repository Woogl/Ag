# Copyright Woogle. All Rights Reserved.
"""에디터 내부에서 실행합니다. 외부 실행기는 start에 작업 파일을 한 번 전달합니다."""
import json
import math
import sys
import time
import traceback
from pathlib import Path

import unreal

from settings import CAMERA_OFFSET, CAMERA_ZOOM, CROP_FRACTION, FPS, HEIGHT, PACK_WEAPONS, RENDER_SCALE, ROOT, WARMUP_FRAMES, WIDTH

sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "Scripts" / "MotionImages"))
from common import CHARACTERS, add, qconj, qmul, qrot, sub

_active = None


def atomic_json(path, value):
    temporary = path.with_suffix(".tmp")
    temporary.write_text(json.dumps(value), encoding="utf-8")
    # Windows에서는 상태 파일을 교체하는 순간 읽기 핸들과 잠깐 충돌할 수 있습니다.
    for attempt in range(10):
        try:
            temporary.replace(path)
            break
        except PermissionError:
            if attempt == 9:
                raise
            time.sleep(0.02)


def inventory():
    assets = unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_class(
        unreal.TopLevelAssetPath("/Script/Engine", "AnimSequence"), True)
    # 다른 스켈레톤의 시퀀스(Dodge Animations 팩의 UE4 마네킹용 원본)는 프로젝트 마네킹에서 재생되지 않아 뺍니다.
    skeleton = "/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"
    return sorted(str(asset.package_name) for asset in assets
                  if str(asset.package_name).startswith("/Game/") and skeleton in str(asset.get_tag_value("Skeleton")))


class Capture:
    def __init__(self, manifest):
        self.folder = Path(manifest).parent
        self.jobs = json.loads(Path(manifest).read_text(encoding="utf-8"))
        self.eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        self.world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        self.actors = []
        self.characters = {}
        self.handle = None
        self.settings = None
        self.throttle = None
        self.sequence_index = 0
        self.frame_index = 0
        self.captured = 0
        self.phase = "sequence"
        self.started = time.perf_counter()
        self.last_status = 0

    def begin(self):
        self.settings = unreal.get_default_object(unreal.load_class(None, "/Script/UnrealEd.EditorPerformanceSettings"))
        self.throttle = self.settings.get_editor_property("bThrottleCPUWhenNotForeground")
        self.settings.set_editor_property("bThrottleCPUWhenNotForeground", False, unreal.PropertyAccessChangeNotifyMode.NEVER)
        self.camera = self.spawn(unreal.SceneCapture2D, (0, 0, 0))
        self.capture = self.camera.get_component_by_class(unreal.SceneCaptureComponent2D)
        self.capture.set_editor_property("capture_every_frame", False)
        self.capture.set_editor_property("capture_on_movement", False)
        self.capture.set_editor_property("always_persist_rendering_state", True)
        self.capture.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
        self.capture.set_editor_property("primitive_render_mode", unreal.SceneCapturePrimitiveRenderMode.PRM_USE_SHOW_ONLY_LIST)
        self.capture.set_editor_property("fov_angle", math.degrees(2 * math.atan(CROP_FRACTION / CAMERA_ZOOM)))
        post = unreal.PostProcessSettings()
        post.set_editor_property("override_auto_exposure_method", True)
        post.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
        post.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
        post.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
        post.set_editor_property("override_auto_exposure_bias", True)
        post.set_editor_property("auto_exposure_bias", 2.0)
        self.capture.set_editor_property("post_process_settings", post)
        self.capture.set_editor_property("show_flag_settings", [
            unreal.EngineShowFlagsSetting(show_flag_name=name, enabled=False)
            for name in ("MotionBlur", "TemporalAA", "Fog", "VolumetricFog", "Cloud")
        ])
        self.target = unreal.RenderingLibrary.create_render_target2d(self.world, WIDTH * RENDER_SCALE, HEIGHT * RENDER_SCALE, unreal.TextureRenderTargetFormat.RTF_RGBA8)
        self.capture.set_editor_property("texture_target", self.target)
        self.mask_camera = self.spawn(unreal.SceneCapture2D, (0, 0, 0))
        self.mask_capture = self.mask_camera.get_component_by_class(unreal.SceneCaptureComponent2D)
        self.mask_capture.set_editor_property("capture_every_frame", False)
        self.mask_capture.set_editor_property("capture_on_movement", False)
        self.mask_capture.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_SCENE_COLOR_HDR)
        self.mask_capture.set_editor_property("primitive_render_mode", unreal.SceneCapturePrimitiveRenderMode.PRM_USE_SHOW_ONLY_LIST)
        self.mask_capture.set_editor_property("fov_angle", self.capture.get_editor_property("fov_angle"))
        self.mask_capture.set_editor_property("show_flag_settings", [
            unreal.EngineShowFlagsSetting(show_flag_name=name, enabled=False)
            for name in ("Lighting", "PostProcessing", "MotionBlur", "TemporalAA", "Atmosphere", "Fog", "VolumetricFog", "Cloud")
        ])
        self.mask_target = unreal.RenderingLibrary.create_render_target2d(self.world, WIDTH * RENDER_SCALE, HEIGHT * RENDER_SCALE, unreal.TextureRenderTargetFormat.RTF_RGBA8)
        self.mask_capture.set_editor_property("texture_target", self.mask_target)
        self.handle = unreal.register_slate_post_tick_callback(self.tick)
        self.status("running", force=True)

    def spawn(self, cls, location):
        actor = self.eas.spawn_actor_from_class(cls, unreal.Vector(*location), unreal.Rotator(), True)
        if actor is None:
            raise RuntimeError("Failed to spawn capture actor")
        self.actors.append(actor)
        actor.set_actor_label("TMP_AnimationGif_" + cls.__name__, False)
        return actor

    def character(self, name):
        if name in self.characters:
            return self.characters[name]
        info = CHARACTERS[name]
        actor = self.spawn(unreal.SkeletalMeshActor, info["actor"])
        mesh = actor.get_editor_property("skeletal_mesh_component")
        mesh.set_skeletal_mesh_asset(unreal.load_asset(info["mesh"]))
        mesh.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        mesh.set_update_animation_in_editor(True)
        mesh.set_editor_property("visibility_based_anim_tick_option", unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
        mesh.set_editor_property("enable_update_rate_optimizations", False)
        weapon_info = self.jobs["weapons"][name]
        weapon_mesh = unreal.load_asset(weapon_info["mesh"])
        skeletal = isinstance(weapon_mesh, unreal.SkeletalMesh)
        weapon = self.spawn(unreal.SkeletalMeshActor if skeletal else unreal.StaticMeshActor, info["actor"])
        component = weapon.get_component_by_class(unreal.SkeletalMeshComponent if skeletal else unreal.StaticMeshComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        if skeletal:
            component.set_skeletal_mesh_asset(weapon_mesh)
        else:
            component.set_static_mesh(weapon_mesh)
        rule = unreal.AttachmentRule.KEEP_RELATIVE
        component.attach_to_component(mesh, "hand_r", rule, rule, rule, False)
        component.set_relative_transform(unreal.Transform(unreal.Vector(*weapon_info["loc"]), unreal.Quat(*weapon_info["q"]).rotator(), unreal.Vector(*weapon_info["scale"])), False, False)
        equipment = [(weapon, component, "game")]
        if name == "boss":
            for pack, definitions in PACK_WEAPONS.items():
                for asset, bone, location in definitions:
                    extra = self.spawn(unreal.StaticMeshActor, info["actor"])
                    part = extra.get_component_by_class(unreal.StaticMeshComponent)
                    part.set_mobility(unreal.ComponentMobility.MOVABLE)
                    part.set_static_mesh(unreal.load_asset("/Game/Fab/ARPG_Pack/ARPG_" + pack + "/Demo/Weapon/Mesh/" + asset))
                    part.attach_to_component(mesh, bone, rule, rule, rule, False)
                    part.set_relative_transform(unreal.Transform(unreal.Vector(*location), unreal.Quat(-0.089812, 0.143877, 0.059294, -0.983726).rotator(), unreal.Vector(1, 1, 1)), False, False)
                    equipment.append((extra, part, pack))
        self.characters[name] = (actor, mesh, equipment)
        return self.characters[name]

    def prepare_sequence(self):
        job = self.jobs["sequences"][self.sequence_index]
        self.path = job["path"]
        self.sequence_dir = self.folder / ("%05d" % self.sequence_index)
        self.sequence_dir.mkdir(exist_ok=True)
        self.animation = unreal.load_asset(self.path)
        if not isinstance(self.animation, unreal.AnimSequence):
            raise RuntimeError("Not an AnimSequence: " + self.path)
        self.length = unreal.AnimationLibrary.get_sequence_length(self.animation)
        self.times = [min(i / FPS, self.length) for i in range(math.ceil(self.length * FPS) + 1)]
        player = "ARPG_Samurai" in self.path or "/Dodge_and_Evade_Anims/" in self.path or "/Fab/Dodge_Animations/" in self.path
        self.character_name = "player" if player else "boss"
        self.actor, self.mesh, equipment = self.character(self.character_name)
        visible = [self.actor]
        for weapon, component, pack in equipment:
            enabled = "no_weapon" not in self.path and ((pack == "game" and (player or "ARPG_Halberd" in self.path)) or (pack != "game" and "/ARPG_" + pack + "/" in self.path))
            component.set_visibility(enabled, True)
            if enabled:
                visible.append(weapon)
        self.capture.clear_show_only_components()
        self.mask_capture.clear_show_only_components()
        for visible_actor in visible:
            self.capture.show_only_actor_components(visible_actor)
            self.mask_capture.show_only_actor_components(visible_actor)
        self.mesh.set_animation(self.animation)
        self.mesh.set_play_rate(0)
        self.mesh.stop()
        self.options = unreal.AnimPoseEvaluationOptions()
        self.options.evaluation_type = unreal.AnimDataEvalType.RAW
        self.options.extract_root_motion = False
        self.root_lock = self.animation.get_editor_property("root_motion_root_lock").name if (
            self.animation.get_editor_property("enable_root_motion") or self.animation.get_editor_property("force_root_lock")) else ""
        first_pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(self.animation, 0, self.options)
        self.root0 = self.bone(first_pose, "root")
        self.frame_index = 0
        self.sequence_started = time.perf_counter()
        self.phase = "pose"

    def bone(self, pose, name):
        transform = unreal.AnimPoseExtensions.get_bone_pose(pose, name, unreal.AnimPoseSpaces.WORLD)
        p, q = transform.translation, transform.rotation
        return (p.x, p.y, p.z), (q.x, q.y, q.z, q.w)

    def pose(self):
        moment = self.times[self.frame_index]
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(self.animation, moment, self.options)
        position, rotation = self.bone(pose, "root")
        if self.root_lock == "ANIM_FIRST_FRAME":
            rotation = qmul(rotation, qconj(self.root0[1]))
            position = sub(position, qrot(rotation, self.root0[0]))
        elif self.root_lock not in ("REF_POSE", "ZERO"):
            position, rotation = (0, 0, 0), (0, 0, 0, 1)
        origin = CHARACTERS[self.character_name]["actor"]
        self.mesh.set_world_transform(unreal.Transform(unreal.Vector(*add(origin, position)), unreal.Quat(*rotation).rotator(), unreal.Vector(1, 1, 1)), False, False)
        self.mesh.set_position(moment, False)
        pelvis, _ = self.bone(pose, "pelvis")
        center = add(origin, pelvis)
        ox, oy, oz = CAMERA_OFFSET
        orientation = unreal.Rotator(pitch=math.degrees(math.atan2(-oz, math.hypot(ox, oy))), yaw=math.degrees(math.atan2(-oy, -ox)), roll=0)
        self.camera.set_actor_location_and_rotation(unreal.Vector(*add(center, CAMERA_OFFSET)), orientation, False, True)
        self.mask_camera.set_actor_location_and_rotation(unreal.Vector(*add(center, CAMERA_OFFSET)), orientation, False, True)
        # SetPosition만으로 뼈 행렬이 갱신되지 않으므로 다음 에디터 틱을 거칩니다.
        self.warmup_remaining = WARMUP_FRAMES if self.frame_index == 0 else 0
        self.phase = "capture"

    def tick(self, delta):
        try:
            if (self.folder / "stop").exists():
                self.finish("cancelled")
                return
            if time.time() - (self.folder / "heartbeat").stat().st_mtime > 60:
                raise RuntimeError("Capture client heartbeat expired")
            if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is not None:
                raise RuntimeError("PIE started during capture")
            if self.phase == "sequence":
                if self.sequence_index >= len(self.jobs["sequences"]):
                    self.finish("complete")
                    return
                pending = sum(1 for ready in self.folder.glob("*/ready.json") if not (ready.parent / "composed.json").exists())
                if pending < self.jobs["max_pending"]:
                    self.prepare_sequence()
            elif self.phase == "pose":
                self.pose()
            else:
                self.capture.capture_scene()
                # 첫 렌더는 노출·반사 상태가 아직 초기값이라 결과에 넣지 않습니다.
                if self.warmup_remaining:
                    self.warmup_remaining -= 1
                    return
                filename = "%06d.png" % self.frame_index
                unreal.RenderingLibrary.export_render_target(self.world, self.target, str(self.sequence_dir), filename)
                if not (self.sequence_dir / filename).is_file():
                    raise RuntimeError("Render target export failed: " + filename)
                self.mask_capture.capture_scene()
                mask_filename = "%06d.mask.png" % self.frame_index
                unreal.RenderingLibrary.export_render_target(self.world, self.mask_target, str(self.sequence_dir), mask_filename)
                if not (self.sequence_dir / mask_filename).is_file():
                    raise RuntimeError("Opacity export failed: " + mask_filename)
                self.captured += 1
                self.frame_index += 1
                if self.frame_index == len(self.times):
                    atomic_json(self.sequence_dir / "ready.json", {
                        "path": self.path, "output": self.jobs["sequences"][self.sequence_index]["output"],
                        "length": self.length, "times": self.times, "capture_size": [WIDTH * RENDER_SCALE, HEIGHT * RENDER_SCALE],
                        "mask": "inverse_opacity",
                        "capture_seconds": time.perf_counter() - self.sequence_started})
                    self.sequence_index += 1
                    self.phase = "sequence"
                else:
                    self.phase = "pose"
            self.status("running")
        except Exception:
            self.finish("failed", traceback.format_exc())

    def status(self, state, error=None, force=False):
        now = time.perf_counter()
        if force or now - self.last_status > 1:
            atomic_json(self.folder / "status.json", {"state": state, "sequences": self.sequence_index,
                        "frames": self.captured, "elapsed": now - self.started, "error": error})
            self.last_status = now

    def finish(self, state, error=None):
        global _active
        if self.handle is not None:
            unreal.unregister_slate_post_tick_callback(self.handle)
            self.handle = None
        cleanup_errors = []
        for actor in reversed(self.actors):
            try:
                self.eas.destroy_actor(actor)
            except Exception as failure:
                cleanup_errors.append(str(failure))
        self.actors.clear()
        for name in ("target", "mask_target"):
            target = getattr(self, name, None)
            if target is not None:
                try:
                    unreal.RenderingLibrary.release_render_target2d(target)
                except Exception as failure:
                    cleanup_errors.append(str(failure))
                setattr(self, name, None)
        if self.settings is not None and self.throttle is not None:
            try:
                self.settings.set_editor_property("bThrottleCPUWhenNotForeground", self.throttle, unreal.PropertyAccessChangeNotifyMode.NEVER)
            except Exception as failure:
                cleanup_errors.append(str(failure))
        if cleanup_errors:
            state, error = "failed", str(error or "") + " Cleanup: " + "; ".join(cleanup_errors)
        self.status(state, error, force=True)
        _active = None


def start(manifest):
    global _active
    if _active is not None:
        raise RuntimeError("Animation GIF capture is already running")
    if unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is not None:
        raise RuntimeError("Stop PIE before capturing animation GIFs")
    _active = Capture(manifest)
    try:
        _active.begin()
    except Exception:
        _active.finish("failed", traceback.format_exc())
        raise
