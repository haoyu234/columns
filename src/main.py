#!/usr/bin/env python3

"""
clcli.py
"""

import sys
import textwrap

sys.dont_write_bytecode = True

import ctypes
import getopt
import dataclasses
from io import StringIO
import os
import re
from clang.cindex import conf, register_function
from clang.cindex import (
    Config,
    Index,
    Cursor,
    CursorKind,
    TypeKind,
    TranslationUnit,
    SourceLocation,
)

import plugin_stub


@dataclasses.dataclass
class Context:
    tu: TranslationUnit
    header_sio: StringIO
    current_object_sio: StringIO
    object_stack: list[tuple[str, StringIO]]
    source_sio: StringIO
    parent_tp_str: str
    prev_cursor: Cursor
    source_code: bytes

    def push_new_object(self, parent_tp_str: str):
        self.prev_cursor = None
        self.object_stack.append((self.parent_tp_str, self.current_object_sio))
        self.parent_tp_str = parent_tp_str
        self.current_object_sio = StringIO()

    def pop_object(self):
        parent_tp_str, current_object_sio = self.object_stack.pop()
        self.source_sio.write(self.current_object_sio.getvalue())
        self.current_object_sio = current_object_sio
        self.parent_tp_str = parent_tp_str


def get_compile_args(include_dirs: list[str], standard: str) -> list[str]:
    compile_args = ["-fparse-all-comments"]

    if standard:
        compile_args.append(f"-std={standard}")

    for include_dir in include_dirs:
        compile_args.append(f"-I{include_dir}")

    return compile_args


def create_unit(path: str, include_dirs: list[str], standard: str) -> TranslationUnit:
    idx = Index.create()
    return idx.parse(
        path,
        args=get_compile_args(include_dirs, standard),
        options=TranslationUnit.PARSE_INCOMPLETE
        | TranslationUnit.PARSE_SKIP_FUNCTION_BODIES
        | TranslationUnit.PARSE_INCLUDE_BRIEF_COMMENTS_IN_CODE_COMPLETION,
    )


def in_system_header(location: SourceLocation) -> bool:
    # pylint: disable=no-member
    return conf.lib.clang_Location_isInSystemHeader(location) > 0


CHAR_TYPE = [
    TypeKind.CHAR_U,
    TypeKind.CHAR_S,
    TypeKind.UCHAR,
    TypeKind.CHAR16,
    TypeKind.CHAR32,
    TypeKind.USHORT,
    TypeKind.UINT,
    TypeKind.CHAR_S,
    TypeKind.SCHAR,
    TypeKind.SHORT,
    TypeKind.INT,
]

BASE_TYPE = [
    TypeKind.BOOL,
    TypeKind.CHAR_U,
    TypeKind.CHAR_S,
    TypeKind.UCHAR,
    TypeKind.CHAR16,
    TypeKind.CHAR32,
    TypeKind.USHORT,
    TypeKind.UINT,
    TypeKind.ULONG,
    TypeKind.ULONGLONG,
    TypeKind.UINT128,
    TypeKind.CHAR_S,
    TypeKind.SCHAR,
    TypeKind.SHORT,
    TypeKind.INT,
    TypeKind.LONG,
    TypeKind.LONGLONG,
    TypeKind.INT128,
    TypeKind.FLOAT,
    TypeKind.DOUBLE,
    TypeKind.LONGDOUBLE,
    TypeKind.ENUM,
]


def get_tp_str(cursor: Cursor) -> str:
    declaration = cursor.type.get_declaration()
    if not declaration.is_anonymous():
        return cursor.type.spelling

    child_name = declaration.spelling
    idx = child_name.index("(")
    child_object_name = get_unique_name(declaration)
    return child_name[:idx] + child_object_name


def get_unique_name(cursor: Cursor) -> str:
    return re.sub(r"[^a-zA-Z0-9]", "_", cursor.get_usr())


def process_string(cursor: Cursor, ctx: Context):
    element_type = cursor.type.get_array_element_type()

    if element_type.kind not in CHAR_TYPE:
        raise Exception("类型错误")

    element_type = element_type.get_canonical().get_declaration()

    if element_type.kind != CursorKind.NO_DECL_FOUND:
        raise Exception("类型错误")

    ctx.current_object_sio.write(
        f"    DEFINE_FIELD_STRING({ctx.parent_tp_str}, {cursor.spelling}),\n"
    )


