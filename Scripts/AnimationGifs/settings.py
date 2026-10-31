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
# (메시, 본, 위치, 회전). 회전은 소켓 Details 패널 순서(Roll, Pitch, Yaw)이며 값은 각 팩 스켈레톤의 무기 소켓입니다.
# Warrior 팩에는 방패 소켓이 없습니다. 방패 피벗이 뒷면 손잡이 자리라 Dual_Wield 왼손 소켓(Axe)을 메시 Y축으로
# 180도 돌려 앞면(+Z)이 바깥을 보게 하고, 주먹이 뒷면에 묻히지 않게 앞면 쪽으로 5cm 띄웠습니다.
PACK_WEAPONS = {
    "Warrior": [("Sword", "hand_r", (-10.797201, 4.788953, -1.161483), (87.922997, 12.627769, -10.096642)),
                ("Shield", "hand_l", (11.668250, -1.395718, 2.202199), (-88.0, -9.846271, 164.745805))],
    "Dual_Wield": [("Sword", "hand_r", (-10.463401, 4.405117, -1.218026), (91.067833, 9.846496, -1.753910)),
                   ("Axe", "hand_l", (10.382331, -6.224471, 2.030272), (-92.0, 9.846271, -15.254195))],
}


def output_relative(path):
    if "/Fab/ARPG_Pack/" in path:
        return Path(path.split("/")[4].removeprefix("ARPG_")) / (path.rsplit("/", 1)[-1] + ".gif")
    if "/Fab/Dodge_and_Evade_Anims/" in path:
        return Path("Dodge_and_Evade") / (path.rsplit("/", 1)[-1] + ".gif")
    if "/Fab/Dodge_Animations/" in path:
        return Path("Dodge_Animations") / (path.rsplit("/", 1)[-1] + ".gif")
    if "/Fab/ACS2ComboSkill/" in path:
        return Path("ACS2ComboSkill") / (path.rsplit("/", 1)[-1] + ".gif")
    return Path("Base") / Path(path.removeprefix("/Game/")).with_suffix(".gif")
