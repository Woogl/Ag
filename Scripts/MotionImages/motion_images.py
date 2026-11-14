# Copyright Woogle. All Rights Reserved.
"""sheets/의 조합 정의로 기획서 모션 그림(Docs/Art/Attachments/<그림 이름>.gif)을 만듭니다. 쓰는 법은 README.md를 따릅니다.

    python Scripts/MotionImages/motion_images.py assemble [image ...] [--out DIR]

image는 .json을 뺀 조합 정의 파일 이름이고, 없으면 모든 그림입니다.
"""
import argparse
import glob
import json
import os
import sys

sys.dont_write_bytecode = True   # 저장소에 __pycache__를 남기지 않습니다
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from assemble import assemble_sheet  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
SHEETS = os.path.join(HERE, "sheets")
ATTACHMENTS = os.path.join(os.path.dirname(os.path.dirname(HERE)), "Docs", "Art", "Attachments")


def load_sheets(names):
    sheets = []
    for path in sorted(glob.glob(os.path.join(SHEETS, "*.json"))):
        with open(path, encoding="utf-8") as f:
            sheets.append(json.load(f))
    unknown = set(names) - {s["image"] for s in sheets}
    if unknown:
        raise SystemExit("조합 정의가 없습니다: %s" % ", ".join(sorted(unknown)))
    return [s for s in sheets if not names or s["image"] in names]


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=["assemble"])
    parser.add_argument("images", nargs="*")
    parser.add_argument("--out", default=ATTACHMENTS, help="그림을 둘 폴더 (기본 Docs/Art/Attachments)")
    args = parser.parse_args()
    for sheet in load_sheets(args.images):
        assemble_sheet(sheet, args.out)


if __name__ == "__main__":
    main()
