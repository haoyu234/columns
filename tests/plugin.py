"""
plugin
"""

import re
import tomllib
import typing
from clang.cindex import Cursor


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


def complete() -> typing.Tuple[str, str]:
    return ("// extra_output 1", "// extra_output 2")
