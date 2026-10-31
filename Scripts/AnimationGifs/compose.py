# Copyright Woogle. All Rights Reserved.
import json
import time
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

from settings import BACKGROUND, FPS, HEADER, HEADER_COLOR, HEADER_FONT_SIZE, HEIGHT, HOLD_END, HOLD_START, WIDTH

FONT_PATH = "C:/Windows/Fonts/arial.ttf"


def compose_sequence(sequence_dir):
    folder = Path(sequence_dir)
    meta = json.loads((folder / "ready.json").read_text(encoding="utf-8"))
    if meta.get("mask") != "inverse_opacity":
        raise ValueError("이전 색상 키잉 프레임에는 실루엣 정보가 없습니다. 새 설정으로 재촬영하세요.")
    start = time.perf_counter()
    font = ImageFont.truetype(FONT_PATH, HEADER_FONT_SIZE)
    name = meta["path"].rsplit("/", 1)[-1]
    # 긴 시퀀스 이름은 띠 너비에 들어갈 때까지만 글씨를 줄입니다.
    name_font = font
    while name_font.size > 12 and name_font.getlength(name) > WIDTH - 12:
        name_font = ImageFont.truetype(FONT_PATH, name_font.size - 1)
    frames, durations = [], []
    for index, moment in enumerate(meta["times"]):
        with Image.open(folder / ("%06d.png" % index)) as source:
            if source.size != tuple(meta.get("capture_size", (WIDTH, HEIGHT))):
                raise ValueError("Unexpected capture size: %s" % (source.size,))
            frame = source.convert("RGB")
        with Image.open(folder / ("%06d.mask.png" % index)) as mask_image:
            if mask_image.size != frame.size or mask_image.mode != "RGBA":
                raise ValueError("Invalid inverse opacity capture")
            inverse_opacity = mask_image.getchannel("A")
        frame = Image.composite(Image.new("RGB", frame.size, BACKGROUND), frame, inverse_opacity)
        if frame.size != (WIDTH, HEIGHT):
            frame = frame.resize((WIDTH, HEIGHT), Image.Resampling.LANCZOS)
        canvas = Image.new("RGB", (WIDTH, HEIGHT + HEADER), HEADER_COLOR)
        canvas.paste(frame, (0, HEADER))
        draw = ImageDraw.Draw(canvas)
        draw.text((6, 4), name, font=name_font, fill="white")
        label = "%.2f / %.2f s | 1x | %d fps" % (moment, meta["length"], FPS)
        if not meta.get("weapon", "/Fab/" in meta["path"]):
            label += " | body motion (no weapon)"
        draw.text((6, 30), label, font=font, fill=(170, 190, 210))
        frames.append(canvas)
        durations.append(round(1000 / FPS))
    durations[0] += HOLD_START
    durations[-1] += HOLD_END
    # 전체 구간의 공통 팔레트로 같은 표면의 색이 프레임마다 바뀌는 현상을 줄입니다.
    sample_count = min(64, len(frames))
    samples = Image.new("RGB", (128, 115 * sample_count))
    for index in range(sample_count):
        frame_index = round(index * (len(frames) - 1) / max(1, sample_count - 1))
        samples.paste(frames[frame_index].resize((128, 115), Image.Resampling.LANCZOS), (0, 115 * index))
    palette = samples.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    frames = [frame.quantize(palette=palette, dither=Image.Dither.NONE) for frame in frames]
    dest = Path(meta["output"])
    dest.parent.mkdir(parents=True, exist_ok=True)
    temporary = dest.with_name(dest.name + "." + folder.parent.name + ".tmp")
    try:
        frames[0].save(temporary, format="GIF", save_all=True, append_images=frames[1:], duration=durations, loop=0, optimize=True)
        temporary.replace(dest)
    finally:
        temporary.unlink(missing_ok=True)
    result = {"output": str(dest), "frames": len(frames), "compose_seconds": time.perf_counter() - start}
    (folder / "composed.json").write_text(json.dumps(result), encoding="utf-8")
    return result