def process_array(cursor: Cursor, ctx: Context):
    element_type = cursor.type.get_array_element_type()
    element_type_declaration = element_type.get_canonical().get_declaration()

    array_or_flexable_array = {True: "FLEXIBLE_ARRAY", False: "FIXED_ARRAY"}

    is_flexable_array = False
    if ctx.prev_cursor:
        is_flexable_array = plugin_stub.check_is_flexable_array(ctx.prev_cursor, cursor)
    prefix_str = array_or_flexable_array.get(is_flexable_array)

    if element_type_declaration.kind != CursorKind.NO_DECL_FOUND:
        element_name = plugin_stub.generate_object_name(element_type_declaration)
        ctx.current_object_sio.write(
            f"    DEFINE_FIELD_OBJECT_{prefix_str}({ctx.parent_tp_str}, {cursor.spelling}, {element_name}),\n"
        )
    else:
        ctx.current_object_sio.write(
            f"    DEFINE_FIELD_{prefix_str}({ctx.parent_tp_str}, {cursor.spelling}),\n"
        )


def process_field(cursor: Cursor, ctx: Context):
    canonical_type = cursor.type.get_canonical()
    element_type_declaration = canonical_type.get_declaration()

    if canonical_type.kind in BASE_TYPE:
        ctx.prev_cursor = cursor
        ctx.current_object_sio.write(
            f"    DEFINE_FIELD_NUMBER({ctx.parent_tp_str}, {cursor.spelling}),\n"
        )
        return

    if canonical_type.kind in (
        TypeKind.CONSTANTARRAY,
        TypeKind.VARIABLEARRAY,
    ):
        comment = cursor.raw_comment
        if comment:
            if "@string" in comment:
                process_string(cursor, ctx)
                return

        element_type = cursor.type.get_array_element_type()
        if element_type.spelling == "char":
            process_string(cursor, ctx)
            return

        process_array(cursor, ctx)
        return

    if element_type_declaration.kind in (CursorKind.STRUCT_DECL, CursorKind.UNION_DECL):
        struct_or_union = {
            CursorKind.UNION_DECL: "DEFINE_FIELD_UNION",
            CursorKind.STRUCT_DECL: "DEFINE_FIELD_OBJECT",
        }

        prefix_str = struct_or_union.get(element_type_declaration.kind)

        unique_name = get_unique_name(element_type_declaration)

        ctx.current_object_sio.write(
            f"    {prefix_str}({ctx.parent_tp_str}, {cursor.spelling}, {unique_name}),\n"
        )


def process_inline_union_or_struct(cursor: Cursor, ctx: Context):
    extent = cursor.extent

    unique_name = get_unique_name(cursor)

    start = extent.start.offset
    end = extent.end.offset

    source_code = str(ctx.source_code[start:end], encoding="UTF-8")
    open_curly_idx = source_code.index("{")

    idx = open_curly_idx - 1
    while idx >= 0 and source_code[idx].isspace():
        idx = idx - 1

    idx = idx + 1
    source_code = source_code[:idx] + f" {unique_name}" + source_code[idx:] + ";\n"
    source_code = textwrap.dedent(" " * (extent.start.column - 1) + source_code)

    ctx.push_new_object(unique_name)
    ctx.current_object_sio.write(source_code)
    ctx.pop_object()


def process_union_or_struct(cursor: Cursor, ctx: Context):
    location = cursor.location
    fname = os.path.basename(location.file.name)
    line = location.line
    column = location.column

    if not cursor.is_anonymous():
        plugin_stub.begin_object(cursor)
        object_name = plugin_stub.generate_object_name(cursor)
    unique_name = get_unique_name(cursor)

    ctx.current_object_sio.write(f"// {fname}:{line}:{column}\n")
    ctx.current_object_sio.write(f"static const clColumn {unique_name}[] = {{\n")

    for child in cursor.get_children():
        name = child.spelling

        if child.kind == CursorKind.UNION_DECL or child.kind == CursorKind.STRUCT_DECL:
            process_inline_union_or_struct(child, ctx)
            continue

        if child.kind != CursorKind.FIELD_DECL:
            continue

        if child.is_anonymous():
            tp_str = get_tp_str(child)
            ctx.push_new_object(tp_str)
            process_union_or_struct(child.type.get_declaration(), ctx)
            ctx.pop_object()

        process_field(child, ctx)

    ctx.current_object_sio.write("};\n")

    if cursor.kind != CursorKind.STRUCT_DECL:
        return

    if not cursor.is_anonymous():
        ctx.current_object_sio.write(f"const clColumn {object_name}[] = {{\n")
        ctx.current_object_sio.write(
            f"    DEFINE_OBJECT({ctx.parent_tp_str}, {unique_name}),\n"
        )

        ctx.current_object_sio.write("};\n")
        ctx.header_sio.write(f"extern const struct clColumn {object_name}[];\n")

        plugin_stub.end_object(cursor, object_name)


