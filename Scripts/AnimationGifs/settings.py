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
# 촬영용 임시 캐릭터는 레벨의 물체에서 멀리 떨어진 곳에 둡니다. 무기는 blueprint의 게임 무기를 그대로 씁니다.
CHARACTERS = {
    "player": {"mesh": "/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple", "blueprint": "/Game/Characters/BP_Player",
               "actor": (0.0, -20000.0, 0.0)},
    "boss": {"mesh": "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple", "blueprint": "/Game/Characters/BP_Boss",
             "actor": (0.0, 20000.0, 0.0)},
}
# 게임 무기 소켓이 붙은 본입니다. 게임 무기를 이 본 기준으로 재고 붙입니다.
WEAPON_BONE = "hand_r"
# ARPG 팩이 아닌 팩의 경로와 GIF 폴더 이름입니다. 이 팩들은 플레이어 마네킹으로 찍습니다.
PLAYER_PACKS = {"/Fab/Dodge_and_Evade_Anims/": "Dodge_and_Evade", "/Fab/ACS2ComboSkill/": "ACS2ComboSkill"}


def output_relative(path):
    if "/Fab/ARPG_Pack/" in path:
        return Path(path.split("/")[4].removeprefix("ARPG_")) / (path.rsplit("/", 1)[-1] + ".gif")
    for folder, name in PLAYER_PACKS.items():
        if folder in path:
            return Path(name) / (path.rsplit("/", 1)[-1] + ".gif")
    return Path("Base") / Path(path.removeprefix("/Game/")).with_suffix(".gif")
