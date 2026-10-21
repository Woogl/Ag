# Copyright Woogle. All Rights Reserved.
"""촬영된 시퀀스 GIF를 구간·배속 정의에 따라 조립합니다."""
import bisect
import importlib.util
import math
from pathlib import Path

from PIL import Image, ImageSequence

ROOT = Path(__file__).resolve().parents[2]
_spec = importlib.util.spec_from_file_location("animation_gif_settings", ROOT / "Scripts/AnimationGifs/settings.py")
SOURCE = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(SOURCE)
OUTPUT_WIDTH = 480
OUTPUT_HEIGHT = math.floor((SOURCE.HEIGHT + SOURCE.HEADER) * OUTPUT_WIDTH / SOURCE.WIDTH + 0.5)


def read_clip(clip):
    pack, name = clip.split("/", 1)
    path = ROOT / "Docs/Art/GIF" / pack / ("ARPG_" + pack + "_Anim_UE_" + name + ".gif")
    frames, samples = [], []
    with Image.open(path) as image:
        if image.size not in ((640, 574), (SOURCE.WIDTH, SOURCE.HEIGHT + SOURCE.HEADER)):
            raise ValueError("원본 GIF 크기가 다릅니다: " + str(path))
        count = image.n_frames
        for index, frame in enumerate(ImageSequence.Iterator(image)):
            duration = frame.info["duration"]
            if index == 0:
                duration -= SOURCE.HOLD_START
            if index == count - 1:
                duration -= SOURCE.HOLD_END
            sample_count = round(duration * SOURCE.FPS / 1000)
            if sample_count < 1 or abs(sample_count * 1000 / SOURCE.FPS - duration) > 0.01:
                raise ValueError("원본 GIF의 프레임 시간이 다릅니다: " + str(path))
            body = frame.convert("RGB")
            # 축소 뒤에도 원본 프레임의 색으로 매핑해 새 팔레트의 색 변화를 피합니다.
            indexed = body.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
            if body.size != (OUTPUT_WIDTH, OUTPUT_HEIGHT):
                resized = body.resize((OUTPUT_WIDTH, OUTPUT_HEIGHT), Image.Resampling.LANCZOS)
                indexed = resized.quantize(palette=indexed, dither=Image.Dither.NONE)
            frames.append(indexed)
            samples.extend([len(frames) - 1] * sample_count)
    return frames, samples


def assemble_sheet(sheet, out_dir, fps=20, hold_start=8, hold_end=10):
    animation = sheet["animation"]
    rate = animation.get("play_rate", 1.0)
    if not math.isfinite(rate) or rate <= 0:
        raise ValueError("play_rate는 양수여야 합니다")
    clips = {clip: read_clip(clip) for clip in dict.fromkeys(segment["clip"] for segment in animation["segments"])}
    segments, starts, total = [], [], 0.0
    for segment in animation["segments"]:
        begin = segment["from"]
        end = segment.get("to", begin)
        speed = segment.get("rate", 1.0) * rate
        if not math.isfinite(speed) or speed <= 0:
            raise ValueError("구간 배속은 양수여야 합니다")
        duration = segment["hold"] if "hold" in segment else (end - begin) / speed
        if not all(math.isfinite(value) for value in (begin, end, speed, duration)) or speed <= 0 or duration <= 0 or begin < 0 or end < begin:
            raise ValueError("잘못된 모션 구간: " + str(segment))
        if end * SOURCE.FPS > len(clips[segment["clip"]][1]) - 1 + 0.01:
            raise ValueError("원본 GIF 범위를 넘는 구간: " + str(segment))
        starts.append(total)
        segments.append((segment["clip"], begin, end, 0 if "hold" in segment else speed))
        total += duration
    if not segments:
        raise ValueError("모션 구간이 없습니다")
    frames = []
    for index in range(-hold_start, round(total * fps) + 1 + hold_end):
        moment = min(max(index / fps, 0), total)
        segment_index = bisect.bisect_right(starts, moment + 1e-9) - 1
        clip, begin, end, speed = segments[segment_index]
        position = min(begin + (moment - starts[segment_index]) * speed, end)
        source_frames, samples = clips[clip]
        sample = min(round(position * SOURCE.FPS), len(samples) - 1)
        frames.append(source_frames[samples[sample]])
    output = Path(out_dir) / (sheet["image"] + ".gif")
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(".gif.tmp")
    try:
        frames[0].save(temporary, format="GIF", save_all=True, append_images=frames[1:],
                       duration=round(1000 / fps), loop=0, optimize=False, disposal=1)
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)
    print(str(output), flush=True)
    return output
