"""
plugin
"""

import re
import tomllib
from clang.cindex import Cursor


def generate_object_name(cursor: Cursor) -> str:
    return cursor.spelling + "Object"


def end_object(cursor: Cursor, name: str):
    comment = cursor.raw_comment
    if not comment:
        return

    comment = re.sub(r"(?://)|(?:/\*)|(?:\*/)", "", comment)
    comment = comment.strip()

    comment_dict = tomllib.loads(comment)
    cmd = comment_dict.get("cmd", None)
    if cmd:
        print(f"cmd: {cmd}, {name}")