def search_union_or_struct(cursor: Cursor, ctx: Context):
    if not cursor.spelling:
        return

    ctx.push_new_object(cursor.type.spelling)
    process_union_or_struct(cursor, ctx)
    ctx.pop_object()


def search_namespace_or_union_or_struct(cursor: Cursor, ctx: Context):
    actions = {
        CursorKind.NAMESPACE: search_namespace_or_union_or_struct,
        CursorKind.STRUCT_DECL: search_union_or_struct,
        CursorKind.UNION_DECL: search_union_or_struct,
    }

    for child in cursor.get_children():
        if child.kind not in (
            CursorKind.STRUCT_DECL,
            CursorKind.NAMESPACE,
            CursorKind.CLASS_DECL,
            CursorKind.UNION_DECL,
        ):
            continue

        if in_system_header(child.location):
            continue

        if not child.is_definition():
            continue

        action = actions.get(child.kind, None)
        if action:
            action(child, ctx)
            continue


SOURCE_CODE_TPL = """
// generated by the columns. DO NOT EDIT!

#define USE_COLUMN_MACROS
#include <columns.h>

$includes

#ifdef __cplusplus
extern "C" {
#endif

$code

$extra_output
#ifdef __cplusplus
}
#endif
"""

HEADER_CODE_TPL = """
#pragma once

// generated by the columns. DO NOT EDIT!
#include <columns.h>

$includes

#ifdef __cplusplus
extern "C" {
#endif
struct clColumn;

$code

$extra_output
#ifdef __cplusplus
}
#endif
"""


def render_tpl(tpl: str, data: dict[str, str]) -> str:
    result = tpl

    for k, v in data.items():
        result = re.sub(f"\\${k}", v, result)

    result = result.strip() + "\n"
    result = re.sub("(\\r*\\n{3,})", "\r\n\r\n", result)
    return result


def render_include(path: str) -> str:
    return f'#include "{path}"\n'


def guess_suffix(standard: str) -> str:
    if "+" in standard:
        return "cpp"
    return "c"


def main():
    inputs = []
    includes = []
    work_dir = os.getcwd()
    standard = "c11"
    plugin = ""

    opts, args = getopt.getopt(sys.argv[1:], "C:I:p:", ["std="])
    for opt in opts:
        if opt[0] == "-C":
            work_dir = opt[1]
        elif opt[0] == "-I":
            includes.append(opt[1])
        elif opt[0] == "--std=":
            standard = opt[1]
        elif opt[0] == "-p":
            plugin = opt[1]

    inputs.extend(args)

    os.chdir(work_dir)
    work_dir = os.getcwd()

    outputs = dict()

    if len(plugin) > 0:
        plugin_stub.load_plugin(plugin)

    for path in inputs:
        plugin_stub.init()

        source_code = bytes()

        with open(path, "rb") as f:
            source_code = f.read()

        tu = create_unit(path, includes, standard)

        path = tu.spelling
        header_path = os.path.relpath(path, work_dir)
        ctx = Context(
            tu=tu,
            header_sio=StringIO(),
            current_object_sio=StringIO(),
            object_stack=list(),
            source_sio=StringIO(),
            parent_tp_str="",
            prev_cursor=None,
            source_code=source_code,
        )

        search_namespace_or_union_or_struct(tu.cursor, ctx)

        extra_output = plugin_stub.complete()
        if not extra_output:
            extra_output = ("", "")

        stem, _ = os.path.splitext(header_path)

        includes_str = ""
        if len(includes) > 0:
            includes_str = "".join(map(render_include, includes))

        outputs[stem] = (
            render_tpl(
                HEADER_CODE_TPL,
                {
                    "includes": includes_str,
                    "code": ctx.header_sio.getvalue().strip(),
                    "extra_output": extra_output[0],
                },
            ),
            render_tpl(
                SOURCE_CODE_TPL,
                {
                    "includes": render_include(header_path),
                    "code": ctx.source_sio.getvalue().strip(),
                    "extra_output": extra_output[1],
                },
            ),
        )

    suffix = guess_suffix(standard)

    for stem, (header, source) in outputs.items():
        with open(f"{stem}_def.h", mode="w", encoding="UTF-8") as out:
            out.write(header)

        with open(f"{stem}_def.{suffix}", mode="w", encoding="UTF-8") as out:
            out.write(source)


if __name__ == "__main__":
    functions = [
        ("clang_Location_isInSystemHeader", [SourceLocation], ctypes.c_int),
    ]

    for item in functions:
        register_function(conf.lib, item, not Config.compatibility_check)

    main()
