import importlib.util
import os
import re
from clang.cindex import Cursor


def import_module_by_path(root, module_path, module_name):
    path = os.path.join(root, module_path)
    spec = importlib.util.spec_from_file_location(module_name, path)
    if spec:
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        print("plugin", module_path, "loaded")
        return module


_plugin = None


def load_plugin(path: str):
    global _plugin
    _plugin = import_module_by_path(os.getcwd(), path, "plugin")


def _plugin_stub(fn):
    def wrapper(*args, **kwargs):
        global _plugin
        if _plugin:
            if hasattr(_plugin, fn.__name__):
                return getattr(_plugin, fn.__name__)(*args, **kwargs)
        return fn(*args, **kwargs)

    return wrapper


@_plugin_stub
def generate_columns_name(cursor: Cursor) -> str:
    return re.sub(r"[^a-zA-Z0-9]", "_", cursor.get_usr())


@_plugin_stub
def generate_object_name(cursor: Cursor) -> str:
    return f"{cursor.spelling}Object"


def check_is_len(prev_name: str, name: str) -> bool:
    prefixs = ["num"]
    suffixs = ["num", "size", "count", "len"]

    prev_name_lower = prev_name.lower()

    all_keywords = set(prefixs).union(suffixs)
    if prev_name_lower in all_keywords:
        return True

    for s in prefixs:
        s2 = f"{name}{s}".lower()
        s3 = f"{name}_{s}".lower()

        if prev_name_lower == s2 or prev_name_lower == s3:
            return True

    for s in suffixs:
        s2 = f"{s}{name}".lower()
        s3 = f"{s}_{name}".lower()

        if prev_name_lower == s2 or prev_name_lower == s3:
            return True

    return False


@_plugin_stub
def check_is_flexable_array(cursor1: Cursor, cursor2: Cursor) -> bool:
    return check_is_len(cursor1.spelling, cursor2.spelling)


@_plugin_stub
def begin_object(cursor: Cursor):
    pass


@_plugin_stub
def end_object(cursor: Cursor, name: str):
    pass


@_plugin_stub
def init():
    pass


@_plugin_stub
def complete():
    pass
