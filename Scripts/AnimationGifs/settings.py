# Copyright Woogle. All Rights Reserved.
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WIDTH, HEIGHT, HEADER = 480, 401, 56
HEADER_COLOR = (20, 23, 28)
HEADER_FONT_SIZE = 20
RENDER_SCALE = 2
CAMERA_ZOOM = 1.6
WARMUP_FRAMES = 3
FPS = 20
HOLD_START, HOLD_END = 300, 500
CAMERA_OFFSET = (-430.0, 270.0, 230.0)
# 기존 뷰포트의 수평 90도 투영에서 1774픽셀 중 840픽셀을 잘랐던 구도입니다.
CROP_FRACTION = 840 / 1774
BACKGROUND = (68, 102, 150)
PACK_WEAPONS = {
    "Warrior": [("Sword", "hand_r", (-1.163383, 1.317055, -0.519517)),
                ("Shield", "hand_l", (1.163378, -1.317059, 0.519516))],
    "Dual_Wield": [("Sword", "hand_r", (-1.163383, 1.317055, -0.519517)),
                   ("Axe", "hand_l", (-1.163383, 1.317055, -0.519517))],
}


def output_relative(path):
    if "/Fab/ARPG_Pack/" in path:
        return Path(path.split("/")[4].removeprefix("ARPG_")) / (path.rsplit("/", 1)[-1] + ".gif")
    if "/Fab/Dodge_and_Evade_Anims/" in path:
        return Path("Dodge_and_Evade") / (path.rsplit("/", 1)[-1] + ".gif")
    if "/Fab/Dodge_Animations/" in path:
        return Path("Dodge_Animations") / (path.rsplit("/", 1)[-1] + ".gif")
    return Path("Base") / Path(path.removeprefix("/Game/")).with_suffix(".gif")
